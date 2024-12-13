#include <fstream>
#include <string>
#include "lib/json.hpp"
using json = nlohmann::json;

#ifndef LIBERTY_PARSER
#define LIBERTY_PARSER

// Virtual Liberty Parser 
class LibertyParser {
	public:
		virtual ~LibertyParser() {}; 
		virtual int check() = 0;
		virtual std::string get_error_text() = 0;
		virtual json as_json() = 0;
		virtual void to_json_file(std::string filename) = 0;
	private:
};

#endif