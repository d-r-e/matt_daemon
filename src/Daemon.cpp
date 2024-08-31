
#include "Daemon.hpp"

Daemon::Daemon() {
	pid = 0;
	if (!this->check_requirements()) {
		exit(1);
	}
	if (!this->daemonize()) {
		std::cerr << "Error: Failed to daemonize." << std::endl;
		exit(1);
	}
	bzero(client_fds, sizeof(client_fds));
}

Daemon::~Daemon() {
	this->reporter.log("Daemon stopped.");
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
	std::ifstream pid_file("/var/run/matt_daemon.lock");
	pid_t         pid;
	pid_t         sid;

	if (std::filesystem::exists("/var/run/matt_daemon.lock")) {
		std::cerr << "Error: Daemon is already running." << std::endl;
		return false;
	}

	std::ofstream pid_file_write("/var/run/matt_daemon.lock");
	if (!pid_file_write.is_open()) {
		std::cerr << "Error: Failed to create pid file." << std::endl;
		return false;
	}
	pid_file_write << getpid();
	pid_file_write.close();
	pid = fork();
	if (pid < 0) {
		std::cerr << "Error: Failed to fork." << std::endl;
		return false;
	}
	if (pid > 0) {
		exit(0);
	}

	pid = fork();
	if (pid < 0) {
		std::cerr << "Error: Failed to fork." << std::endl;
		return false;
	}
	if (pid > 0) {
		exit(0);
	}
	umask(0);
	sid = setsid();
	if (sid < 0) {
		std::cerr << "Error: Failed to setsid." << std::endl;
		return false;
	}
	if ((chdir("/")) < 0) {
		std::cerr << "Error: Failed to chdir." << std::endl;
		return false;
	}

	close(STDIN_FILENO);
	close(STDOUT_FILENO);
	close(STDERR_FILENO);
	this->pid = getpid();

	signal(SIGTERM, Daemon::handle_signal);
	signal(SIGINT, Daemon::handle_signal);
	signal(SIGHUP, Daemon::handle_signal);

	std::string start_message = "Daemon started with PID: " + std::to_string(this->pid);
	this->reporter.log(start_message);
	return true;
}

int Daemon::start_remote_shell() {
	struct sockaddr_in address;
	int                addrlen = sizeof(address);

	if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		reporter.error("Socket creation failed: " + std::string(strerror(errno)));
		return -1;
	}

	int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
		reporter.error("setsockopt failed: " + std::string(strerror(errno)));
		close(server_fd);
		return -1;
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(PORT);

	if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		reporter.error("Bind failed: " + std::string(strerror(errno)));
		close(server_fd);
		return -1;
	}

	if (listen(server_fd, MAX_CLIENTS) < 0) {
		reporter.error("Listen failed: " + std::string(strerror(errno)));
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
			reporter.error("Select error: " + std::string(strerror(errno)));
		}

		if (FD_ISSET(server_fd, &readfds)) {
			int new_socket;
			if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
				reporter.error("Accept failed: " + std::string(strerror(errno)));
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
	char buffer[1024];
	int  bytes_read;

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
		reporter.log("Received command: " + std::string(buffer));

		int result = execute_command(std::string(buffer));

		std::string response = "Command executed with result: " + std::to_string(result) + "\n";
		send(client_socket, response.c_str(), response.size(), 0);
		reporter.log("Sent command result to client.");

		send(client_socket, prompt.c_str(), prompt.size(), 0);
		reporter.log("Sent prompt to client again.");
	}
}

int Daemon::execute_command(const std::string &command) {
	char        buffer[128];
	std::string result;
	int         return_code;
	FILE       *pipe = popen(command.c_str(), "r");

	if (!pipe) {
		reporter.error("Failed to execute command: " + command);
		return -1;
	}

	while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
		result += buffer;
	}

	return_code = pclose(pipe);
	reporter.log("Command output: " + result);

	return return_code;
}