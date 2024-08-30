#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>

#ifndef TINTIN_REPORTER_HPP
#define TINTIN_REPORTER_HPP

class TintinReporter{
public:
    TintinReporter();
    ~TintinReporter();
    TintinReporter(const TintinReporter&);
    TintinReporter& operator=(const TintinReporter&);

    void log(const std::string &message);
    void log(const std::string &message, const std::string &file);

private:
    std::string log_file;
    std::ofstream log_stream;
    void open_log_file();
};

#endif // TINTIN_REPORTER_HPP