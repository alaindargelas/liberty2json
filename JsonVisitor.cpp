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

void SynlibJsonVisitor::SYNLIB_VISIT(SynlibAttr, node)
{
    if (node.GetValue())
        TraverseNode(node.GetValue());
    nlohmann::json simple_attr;
    for (const auto &[keyo, valueo] : _tmp.items())
    {
        simple_attr[node.GetName()] = valueo;
    }
    _tmp = simple_attr;
}

void SynlibJsonVisitor::SYNLIB_VISIT(SynlibComplexAttr, node)
{
    unsigned i;
    SynlibExpr *expr;
    nlohmann::json complex_attr;
    FOREACH_ARRAY_ITEM(node.GetValues(), i, expr)
    {
        TraverseNode(expr);
        complex_attr[node.GetName()].push_back(_tmp);
    }
    _tmp = complex_attr;
}

nlohmann::json jsonValue(SynlibExpr *expr)
{
    nlohmann::json result;
    const char *s = expr->Image();
    SYNLIB_CLASS_ID expr_type = expr->GetClassId();
    if (!strcmp(s, "true"))
    {
        result = 1;
    }
    else if (!strcmp(s, "false"))
    {
        result = 0;
    }
    else if (expr_type == ID_SYNLIBSTRING)
    {
        result = expr->String();
    }
    else
    {
        result = expr->Image();
    }
    return result;
}

std::string valType(const std::string &type)
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

        SYNLIB_CLASS_ID stmt_type = stmt->GetClassId();
        // Defines use one of either keywords
        if (stmt_type == ID_SYNLIBDEFINE)
        {
            for (const auto &[keyo, valueo] : _tmp.items())
            {
                if (valueo.size() == 2 || valueo.size() == 3)
                {
                    nlohmann::json st;
                    if (defines["defines"].contains(valueo.at(0)))
                    {
                        const std::string &prev = defines["defines"][valueo.at(0)]["allowed_group_name"];
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
        else if (stmt_type == ID_SYNLIBGROUP || stmt_type == ID_SYNLIBLIBRARY || stmt_type == ID_SYNLIBCELL || stmt_type == ID_SYNLIBPIN || stmt_type == ID_SYNLIBBUS || stmt_type == ID_SYNLIBBUNDLE || stmt_type == ID_SYNLIBFLIPFLOP || stmt_type == ID_SYNLIBLATCHBANK || stmt_type == ID_SYNLIBLATCH || stmt_type == ID_SYNLIBLATCHBANK || stmt_type == ID_SYNLIBLUT || stmt_type == ID_SYNLIBSTATETABLE || stmt_type == ID_SYNLIBTYPE || stmt_type == ID_SYNLIBTESTCELL || stmt_type == ID_SYNLIBOPERATINGCONDITIONS || stmt_type == ID_SYNLIBLEAKAGEPOWER)
        {
            groups["groups"].push_back(_tmp);
        }
        // Remainders are attributes
        else
        {
            attributes["attributes"].push_back(_tmp);
        }
    }

    nlohmann::json jnames;
    for (auto n : names)
    {
        jnames["names"].push_back(n);
    }
    if (!strcmp(node.GetName(), "library"))
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

        _tmp = j;
    }
}

void SynlibJsonVisitor::SYNLIB_VISIT(SynlibExpr, node)
{
    char *image = node.Image();
    if (!image)
        return;
    _tmp = jsonValue(&node);
    Strings::free(image);
}

void SynlibJsonVisitor::SYNLIB_VISIT(SynlibReal, node)
{
    char *image = node.Image();
    if (!image)
        return;
    _tmp = node.Double();
    Strings::free(image);
}

void SynlibJsonVisitor::SYNLIB_VISIT(SynlibInt, node)
{
    char *image = node.Image();
    if (!image)
        return;
    _tmp = node.Int();
    Strings::free(image);
}

void SynlibJsonVisitor::SYNLIB_VISIT(SynlibString, node)
{
    char *image = node.Image();
    if (!image)
        return;
    _tmp = jsonValue(&node);
    Strings::free(image);
}

/* -------------------------------------------------------------- */

// EOF
