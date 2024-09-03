#include "Tintin_reporter.hpp"
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

Tintin_reporter::Tintin_reporter() {
	log_file = "/var/log/matt_daemon/matt_daemon.log";
	open_log_file();
}

Tintin_reporter::~Tintin_reporter() {
	close_log_file();
	log_stream.clear();
}

Tintin_reporter::Tintin_reporter(const Tintin_reporter &other)
    : log_file(other.log_file) {
	open_log_file();
}

Tintin_reporter &Tintin_reporter::operator=(const Tintin_reporter &other) {
	if (this != &other) {
		if (log_stream.is_open()) {
			log_stream.close();
		}
		log_file = other.log_file;
		open_log_file();
	}
	return *this;
}

void Tintin_reporter::open_log_file() {
	std::filesystem::path log_path = log_file;
	std::filesystem::path log_dir = log_path.parent_path();

	if (!std::filesystem::exists(log_dir))
		std::filesystem::create_directories(log_dir);
	log_stream.open(log_file, std::ios::app);
	if (!log_stream.is_open()) {
		std::cerr << "Error: Could not open log file " << log_file << std::endl;
		exit(1);
	}
}

void Tintin_reporter::close_log_file() {
	if (log_stream.is_open()) {
		log_stream.flush();
		log_stream.close();
	}
}

void Tintin_reporter::log(const std::string &message) {
	if (log_stream.is_open()) {
		std::time_t t = std::time(0);
		std::tm    *localtime = std::localtime(&t);
		log_stream << std::put_time(localtime, "[%d/%m/%Y - %H:%M:%S]   LOG: ") << message << std::endl;
	}
}

void Tintin_reporter::debug(const std::string &message) {
	if (log_stream.is_open() && DEBUG) {
		std::time_t t = std::time(0);
		std::tm    *localtime = std::localtime(&t);
		log_stream << std::put_time(localtime, "[%d/%m/%Y - %H:%M:%S] DEBUG: ") << message << std::endl;
	}
}

void Tintin_reporter::error(const std::string &message) {
	if (log_stream.is_open()) {
		std::time_t t = std::time(0);
		std::tm    *localtime = std::localtime(&t);
		log_stream << std::put_time(localtime, "[%d/%m/%Y - %H:%M:%S] ERROR: ") << message << std::endl;
	}
}

void Tintin_reporter::info(const std::string &message) {
	if (log_stream.is_open()) {
		std::time_t t = std::time(0);
		std::tm    *localtime = std::localtime(&t);
		log_stream << std::put_time(localtime, "[%d/%m/%Y - %H:%M:%S]  INFO: ") << message << std::endl;
	}
}
