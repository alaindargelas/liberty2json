#include <iostream>
#include "SynLibertyParser.hpp"
#include "VerificLibertyParser.hpp"
#include "backward.hpp"

// Main method
int main(int argc, char *argv[]) {
	LibertyParser *parser = nullptr;
	backward::SignalHandling sh;
	typedef enum ParserType {
    VerificParser,
		SynopsysParser
  } ParserType ; 
	ParserType parserType = VerificParser;
	if (argc == 2 || argc == 3 || argc == 4) {
		parser = (parserType == SynopsysParser) ? dynamic_cast<LibertyParser *>(new SynLibertyParser(argv[1])) 
																						: dynamic_cast<LibertyParser *>(new VerificLibertyParser(argv[1]));
	}
	switch (argc) {
		case 2: // dump to stdout
			try
			{
				std::cout << parser->as_json().dump(2) << std::endl;
			}
			catch (std::exception &e)
			{
				std::cerr << "Error: " << e.what() << std::endl;
				return 1;
			}
			break;
		case 3: // dump to file
			try {
				parser->to_json_file(argv[2]);
			} catch (std::exception &e) {
				std::cerr << "Error: " << e.what() << std::endl;
				return 1;
			}
			break;
		case 4: // dump to file with error check
			try {
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
