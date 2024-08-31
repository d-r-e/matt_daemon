#include "main.hpp"

int main() {
	try {

		Daemon d;
		d.start_remote_shell();

	} catch (std::filesystem::filesystem_error &e) {
		std::cerr << "Filesystem Error: " << e.what() << " - Error code: " << e.code() << std::endl;
		return 1;
	} catch (std::exception &e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}