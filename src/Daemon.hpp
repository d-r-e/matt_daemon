#ifndef DAEMON_HPP
#define DAEMON_HPP

# include <string>
# include <iostream>
# include <unistd.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <cstring>
# include <cstdlib>
# include <cerrno>
# include <filesystem>
# include <fstream>
# include <sys/stat.h>
# include "TintinReporter.hpp"

class Daemon
{
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