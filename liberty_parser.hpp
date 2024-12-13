#include <fstream>
#include <string>
#include "include/si2dr_liberty.h"
#include "lib/json.hpp"
#include "LibertyParser.hpp"
using string = std::string;
using json = nlohmann::json;

// C++ wrapper for Synopsys Liberty parser
class SynLibertyParser : public LibertyParser {
	public:
		SynLibertyParser(string filename, bool debug=false) {
			set_debug_mode(debug);
			si2drPIInit(&err); 
			si2drReadLibertyFile(strdup(filename.c_str()), &err);
			if (err == SI2DR_SYNTAX_ERROR) {
				throw std::invalid_argument(get_error_text());
			}
		}
		~SynLibertyParser() { si2drPIQuit(&err); }
		bool get_debug_mode() {
			return si2drPIGetTraceMode(&err);
		}
		void set_debug_mode(bool enabled) {
			if (enabled) {
				si2drPISetDebugMode(&err);
			} else {
				si2drPIUnSetDebugMode(&err);
			}
		}
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
		string get_error_text() {
			return si2drPIGetErrorText(err, &err);
		}
		json as_json() {
			si2drGroupsIdT groups = si2drPIGetGroups(&err);
			si2drGroupIdT group = si2drIterNextGroup(groups, &err);
			json result = _group2json(group);
			si2drIterQuit(groups, &err);
			return result;
		}
		void to_json_file(string filename) {
			std::ofstream file(filename);
			file << as_json().dump();
			file.close();
		}
	
		static bool get_ignore_complex_attrs() {
			return si2drPIGetIgnoreComplexAttrs();
		}
		static void set_ignore_complex_attrs(bool enabled) {
			if (enabled) {
				si2drPISetIgnoreComplexAttrs();
			} else {
				si2drPIUnSetIgnoreComplexAttrs();
			}
		}
	private:
		si2drErrorT err;

		json _group2json(si2drGroupIdT group) {
			json j;
			// Group names
			si2drNamesIdT gnames = si2drGroupGetNames(group, &err);
			si2drStringT gname;
			while ((gname=si2drIterNextName(gnames, &err))) {
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
				si2drStringT name, allowed_group_name;
				si2drValueTypeT valtype;
				si2drDefineGetInfo(define, &name, &allowed_group_name, &valtype, &err);
				j["defines"][name]["allowed_group_name"] = allowed_group_name;
				j["defines"][name]["valtype"] = _vt2str(valtype);
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
				case SI2DR_EXPR:
					j = si2drExprToString(si2drSimpleAttrGetExprValue(attr, &err), &err);
					break;
				case SI2DR_MAX_VALUETYPE:
				case SI2DR_UNDEFINED_VALUETYPE:
				default:
					throw std::invalid_argument("Invalid simple attr value type");
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
						j.push_back(si2drExprToString(expr, &err));
						break;
					case SI2DR_MAX_VALUETYPE:
					case SI2DR_UNDEFINED_VALUETYPE:
						si2drIterQuit(values, &err);
						return j;
					default:
						throw std::invalid_argument("Invalid complex attr value type");
				}
			}
		}
		string _vt2str(si2drValueTypeT vt) {
			switch (vt) {
				case SI2DR_INT32:
					return "int32";
				case SI2DR_FLOAT64:
					return "float64";
				case SI2DR_STRING:
					return "string";
				case SI2DR_BOOLEAN:
					return "boolean";
				case SI2DR_EXPR:
					return "expr";
				case SI2DR_MAX_VALUETYPE:
					return "max_valuetype";
				case SI2DR_UNDEFINED_VALUETYPE:
					return "undefined_valuetype";
				default:
					return "unknown_valuetype";
			}
		}
};
