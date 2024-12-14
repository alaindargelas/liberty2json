#include <iostream>

#include "VerificLibertyParser.hpp"
#include <synlib_file.h>
#include "SynlibGroup.h" // Make SynlibLibrary available
#include "JsonVisitor.hpp"

#include "Array.h"	 // Make Array available
#include "Map.h"		 // Make Map available
#include "Strings.h" // Make String utility available

#include "Message.h"		// Make message handlers available
#include "FileSystem.h" // Make FileSystem utility available

using namespace Verific;

using json = nlohmann::json;

VerificLibertyParser::VerificLibertyParser(std::string filename)
{
	status = reader.Analyze(filename.c_str());
	// reader.PrettyPrint(std::string(filename + ".debug").c_str());
}

VerificLibertyParser::~VerificLibertyParser()
{
	reader.Reset();
}

int VerificLibertyParser::check()
{
	return status;
}

std::string VerificLibertyParser::get_error_text()
{
	std::string message;
	return message;
}

json VerificLibertyParser::as_json()
{
	json obj;
	SynlibLibrary *lib = 0;
	SynlibJsonVisitor v(obj);
	MapIter mi;
	FOREACH_MAP_ITEM(Verific::synlib_file::GetLibraries(), mi, 0, &lib)
	{
		if (lib)
			lib->Accept(v);
	}
	return obj;
}
