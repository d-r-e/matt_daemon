#include "TintinReporter.hpp"
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

TintinReporter::TintinReporter() {
	log_file = "/var/log/matt_daemon.log";
	open_log_file();
}

TintinReporter::~TintinReporter() {
	close_log_file();
	log_stream.clear();
}

TintinReporter::TintinReporter(const TintinReporter &other)
    : log_file(other.log_file) {
	open_log_file();
}

TintinReporter &TintinReporter::operator=(const TintinReporter &other) {
	if (this != &other) {
		if (log_stream.is_open()) {
			log_stream.close();
		}
		log_file = other.log_file;
		open_log_file();
	}
	return *this;
}

void TintinReporter::open_log_file() {
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

void TintinReporter::close_log_file() {
	if (log_stream.is_open()) {
		log_stream.flush();
		log_stream.close();
	}
}

void TintinReporter::log(const std::string &message) {
	if (log_stream.is_open()) {
		std::time_t t = std::time(0);
		std::tm    *localtime = std::localtime(&t);
		log_stream << std::put_time(localtime, "[%d/%m/%Y - %H:%M:%S]  [LOG] ") << message << std::endl;
	}
}

void TintinReporter::debug(const std::string &message) {
	if (log_stream.is_open() && DEBUG) {
		std::time_t t = std::time(0);
		std::tm    *localtime = std::localtime(&t);
		log_stream << std::put_time(localtime, "[%d/%m/%Y - %H:%M:%S][DEBUG] ") << message << std::endl;
	}
}

void TintinReporter::error(const std::string &message) {
	if (log_stream.is_open()) {
		std::time_t t = std::time(0);
		std::tm    *localtime = std::localtime(&t);
		log_stream << std::put_time(localtime, "[%d/%m/%Y - %H:%M:%S][ERROR] ") << message << std::endl;
	}
}

void TintinReporter::info(const std::string &message) {
	if (log_stream.is_open()) {
		std::time_t t = std::time(0);
		std::tm    *localtime = std::localtime(&t);
		log_stream << std::put_time(localtime, "[%d/%m/%Y - %H:%M:%S][INFO] ") << message << std::endl;
	}
}
