#include <string>
#include <iostream>

#ifndef DAEMON_HPP
#define DAEMON_HPP
class Daemon {
public:
    Daemon();
    ~Daemon();
    Daemon(const Daemon&);
    Daemon& operator=(const Daemon&);

    unsigned int getPid();

private:
    unsigned int pid;

};

#endif // DAEMON_HPP