#ifndef DAEMON_HPP
# define DAEMON_HPP

# include <string>
# include <iostream>
# include "TintinReporter.hpp"

class Daemon {
public:
    Daemon();
    ~Daemon();
    Daemon(const Daemon&);
    Daemon& operator=(const Daemon&);

    unsigned int getPid();

private:
    unsigned int pid;
    TintinReporter reporter;

};

#endif // DAEMON_HPP