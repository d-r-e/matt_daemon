
#include "Daemon.hpp"

Daemon::Daemon() {
    pid = 0;
}

Daemon::~Daemon() {}	

Daemon::Daemon(const Daemon& d) {
    pid = d.pid;
}

Daemon& Daemon::operator=(const Daemon& d) {
    pid = d.pid;
    return *this;
}

unsigned int Daemon::getPid() {
    return pid;
}