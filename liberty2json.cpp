#include <iostream>
#include "liberty_parser.hpp"

// Main method
int main(int argc, char *argv[]) {
	LibertyParser *parser;
	switch (argc) {
		case 2:
			try {
				parser = new LibertyParser(argv[1]);
				parser->check();
				std::cout << parser->as_json().dump(2) << std::endl;
			} catch (std::exception &e) {
				std::cerr << "Error: " << e.what() << std::endl;
				return 1;
			}
			break;
		case 3:
		try {
			parser = new LibertyParser(argv[1]);
			parser->check();
			parser->to_json_file(argv[2]);
		} catch (std::exception &e) {
			std::cerr << "Error: " << e.what() << std::endl;
			return 1;
		}
			break;
		default:
			std::cerr << "Usage: " << argv[0] << " <liberty_file> [output_file]" << std::endl;
			return 1;
	}
}
