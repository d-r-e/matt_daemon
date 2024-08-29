#include <iostream>
#include <string>
#include "Daemon.hpp"

int main() {
    Daemon d;
    std::cout << "Daemon PID: " << d.getPid() << std::endl;
    return 0;
}