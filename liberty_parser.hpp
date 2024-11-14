#include <fstream>
#include <string>
#include "include/si2dr_liberty.h"
#include "lib/json.hpp"
using json = nlohmann::json;

// C++ wrapper for Synopsys Liberty parser
class LibertyParser {
	public:
		LibertyParser(std::string filename) {
			si2drPIInit(&err); 
			si2drReadLibertyFile(strdup(filename.c_str()), &err);
		}
		~LibertyParser() { si2drPIQuit(&err); }
		int check() {
			si2drGroupsIdT groups = si2drPIGetGroups(&err);
			si2drGroupIdT group;
			while (!si2drObjectIsNull((group=si2drIterNextGroup(groups, &err)), &err)) {
				si2drCheckLibertyLibrary(group, &err);
				if (err != SI2DR_NO_ERROR) return err;
			}
			si2drIterQuit(groups, &err);
			return err;
		}
		std::string get_error_text() {
			return si2drPIGetErrorText(err, &err);
		}
		json as_json() {
			si2drGroupsIdT groups = si2drPIGetGroups(&err);
			si2drGroupIdT group = si2drIterNextGroup(groups, &err);
			json result = _group2json(group);
			si2drIterQuit(groups, &err);
			return result;
		}
		void to_json_file(std::string filename) {
			std::ofstream file(filename);
			file << as_json().dump();
			file.close();
		}
	private:
		si2drErrorT err;
		json _group2json(si2drGroupIdT group) {
			json j;
			// Group names
			si2drNamesIdT gnames = si2drGroupGetNames(group, &err);
			si2drStringT gname;
			while (gname=si2drIterNextName(gnames, &err)) {
				j["names"].push_back(gname);
			}
			si2drIterQuit(gnames, &err);
			// Group attributes
			si2drAttrsIdT attrs = si2drGroupGetAttrs(group, &err);
			si2drAttrIdT attr;
			while (!si2drObjectIsNull((attr=si2drIterNextAttr(attrs, &err)), &err)) {
				if (si2drAttrGetAttrType(attr, &err) == SI2DR_SIMPLE) {
					j[si2drAttrGetName(attr, &err)] = _simpleattr2json(attr);
				} else {
					j[si2drAttrGetName(attr, &err)] = _complexattr2json(attr);
				}
			}
			si2drIterQuit(attrs, &err);
			// Group defines
			si2drDefinesIdT defines = si2drGroupGetDefines(group, &err);
			si2drDefineIdT define;
			while (!si2drObjectIsNull((define=si2drIterNextDefine(defines, &err)), &err)) {
				j["defines"].push_back(si2drDefineGetName(define, &err));
			}
			si2drIterQuit(defines, &err);
			// Group groups
			si2drGroupsIdT groups = si2drGroupGetGroups(group, &err);
			si2drGroupIdT group2;
			while (!si2drObjectIsNull((group2=si2drIterNextGroup(groups, &err)), &err)) {
				j["groups"].push_back(_group2json(group2));
			}
			si2drIterQuit(groups, &err);
			json jfinal = {
				{si2drGroupGetGroupType(group, &err), j}
			};
			return jfinal;
		}
		json _simpleattr2json(si2drAttrIdT attr) {
		json j;
		si2drValueTypeT type = si2drSimpleAttrGetValueType(attr, &err);
		switch (type) {
			case SI2DR_INT32:
				j = si2drSimpleAttrGetInt32Value(attr, &err);
				break;
			case SI2DR_FLOAT64:
				j = si2drSimpleAttrGetFloat64Value(attr, &err);
				break;
			case SI2DR_STRING:
				j = si2drSimpleAttrGetStringValue(attr, &err);
				break;
			case SI2DR_BOOLEAN:
				j = si2drSimpleAttrGetBooleanValue(attr, &err);
				break;
			default:
				j = "Unsupported type";
		}
		return j;
	}
		json _complexattr2json(si2drAttrIdT attr) {
		json j;
		si2drValuesIdT values = si2drComplexAttrGetValues(attr, &err);
		si2drValueTypeT type;
		si2drInt32T intgr;
		si2drFloat64T float64;
		si2drStringT string;
		si2drBooleanT boolval;
		si2drExprT *expr;
		while (true) {
			si2drIterNextComplexValue(values, &type, &intgr, &float64, &string, &boolval, &expr, &err);
			switch (type) {
				case SI2DR_INT32:
					j.push_back(intgr);
					break;
				case SI2DR_FLOAT64:
					j.push_back(float64);
					break;
				case SI2DR_STRING:
					j.push_back(string);
					break;
				case SI2DR_BOOLEAN:
					j.push_back(boolval);
					break;
				case SI2DR_EXPR:
					j.push_back("Expression");
					break;
				case SI2DR_UNDEFINED_VALUETYPE:
					si2drIterQuit(values, &err);
					return j;
				default:
					throw std::invalid_argument("invalid complex value type");
			}
		}
	}
};
