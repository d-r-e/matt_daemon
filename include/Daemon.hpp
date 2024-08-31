#ifndef DAEMON_HPP
#define DAEMON_HPP

#include "TintinReporter.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

class Daemon {
  public:
	Daemon();
	~Daemon();
	Daemon(const Daemon &);
	Daemon &operator=(const Daemon &);

	unsigned int getPid();

  private:
	unsigned int pid;
	TintinReporter reporter;
	bool check_requirements();
	bool daemonize(void);
};

#endif // DAEMON_HPP