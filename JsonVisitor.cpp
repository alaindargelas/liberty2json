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
using namespace Verific;
#endif

/* -------------------------------------------------------------- */
// SynlibJsonVisitor
/* -------------------------------------------------------------- */

static std::string TAB = "  ";

void SynlibJsonVisitor::SYNLIB_VISIT(SynlibAttr, node)
{
    if (node.GetValue())
        TraverseNode(node.GetValue());
    nlohmann::json simple_attr;
    if (_jstack.size())
    {
        nlohmann::json lookahead = _jstack.top();
        nlohmann::json v = lookahead;
        for (const auto &[keyo, valueo] : v.items())
        {
            simple_attr[node.GetName()] = valueo;
        }
        _jstack.pop();
    }
    _jstack.push(simple_attr);
}

void SynlibJsonVisitor::SYNLIB_VISIT(SynlibComplexAttr, node)
{
    unsigned i;
    SynlibExpr *expr;
    nlohmann::json complex_attr;
    FOREACH_ARRAY_ITEM(node.GetValues(), i, expr)
    {
        TraverseNode(expr);
        if (_jstack.size())
        {
            complex_attr[node.GetName()].push_back(_jstack.top());
            _jstack.pop();
        }
    }
    _jstack.push(complex_attr);
}

nlohmann::json jsonValue(SynlibExpr *expr)
{
    nlohmann::json result;
    std::string true_s = "true";
    std::string false_s = "false";
    const char *s = expr->Image();
    if (true_s == std::string(s))
    {
        result = 1;
    }
    else if (false_s == std::string(s))
    {
        result = 0;
    }
    else if (dynamic_cast<SynlibString *>(expr))
    {
        result = expr->String();
    }
    else
    {
        result = expr->Image();
    }
    return result;
}

std::string valType(std::string type)
{
    if (type == "integer")
    {
        return "int32";
    }
    else if (type == "float")
    {
        return "float64";
    }
    return type;
}

void SynlibJsonVisitor::SYNLIB_VISIT(SynlibGroup, node)
{
    unsigned i;
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

    nlohmann::json defines;
    nlohmann::json groups;
    nlohmann::json attributes;

    SynlibStatement *stmt;
    // There are 3 categories of objects for a given library we care about: defines, groups and attributes
    FOREACH_ARRAY_ITEM(node.GetStatements(), i, stmt)
    {
        TraverseNode(stmt);
        if (_jstack.size())
        {
            nlohmann::json lookahead = _jstack.top();
            // Defines use one of either keywords 
            if (lookahead.contains("define") || lookahead.contains("define_group"))
            {
                nlohmann::json v = lookahead;
                for (const auto &[keyo, valueo] : v.items())
                {
                    if (valueo.size() == 2 || valueo.size() == 3)
                    {
                        nlohmann::json st;
                        if (defines["defines"].contains(valueo.at(0)))
                        {
                            std::string prev = defines["defines"][valueo.at(0)]["allowed_group_name"];
                            defines["defines"][valueo.at(0)]["allowed_group_name"] = prev + "|" + valType(valueo.at(1));
                        }
                        else
                        {
                            st["allowed_group_name"] = valueo.at(1);
                            if (valueo.size() == 2)
                                st["valtype"] = "undefined_valuetype";
                            else 
                                st["valtype"] = valType(valueo.at(2));
                            defines["defines"][valueo.at(0)] = st;
                        }
                    }
                    else
                    {
                        defines["defines"][keyo] = valueo;
                    }
                }
            }
            // Groups are one of the SynlibGroup subclass
            else if (dynamic_cast<SynlibGroup *>(stmt))
            {
                groups["groups"].push_back(lookahead);
            }
            // Remainders are attributes
            else
            {
                attributes["attributes"].push_back(lookahead);
            }
            _jstack.pop();
        }
    }

    nlohmann::json jnames;
    for (auto n : names)
    {
        jnames["names"].push_back(n);
    }
    if (node.GetName() == std::string("library"))
    {
        if (groups.size())
        {
            _json[node.GetName()]["groups"] = groups["groups"];
        }
        if (attributes.size())
        {
            nlohmann::json attr = attributes["attributes"];
            for (const auto &[key, value] : attr.items())
            {
                nlohmann::json v = value;
                for (const auto &[keyo, valueo] : v.items())
                {
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
        if (attributes.size())
        {
            nlohmann::json attr = attributes["attributes"];
            for (const auto &[key, value] : attr.items())
            {
                nlohmann::json v = value;
                for (const auto &[keyo, valueo] : v.items())
                {
                    // Attributes
                    j[node.GetName()][keyo] = valueo;
                }
            }
        }
        if (defines.size())
            j[node.GetName()]["defines"] = defines["defines"];
        if (jnames.size())
            j[node.GetName()]["names"] = jnames["names"];

        _jstack.push(j);
    }
}

void SynlibJsonVisitor::SYNLIB_VISIT(SynlibExpr, node)
{
    char *image = node.Image();
    if (!image)
        return;
    nlohmann::json simple_expr;
    simple_expr = jsonValue(&node);
    _jstack.push(simple_expr);
    Strings::free(image);
}

void SynlibJsonVisitor::SYNLIB_VISIT(SynlibReal, node)
{
    char *image = node.Image();
    if (!image)
        return;
    nlohmann::json simple_expr;
    simple_expr = node.Double();
    _jstack.push(simple_expr);
    Strings::free(image);
}

void SynlibJsonVisitor::SYNLIB_VISIT(SynlibInt, node)
{
    char *image = node.Image();
    if (!image)
        return;
    nlohmann::json simple_expr;
    simple_expr = node.Int();
    _jstack.push(simple_expr);
    Strings::free(image);
}

void SynlibJsonVisitor::SYNLIB_VISIT(SynlibString, node)
{
    char *image = node.Image();
    if (!image)
        return;
    nlohmann::json simple_expr;
    simple_expr = jsonValue(&node);
    _jstack.push(simple_expr);
    Strings::free(image);
}

/* -------------------------------------------------------------- */

// EOF
