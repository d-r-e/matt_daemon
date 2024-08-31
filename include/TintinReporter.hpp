#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#ifndef TINTIN_REPORTER_HPP
#	define TINTIN_REPORTER_HPP

# define DEBUG 1

class TintinReporter {
  public:
	TintinReporter();
	~TintinReporter();
	TintinReporter(const TintinReporter &);
	TintinReporter &operator=(const TintinReporter &);

	void log(const std::string &message);
	void debug(const std::string &message);
	void error(const std::string &message);

  private:
	std::string   log_file;
	std::ofstream log_stream;
	void          open_log_file();
};

#endif // TINTIN_REPORTER_HPP