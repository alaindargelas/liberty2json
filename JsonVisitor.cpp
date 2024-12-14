/*
 *
 * [ File Version : 1.9 - 2024/01/02 ]
 *
 * (c) Copyright 1999 - 2024 Verific Design Automation Inc.
 * All rights reserved.
 *
 * This source code belongs to Verific Design Automation Inc.
 * It is considered trade secret and confidential, and is not to be used
 * by parties who have not received written authorization
 * from Verific Design Automation Inc.
 *
 * Only authorized users are allowed to use, copy and modify
 * this software provided that the above copyright notice
 * remains in all copies of this software.
 *
 *
*/
#include <iostream>

#include "JsonVisitor.hpp"

#include "SynlibTreeNode.h"
#include "SynlibStatement.h"
#include "SynlibGroup.h"
#include "SynlibExpr.h"

#include "Strings.h"
#include "Array.h"

#ifdef VERIFIC_NAMESPACE
using namespace Verific ;
#endif

/* -------------------------------------------------------------- */
// SynlibJsonVisitor
/* -------------------------------------------------------------- */

static std::string TAB = "  ";

void SynlibJsonVisitor::SYNLIB_VISIT(SynlibAttr, node)
{
    //std::cout << "Visit: SynlibAttr" << std::endl;
    if (node.GetValue())
        TraverseNode(node.GetValue());
    nlohmann::json simple_attr;
    if (_jstack.size()) {
        nlohmann::json lookahead = _jstack.top();
        nlohmann::json v = lookahead;
        for (const auto& [keyo, valueo] : v.items()) {
            simple_attr[node.GetName()] = valueo;
        }
        //std::cout << "ATTRIBUTE: " << simple_attr.dump() << std::endl;
        _jstack.pop();
    }
    _jstack.push(simple_attr);
    //std::cout << "\nEnd Visit: SynlibAttr" << std::endl;
}

void SynlibJsonVisitor::SYNLIB_VISIT(SynlibComplexAttr, node)
{
    //std::cout << "Visit: SynlibComplexAttr" << std::endl;
    unsigned i;
    SynlibExpr *expr;
    nlohmann::json complex_attr;
    FOREACH_ARRAY_ITEM(node.GetValues(), i, expr)
    {
        TraverseNode(expr);
        if (_jstack.size()) {
            complex_attr[node.GetName()].push_back(_jstack.top());
            std::cout << "COMPLEX_ATTRIBUTE: " << _jstack.top().dump() << std::endl;
            _jstack.pop();
        }
    }
    _jstack.push(complex_attr);
    //std::cout << "\nEnd Visit: SynlibComplexAttr" << std::endl;
}

void SynlibJsonVisitor::SYNLIB_VISIT(SynlibAssign, node)
{
    //std::cout << "Visit: SynlibAssign" << std::endl;
    //std::cout << node.GetName() << " = ";
    char *image = (node.GetValue()) ? node.GetValue()->Image() : 0;
    //std::cout << ((image) ? image : "") << " ; " << std::endl;
    Strings::free(image);
    //std::cout << "\nEnd Visit: SynlibAssign" << std::endl;
}

nlohmann::json jsonValue(SynlibExpr *expr) {
    nlohmann::json result;
    std::string true_s = "true";
    std::string false_s = "false";
    const char* s = expr->Image();
    if (true_s == std::string(s)) {
        result = 1;
    } else if (false_s == std::string(s)) {
        result = 0;
    } else if (dynamic_cast<SynlibString*>(expr)) {
        result = expr->String();
    } else {
        result = expr->Image();
    }
    return result;
}

void SynlibJsonVisitor::SYNLIB_VISIT(SynlibGroup, node)
{
    //std::cout << "Visit: SynlibGroup" << std::endl;
    unsigned i;
    //std::cout << "Visit arguments" << std::endl;
    SynlibExpr *expr;
    std::vector<nlohmann::json> names;
    FOREACH_ARRAY_ITEM(node.GetArguments(), i, expr)
    {
        char *image = (expr) ? expr->Image() : 0;
        if (image)
        {
            names.push_back(jsonValue(expr));
        }
    }
    //std::cout << "\nEnd Visit arguments" << std::endl;
    _tab++;

    //std::cout << "Visit statements" << std::endl;
    nlohmann::json defines;
    nlohmann::json groups;
    nlohmann::json attributes;

    SynlibStatement *stmt;
    FOREACH_ARRAY_ITEM(node.GetStatements(), i, stmt)
    {
        TraverseNode(stmt);
        if (_jstack.size()) {
            nlohmann::json lookahead = _jstack.top();
            if (lookahead.contains("define") || lookahead.contains("define_group")) {
                nlohmann::json v = lookahead;
                for (const auto& [keyo, valueo] : v.items()) {
                    if (valueo.size() == 2) {
                      nlohmann::json name;
                      nlohmann::json st;
                      st["allowed_group_name"] = valueo.at(1);
                      st["valtype"] = "undefined_valuetype";
                      name[valueo.at(0)] = st;
                      defines["defines"].push_back(name);
                    } else {
                        defines["defines"].push_back(valueo);
                    }
                }
            } else {
                if (dynamic_cast<SynlibGroup*>(stmt)) {
                    groups["groups"].push_back(lookahead);
                } else {
                    //std::cout << "ATTR in STMT: " << lookahead.dump() << std::endl;
                    attributes["attributes"].push_back(lookahead);
                }
            }
            _jstack.pop();
        }
    }
    //std::cout << "\nEnd Visit statements" << std::endl;
    nlohmann::json jnames;
    for (auto n : names)
    {
        jnames["names"].push_back(n);
    }
    if (node.GetName() == std::string("library")) {
        if (groups.size()) {
            _json[node.GetName()]["groups"] = groups["groups"];
        }
        if (attributes.size()) {
            nlohmann::json attr = attributes["attributes"];
            for (const auto& [key, value] : attr.items()) {
                nlohmann::json v = value;
                for (const auto& [keyo, valueo] : v.items()) {
                    // Attributes
                    _json[node.GetName()][keyo] = valueo;
                }
            }
        }
        if (defines.size())
            _json[node.GetName()]["defines"] = defines["defines"];
        if (jnames.size())
            _json[node.GetName()]["names"] = jnames["names"];    
    }
    else
    {
        nlohmann::json j;
        if (groups.size())
            j[node.GetName()]["groups"] = groups["groups"];
        if (attributes.size()) {
            nlohmann::json attr = attributes["attributes"];
            for (const auto& [key, value] : attr.items()) {
                nlohmann::json v = value;
                for (const auto& [keyo, valueo] : v.items()) {
                    // Attributes
                    j[node.GetName()][keyo] = valueo;
                    //std::cout << node.GetName() << " Val: " << v.dump() << " key: " << keyo << " v: " << valueo << std::endl;
                }
            }
        }
        if (defines.size())
            j[node.GetName()]["defines"] = defines["defines"];
        if (jnames.size())
            j[node.GetName()]["names"] = jnames["names"];    


        _jstack.push(j);
    }
    //std::cout << "\nEnd Visit: SynlibGroup" << std::endl;
}

void SynlibJsonVisitor::SYNLIB_VISIT(SynlibExpr, node)
{
    //std::cout << "Visit: SynlibExpr" << std::endl;
    char *image = node.Image();
    if (!image)
        return;
    nlohmann::json simple_expr;
    //std::cout << "Image: " << image << std::endl;
    simple_expr = jsonValue(&node);
    _jstack.push(simple_expr);
    Strings::free(image);
    //std::cout << "\nEnd Visit: SynlibExpr" << std::endl;
}

void SynlibJsonVisitor::SYNLIB_VISIT(SynlibReal, node)
{
    //std::cout << "Visit: SynlibReal" << std::endl;
    char *image = node.Image();
    if (!image)
        return;
    nlohmann::json simple_expr;
    simple_expr = node.Double();
    _jstack.push(simple_expr);
    Strings::free(image);
    //std::cout << "\nEnd Visit: SynlibReal" << std::endl;
}

void SynlibJsonVisitor::SYNLIB_VISIT(SynlibInt, node)
{
    //std::cout << "Visit: SynlibInt" << std::endl;
    char *image = node.Image();
    if (!image)
        return;
    nlohmann::json simple_expr;
    simple_expr = node.Int();
    _jstack.push(simple_expr);
    Strings::free(image);
    //std::cout << "\nEnd Visit: SynlibInt" << std::endl;
}


void SynlibJsonVisitor::SYNLIB_VISIT(SynlibString, node)
{
    //std::cout << "Visit: SynlibString" << std::endl;
    char *image = node.Image();
    if (!image)
        return;
    //std::cout << "Image: " << image << std::endl;
    nlohmann::json simple_expr;
    simple_expr = jsonValue(&node);
    _jstack.push(simple_expr);
    Strings::free(image);
    //std::cout << "\nEnd Visit: SynlibString" << std::endl;
}

/* -------------------------------------------------------------- */

// EOF
