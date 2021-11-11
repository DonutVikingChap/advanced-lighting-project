#include "application/application.hpp"

#include <cstddef>      // std::size_t
#include <cstdlib>      // EXIT_SUCCESS, EXIT_FAILURE
#include <fmt/format.h> // fmt::print
#include <span>         // std::span
#include <stdexcept>    // std::exception

auto main(int argc, char* argv[]) -> int {
	try {
		application{std::span{argv, static_cast<std::size_t>(argc)}}.run();
	} catch (const std::exception& e) {
		fmt::print(stderr, "Fatal error: {}\n", e.what());
		return EXIT_FAILURE;
	} catch (...) {
		fmt::print(stderr, "Fatal error!\n");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
