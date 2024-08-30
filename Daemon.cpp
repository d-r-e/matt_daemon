
#include "Daemon.hpp"

Daemon::Daemon() {
    pid = 0;
    if (!this->daemonize()) {
        std::cerr << "Error: Failed to daemonize." << std::endl;
        exit(1);
    }
    for (int i = 0; i < 15; i++) {
        this->reporter.log("Daemon is running..." + std::to_string(i));
        sleep(1);
    }
}

Daemon::~Daemon() {
    this->reporter.log("Daemon stopped.");
    std::filesystem::remove("/var/run/matt_daemon.pid");
}	

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

bool Daemon::daemonize(void) {
    std::ifstream pid_file("/var/run/matt_daemon.pid");
    if (pid_file.is_open()) {
        std::cerr << "Error: Daemon is already running." << std::endl;
        pid_file.close();
        return false;
    }
    std::ofstream pid_file_write("/var/run/matt_daemon.pid");
    if (!pid_file_write.is_open()) {
        std::cerr << "Error: Failed to create pid file." << std::endl;
        return false;
    }
    pid_file_write << getpid();
    pid_file_write.close();
    pid_t pid = fork();
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
    pid_t sid = setsid();
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
    this->reporter.log("Daemon started.");
    return true;
}