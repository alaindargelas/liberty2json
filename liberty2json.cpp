#include <iostream>
#include "liberty_parser.hpp"

// Main method
int main(int argc, char *argv[]) {
	LibertyParser *parser;
	switch (argc) {
		case 2:
			parser = new LibertyParser(argv[1]);
			std::cout << parser->as_json().dump(2) << std::endl;
			break;
		case 3:
			parser = new LibertyParser(argv[1]);
			parser->to_json_file(argv[2]);
			break;
		default:
			std::cerr << "Usage: " << argv[0] << " <liberty_file> [output_file]" << std::endl;
			return 1;
	}
}
