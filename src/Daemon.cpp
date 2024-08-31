
#include "Daemon.hpp"

Daemon *Daemon::instance = nullptr;

Daemon::Daemon() {
	instance = this;
	pid = 0;
	if (!this->check_requirements()) {
		exit(1);
	}
	if (!this->daemonize()) {
		std::cerr << "Error: Failed to daemonize." << std::endl;
		exit(1);
	}
	signal(SIGTERM, Daemon::handle_signal);
	signal(SIGINT, Daemon::handle_signal);
	signal(SIGHUP, Daemon::handle_signal);
	bzero(client_fds, sizeof(client_fds));
}

Daemon::~Daemon() {
	this->reporter.log("Daemon stopped.");
	close_sockets();
	try {
		std::filesystem::remove("/var/run/matt_daemon.lock");
	} catch (const std::filesystem::filesystem_error &e) {
		reporter.error("Failed to remove lock file: " + std::string(e.what()));
	}
}

Daemon::Daemon(const Daemon &d) {
	pid = d.pid;
}

Daemon &Daemon::operator=(const Daemon &d) {
	pid = d.pid;
	return *this;
}

unsigned int Daemon::getPid() {
	return pid;
}

bool Daemon::check_requirements() {
	if (getuid() != 0 && geteuid() != 0) {
		std::cerr << "You must be root to run this program." << std::endl;
		return false;
	}
	return true;
}

void Daemon::handle_signal(int signal) {
	TintinReporter reporter;

	if (signal == SIGTERM || signal == SIGINT) {
		Daemon::instance->close_sockets();
		std::filesystem::remove("/var/run/matt_daemon.lock");
		if (signal == SIGTERM)
			reporter.log("[SIGTERM] Daemon stopped.");
		else
			reporter.log("[SIGINT] Daemon stopped.");
		exit(0);
	} else if (signal == SIGHUP) {
		reporter.log("[SIGHUP] Daemon reloaded.");
	} else {
		reporter.log("Unknown signal received.");
	}
}

void Daemon::close_sockets() {
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		if (client_fds[i] > 0) {
			close(client_fds[i]);
		}
	}
	if (server_fd > 0) {
		close(server_fd);
	}
}

bool Daemon::daemonize(void) {
	pid_t pid, sid;

	if (getuid() != 0 && geteuid() != 0) {
		std::cerr << "You must be root to run this program." << std::endl;
		return false;
	}
	pid = fork();
	if (pid < 0) {
		std::cerr << "Fork failed: " << strerror(errno) << std::endl;
		return false;
	}
	if (pid > 0)
		exit(EXIT_SUCCESS);
	umask(0);
	sid = setsid();
	if (sid < 0) {
		std::cerr << "Failed to create a new session: " << strerror(errno) << std::endl;
		return false;
	}
	if ((chdir("/")) < 0) {
		std::cerr << "Failed to change directory to /: " << strerror(errno) << std::endl;
		return false;
	}
	if (close(STDIN_FILENO) < 0 || close(STDOUT_FILENO) < 0 || close(STDERR_FILENO) < 0) {
		std::cerr << "Failed to close standard file descriptors: " << strerror(errno) << std::endl;
		return false;
	}
	if (open("/dev/null", O_RDONLY) < 0 || open("/dev/null", O_WRONLY) < 0 || open("/dev/null", O_WRONLY) < 0) {
		std::cerr << "Failed to redirect standard file descriptors to /dev/null: " << strerror(errno) << std::endl;
		return false;
	}
	signal(SIGTERM, handle_signal);
	signal(SIGINT, handle_signal);
	signal(SIGHUP, handle_signal);
	return true;
}

int Daemon::start_remote_shell() {
	struct sockaddr_in address;
	int                addrlen = sizeof(address);

	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		reporter.error("Socket: " + std::string(strerror(errno)));
		return -1;
	}

	int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
		reporter.error("setsockopt: " + std::string(strerror(errno)));
		close(server_fd);
		return -1;
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		reporter.error("Bind: " + std::string(strerror(errno)));
		close(server_fd);
		return -1;
	}

	if (listen(server_fd, MAX_CLIENTS) < 0) {
		reporter.error("Listen: " + std::string(strerror(errno)));
		close(server_fd);
		return -1;
	}

	reporter.log("Daemon listening on port " + std::to_string(PORT));

	while (true) {
		FD_ZERO(&readfds);

		FD_SET(server_fd, &readfds);
		int max_sd = server_fd;

		for (int i = 0; i < MAX_CLIENTS; ++i) {
			int sd = client_fds[i];

			if (sd > 0) {
				FD_SET(sd, &readfds);
			}

			if (sd > max_sd) {
				max_sd = sd;
			}
		}

		int activity = select(max_sd + 1, &readfds, nullptr, nullptr, nullptr);

		if ((activity < 0) && (errno != EINTR)) {
			reporter.error("Select: " + std::string(strerror(errno)));
		}

		if (FD_ISSET(server_fd, &readfds)) {
			int new_socket;
			if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
				reporter.error("Accept: " + std::string(strerror(errno)));
				continue;
			}

			reporter.log("New connection, socket fd: " + std::to_string(new_socket) +
			             ", ip: " + inet_ntoa(address.sin_addr) +
			             ", port: " + std::to_string(ntohs(address.sin_port)));

			for (int i = 0; i < MAX_CLIENTS; ++i) {
				if (client_fds[i] == 0) {
					client_fds[i] = new_socket;
					break;
				}
			}
		}

		for (int i = 0; i < MAX_CLIENTS; ++i) {
			int sd = client_fds[i];

			if (FD_ISSET(sd, &readfds)) {
				handle_client(sd);
			}
		}
	}

	close_sockets();
	return 0;
}

void Daemon::handle_client(int client_socket) {
	char        buffer[1024];
	int         bytes_read;
	std::string cmd;

	std::string prompt = "$ ";
	send(client_socket, prompt.c_str(), prompt.size(), 0);
	reporter.log("Sent prompt to client.");

	bzero(buffer, sizeof(buffer));
	bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);

	if (bytes_read <= 0) {
		if (bytes_read == 0) {
			reporter.log("Client disconnected.");
		} else {
			reporter.error("Read error: " + std::string(strerror(errno)));
		}
		close(client_socket);

		for (int i = 0; i < MAX_CLIENTS; i++) {
			if (client_fds[i] == client_socket) {
				client_fds[i] = 0;
				break;
			}
		}
	} else {
		buffer[bytes_read] = '\0';
		cmd = std::string(buffer);
		// Remove trailing newline
		cmd.erase(std::remove(cmd.begin(), cmd.end(), '\n'), cmd.end());
		cmd.erase(std::remove(cmd.begin(), cmd.end(), '\r'), cmd.end());
		if (Daemon::tolower(cmd) == "quit\r\n") {
			reporter.debug("[" + std::to_string(client_socket) + "] Client requested to exit.");
			close(client_socket);
			for (int i = 0; i < MAX_CLIENTS; i++) {
				if (client_fds[i] == client_socket) {
					client_fds[i] = 0;
					break;
				}
			}
			return;
		}
		reporter.debug("[" + std::to_string(client_socket) + "] Received (" + std::to_string(bytes_read) + "): " + cmd);

		int result = execute_command(cmd, client_socket);

		std::string response = "Command executed with result: " + std::to_string(result) + "\n";
		send(client_socket, response.c_str(), response.size(), 0);
		reporter.debug("Sent command result to client.");

		send(client_socket, prompt.c_str(), prompt.size(), 0);
	}
}

std::string Daemon::tolower(std::string str) {
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	return str;
}

int Daemon::execute_command(const std::string &command, int client_socket) {
	std::string           sanitized_command;
	std::array<char, 128> buffer;
	std::string           result;

	
	if (command.empty()) {
		reporter.error("Empty command received.");
		return -1;
	}

	for (const char &ch : command) {
		if (std::isalnum(ch) || std::isspace(ch) || ch == '.' || ch == '-' || ch == '/' || ch == '_') {
			sanitized_command += ch;
		} else if (UNSAFE) {
			sanitized_command += ch;
		} else {
			reporter.error("Invalid character detected in command: " + std::string(1, ch));
			return -1;
		}
	}
	sanitized_command = "/bin/sh -c \"" + sanitized_command + "\"";

	if (sanitized_command.empty()) {
		reporter.error("Sanitized command is empty, nothing to execute.");
		return -1;
	}

	sanitized_command += " 2>&1";

	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(sanitized_command.c_str(), "r"), pclose);

	if (!pipe) {
		reporter.error("Failed to open pipe for command execution.");
		return -1;
	}

	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
		result += buffer.data();
	}

	int exit_code = WEXITSTATUS(pclose(pipe.release()));

	if (exit_code != 0) {
		reporter.error("Command execution failed with exit code " + std::to_string(exit_code) + ": " + result);
		return exit_code;
	}

	reporter.debug("[" + std::to_string(client_socket) + "] Command executed successfully: " + result);
	return exit_code;
}