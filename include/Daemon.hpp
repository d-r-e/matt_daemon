#ifndef DAEMON_HPP
#define DAEMON_HPP

#include "Tintin_reporter.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <array>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#define MAX_CLIENTS 3
#define PORT 4242
#define UNSAFE 1
#define QUIT_REQUESTED 1337

class Daemon {
  public:
	Daemon();
	Daemon(bool daemonize);
	~Daemon();
	Daemon(const Daemon &);
	Daemon &operator=(const Daemon &);

	unsigned int get_pid();
	int          start_remote_shell();

  private:
	unsigned int                 pid;
	Tintin_reporter               reporter;
	int                          server_fd;
	int                          lock_fd;
	int                          client_fds[MAX_CLIENTS + 1];
	fd_set                       readfds;
	static Daemon               *instance;
	static volatile sig_atomic_t stop_requested;

	bool         check_requirements();
	bool         daemonize(void);
	unsigned int get_client_count() const;
	void         handle_client(int client_socket);
	int          execute_command(const std::string &command, int client_socket);
	void         close_sockets();
	void         close_clients();

	static void        handle_signal(int signal);
	static std::string tolower(std::string str);
};

#endif // DAEMON_HPP