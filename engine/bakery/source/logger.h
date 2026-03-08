#pragma once

#include <iostream>

namespace z1::bakery {

	void log_info(std::string const& msg) {
		std::cout << "z1::bakery INFO: " << msg << std::endl;
	}
	void log_warning(std::string const& msg) {
		std::cerr << "z1::bakery WARN: " << msg << std::endl;
	}
	void log_error(std::string const& msg) {
		std::cerr << "z1::bakery ERROR: " << msg << std::endl;
	}
	void log_fatal(std::string const& msg) {
		std::cerr << "z1::bakery FATAL: " << msg << std::endl;
		std::exit(EXIT_FAILURE);
	}

}
