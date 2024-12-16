#include <fstream>
#include <string>
#include "LibertyParser.hpp"
#include "lib/json.hpp"
#include <synlib_file.h>
using json = nlohmann::json;

// C++ wrapper for Verific Liberty parser
class VerificLibertyParser : public LibertyParser
{
public:
	VerificLibertyParser(std::string filename);
	~VerificLibertyParser();
	int check();
	std::string get_error_text();

	json as_json();
	void to_json_file(std::string filename, bool indent)
	{
		std::ofstream file(filename);
		if (indent)
			file << as_json().dump(2);
		else
			file << as_json().dump();
		file.close();
	}

private:
	Verific::synlib_file reader ;
	bool status = false;
};
