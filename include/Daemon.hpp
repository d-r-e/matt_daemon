#ifndef DAEMON_HPP
#define DAEMON_HPP

#include "TintinReporter.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#define MAX_CLIENTS 3
#define PORT 4242

class Daemon {
  public:
	Daemon();
	~Daemon();
	Daemon(const Daemon &);
	Daemon &operator=(const Daemon &);

	unsigned int getPid();
	int          start_remote_shell();

  private:
	unsigned int   pid;
	TintinReporter reporter;
	int            server_fd;
	int            client_fds[MAX_CLIENTS];
	fd_set         readfds;

	static Daemon *instance;

	bool        check_requirements();
	bool        daemonize(void);
	static void handle_signal(int signal);
	void        handle_client(int client_socket);
	int         execute_command(const std::string &command);
	void        close_sockets();
};

#endif // DAEMON_HPP