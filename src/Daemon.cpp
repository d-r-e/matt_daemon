
#include "Daemon.hpp"

Daemon               *Daemon::instance = nullptr;
volatile sig_atomic_t Daemon::stop_requested = 0;

Daemon::Daemon() {
	instance = this;
	pid = 0;
	if (!this->check_requirements()) {
		this->stop_requested = 1;
		exit(EXIT_FAILURE);
	}
	if (!this->daemonize()) {
		std::cerr << "Error: Failed to daemonize." << std::endl;
		this->stop_requested = 1;
		exit(EXIT_FAILURE);
	}
	signal(SIGTERM, Daemon::handle_signal);
	signal(SIGINT, Daemon::handle_signal);
	signal(SIGHUP, Daemon::handle_signal);
}

Daemon::Daemon(bool daemonize) {
	instance = this;
	pid = 0;
	if (!this->check_requirements()) {
		this->stop_requested = 1;

		exit(EXIT_FAILURE);
	}
	if (daemonize) {
		if (!this->daemonize()) {
			std::cerr << "Error: Failed to daemonize." << std::endl;
			this->stop_requested = 1;
			exit(EXIT_FAILURE);
		}
	}
	signal(SIGTERM, Daemon::handle_signal);
	signal(SIGINT, Daemon::handle_signal);
	signal(SIGHUP, Daemon::handle_signal);
}

Daemon::~Daemon() {
	this->reporter.info("Daemon stopped.");
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

unsigned int Daemon::get_pid() {
	return pid;
}

bool Daemon::check_requirements() const {
	if (getuid() != 0 && geteuid() != 0) {
		std::cerr << "You must be root to run this program." << std::endl;
		return false;
	}
	std::ifstream lock_file("/var/run/matt_daemon.lock");
	if (std::filesystem::exists("/var/run/matt_daemon.lock")) {
		std::string message = "Daemon is already running with PID: ";
		std::string pid_str;
		std::getline(lock_file, pid_str);
		message += pid_str;
		std::cerr << message << std::endl;
		lock_file.close();
		return false;
	}
	return true;
}

void Daemon::handle_signal(int signal) {
	TintinReporter reporter;

	if (signal == SIGTERM || signal == SIGINT) {
		Daemon::instance->close_clients();
		Daemon::instance->close_sockets();
		std::filesystem::remove("/var/run/matt_daemon.lock");
		if (signal == SIGTERM)
			reporter.info("[SIGTERM] Daemon stopped.");
		else
			reporter.info("[SIGINT] Daemon stopped.");
		stop_requested = 1;
	} else if (signal == SIGHUP) {
		Daemon::instance->close_clients();
		reporter.info("[SIGHUP] Daemon reloaded.");
	} else {
		reporter.info("Unknown signal received.");
	}
}

unsigned int Daemon::get_client_count() const {
	unsigned int count = 0;
	for (int i = 0; i < MAX_CLIENTS + 1; ++i) {
		if (client_fds[i] > 0) {
			count++;
		}
	}
	return count;
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

	if (!check_requirements()) {
		return false;
	}

	pid = fork();
	if (pid < 0) {
		reporter.error("Fork failed: " + std::string(strerror(errno)));
		return false;
	}
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	umask(027);

	sid = setsid();
	if (sid < 0) {
		reporter.error("Failed to create a new session: " + std::string(strerror(errno)));
		return false;
	}

	pid = fork();
	if (pid < 0) {
		reporter.error("Fork failed: " + std::string(strerror(errno)));
		return false;
	}
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	if (chdir("/") < 0) {
		reporter.error("Failed to change directory to /: " + std::string(strerror(errno)));
		return false;
	}

	if (close(STDIN_FILENO) < 0 || close(STDOUT_FILENO) < 0 || close(STDERR_FILENO) < 0) {
		reporter.error("Failed to close standard file descriptors: " + std::string(strerror(errno)));
		return false;
	}

	if (open("/dev/null", O_RDONLY) < 0 || open("/dev/null", O_WRONLY) < 0 || open("/dev/null", O_WRONLY) < 0) {
		reporter.error("Failed to redirect standard file descriptors to /dev/null: " + std::string(strerror(errno)));
		return false;
	}

	std::ofstream lock_file("/var/run/matt_daemon.lock");
	if (!lock_file.is_open()) {
		reporter.error("Failed to create lock file: " + std::string(strerror(errno)));
		return false;
	}

	lock_file << getpid();
	lock_file.close();

	signal(SIGTERM, handle_signal);
	signal(SIGINT, handle_signal);
	signal(SIGHUP, handle_signal);

	return true;
}

int Daemon::start_remote_shell() {
	struct sockaddr_in address;
	int                addrlen = sizeof(address);
	int                max_sd;

	if (this->stop_requested) {
		return -1;
	}
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
	bzero(this->client_fds, sizeof(this->client_fds));
	FD_ZERO(&readfds);
	FD_SET(server_fd, &readfds);

	this->reporter.info("Daemon listening on port " + std::to_string(PORT));
	while (!this->stop_requested) {
		FD_ZERO(&readfds);
		FD_SET(server_fd, &readfds);
		max_sd = server_fd;

		for (int i = 0; i < MAX_CLIENTS; ++i) {
			int sd = client_fds[i];
			if (sd > 0)
				FD_SET(sd, &readfds);
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
			reporter.info("New connection, socket fd: " + std::to_string(new_socket) +
			              ", ip: " + inet_ntoa(address.sin_addr) +
			              ", port: " + std::to_string(ntohs(address.sin_port)));
			for (int i = 0; i < MAX_CLIENTS + 1; ++i) {
				if (client_fds[i] == 0) {
					client_fds[i] = new_socket;
					break;
				}
			}
			if (get_client_count() > MAX_CLIENTS) {
				std::string msg = "[ERROR] Max clients reached, connection closed.\r\n";
				send(new_socket, msg.c_str(), msg.size(), 0);
				reporter.error("[" + std::to_string(new_socket) + "] Max clients reached, connection closed.");
				for (int i = 0; i < MAX_CLIENTS + 1; ++i) {
					if (client_fds[i] == new_socket) {
						close(client_fds[i]);
						client_fds[i] = 0;
						break;
					}
				}
			} else
				reporter.info("Client " + std::to_string(get_client_count()) + " connected.");
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
	char        buffer[1024 * 10];
	int         bytes_read;
	std::string cmd;
	std::string prompt = "\r$ ";


	bzero(buffer, sizeof(buffer));
	bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);

	if (bytes_read <= 0) {
		if (bytes_read == 0) {
			reporter.info("Client disconnected.");
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
		cmd.erase(std::remove(cmd.begin(), cmd.end(), '\n'), cmd.end());
		cmd.erase(std::remove(cmd.begin(), cmd.end(), '\r'), cmd.end());
		if (Daemon::tolower(cmd) == "quit") {
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
		reporter.log("[" + std::to_string(client_socket) + "] " + cmd);
		// execute_command(cmd, client_socket);
		// send(client_socket, prompt.c_str(), prompt.size(), 0);
		// std::thread command_thread(&Daemon::execute_command, this, cmd, client_socket);
		// command_thread.detach();
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
	FILE                 *pipe;

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
	pipe = popen(sanitized_command.c_str(), "r");
	if (!pipe) {
		reporter.error("Failed to open pipe for command execution.");
		return -1;
	}
	while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
		result += buffer.data();
	}
	int exit_code = pclose(pipe);
	exit_code = WEXITSTATUS(exit_code);
	reporter.debug("[" + std::to_string(client_socket) + "] Command executed successfully: " + result);
	if (send(client_socket, result.c_str(), result.size(), 0) < 0) {
		reporter.error("Failed to send command result to client.");
	}

	if (exit_code != 0) {
		reporter.error("Command execution failed with exit code " + std::to_string(exit_code) + ": " + result);
		return exit_code;
	}
	return exit_code;
}

int Daemon::execute_command_with_tty(const std::string &command, int client_socket) {
	int master_fd = posix_openpt(O_RDWR | O_NOCTTY);
	if (master_fd == -1) {
		reporter.error("Failed to open PTY master: " + std::string(strerror(errno)));
		return -1;
	}

	if (grantpt(master_fd) == -1 || unlockpt(master_fd) == -1) {
		reporter.error("Failed to grant or unlock PTY: " + std::string(strerror(errno)));
		close(master_fd);
		return -1;
	}

	char *slave_name = ptsname(master_fd);
	if (!slave_name) {
		reporter.error("Failed to get PTY slave name: " + std::string(strerror(errno)));
		close(master_fd);
		return -1;
	}

	pid_t pid = fork();
	if (pid == -1) {
		reporter.error("Failed to fork process: " + std::string(strerror(errno)));
		close(master_fd);
		return -1;
	} else if (pid == 0) {
		// Child process
		close(master_fd); // Close master in the child process

		int slave_fd = open(slave_name, O_RDWR);
		if (slave_fd == -1) {
			reporter.error("Failed to open PTY slave: " + std::string(strerror(errno)));
			exit(EXIT_FAILURE);
		}

		// Create a new session and set the controlling terminal
		if (setsid() == -1) {
			reporter.error("Failed to create new session: " + std::string(strerror(errno)));
			exit(EXIT_FAILURE);
		}

		if (ioctl(slave_fd, TIOCSCTTY, 0) == -1) {
			reporter.error("Failed to set controlling terminal: " + std::string(strerror(errno)));
			exit(EXIT_FAILURE);
		}

		// Redirect stdin, stdout, and stderr to the PTY slave
		dup2(slave_fd, STDIN_FILENO);
		dup2(slave_fd, STDOUT_FILENO);
		dup2(slave_fd, STDERR_FILENO);
		close(slave_fd);

		// Execute the command
		std::vector<char *> args;
		args.push_back((char *)"/bin/sh");
		args.push_back((char *)"-c");
		args.push_back((char *)command.c_str());
		args.push_back(nullptr);

		execvp("/bin/sh", args.data());
		// If execvp returns, it failed
		reporter.error("Failed to execute command: " + std::string(strerror(errno)));
		exit(EXIT_FAILURE);
	} else {
		// Parent process
		close(master_fd); // You might want to keep master_fd open to interact with the child process

		char        buffer[128];
		std::string result;
		ssize_t     bytes_read;

		while ((bytes_read = read(master_fd, buffer, sizeof(buffer))) > 0) {
			result.append(buffer, bytes_read);
			if (send(client_socket, buffer, bytes_read, 0) < 0) {
				reporter.error("Failed to send command result to client.");
				break;
			}
		}

		int status;
		waitpid(pid, &status, 0);
		int exit_code = WEXITSTATUS(status);

		if (exit_code != 0) {
			reporter.error("Command execution failed with exit code " + std::to_string(exit_code) + ": " + result);
		} else {
			reporter.debug("[" + std::to_string(client_socket) + "] Command executed successfully: " + result);
		}

		return exit_code;
	}

	return 0;
}

void Daemon::close_clients() {
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		if (client_fds[i] > 0) {
			std::string msg = "Server shutting down.\n";
			send(client_fds[i], msg.c_str(), msg.size(), 0);
			close(client_fds[i]);
			client_fds[i] = 0;
		}
	}
}