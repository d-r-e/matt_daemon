#include "main.hpp"

int is_port_in_use(int port) {
	struct sockaddr_in sa;
	int                sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		std::cerr << "Failed to create socket." << std::endl;
		return 1;
	}
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = INADDR_ANY;
	sa.sin_port = htons(port);
	if (bind(sock, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
		close(sock);
		return 1;
	}
	close(sock);
	return 0;
}

int validate_requirements() {
	if (getuid() != 0 && geteuid() != 0) {
		std::cerr << "You must be root to run this program." << std::endl;
		return 1;
	}
	if (is_port_in_use(4242)) {
		std::cerr << "Port 4242 is already in use." << std::endl;
		return 1;
	}
	return 0;
}

int main() {
	try {

		if (validate_requirements() != 0)
			return 1;
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