
#ifndef TINTIN_REPORTER_HPP
#define TINTIN_REPORTER_HPP
#define DEBUG 1

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

class TintinReporter {
  public:
	TintinReporter();
	~TintinReporter();
	TintinReporter(const TintinReporter &);
	TintinReporter &operator=(const TintinReporter &);

	void log(const std::string &message);
	void debug(const std::string &message);
	void error(const std::string &message);
	void info(const std::string &message);

  private:
	std::string   log_file;
	std::ofstream log_stream;
	void          open_log_file();
	void          close_log_file();
};

#endif // TINTIN_REPORTER_HPP