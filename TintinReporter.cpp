#include "TintinReporter.hpp"

TintinReporter::TintinReporter() : log_file("/var/log/matt_daemon/matt_daemon.log")
{
    std::filesystem::create_directories("/var/log/matt_daemon");
    open_log_file();
}

TintinReporter::~TintinReporter()
{
    if (log_stream.is_open())
    {
        log_stream.close();
    }
}

TintinReporter::TintinReporter(const TintinReporter &other) : log_file(other.log_file)
{
    open_log_file();
}

TintinReporter &TintinReporter::operator=(const TintinReporter &other)
{
    if (this != &other)
    {
        if (log_stream.is_open())
        {
            log_stream.close();
        }
        log_file = other.log_file;
        open_log_file();
    }
    return *this;
}

void TintinReporter::open_log_file()
{
    log_stream.open(log_file, std::ios::app);
    if (!log_stream.is_open())
    {
        std::cerr << "Error: Could not open log file " << log_file << std::endl;
        exit(1);
    }
}

void TintinReporter::log(const std::string &message)
{
    if (log_stream.is_open())
    {
        log_stream << message << std::endl;
    }
}

void TintinReporter::log(const std::string &message, const std::string &file)
{
    if (log_stream.is_open())
    {
        log_stream << file << ": " << message << std::endl;
    }
}

