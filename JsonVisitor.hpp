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

#ifndef _VERIFIC_SYNLIB_JSON_VISITOR_H_
#define _VERIFIC_SYNLIB_JSON_VISITOR_H_

#include <ostream>
#include <stack>
#include "VerificSystem.h"
#include "SynlibCompileFlags.h"
#include "SynlibVisitor.h"
#include "lib/json.hpp"

#ifdef VERIFIC_NAMESPACE
namespace Verific { // start definitions in verific namespace
#endif

/* -------------------------------------------------------------- */

/*
    A Visitor class lets you define a new operation on a tree (or data structure)
    without changing the classes of the elements on which it operates (tree nodes).
    In essence, it lets you keep related operations together by defining them in
    one class. To get this to work, the following must occur:

    1) Declare a abstract base Visitor class which contains a "void SYNLIB_VISIT(<type>&)"
       method for every <type> of element (SynlibTreeNode) the Visitor will traverse.
       This has already been done, and is located here in this file (SynlibVisitor.h).

    2) Declare a "void Accept(SynlibVisitor &)" method for every element (SynlibTreeNode) class.

    3) To create your particular Visitor class, derive from SynlibVisitor, and
       implement only the SYNLIB_VISIT() methods needed for your particular algorithm.

    4) Please note that starting from April 2009 release SYNLIB_VISIT is a macro defined above.
       Depending on the compile switch VERILOG_SPECIALIZED_VISITORS the definition of
       the macro SYNLIB_VISIT changes. Please look into SynlibCompileFlags.h for more details.
*/

class VFC_DLL_PORT SynlibJsonVisitor : public SynlibVisitor
{
public:
    explicit SynlibJsonVisitor(nlohmann::json& obj) : SynlibVisitor(), _json(obj), _tab(0) { }
    virtual ~SynlibJsonVisitor() { }

private:
    // Prevent compiler from defining the following
    SynlibJsonVisitor(const SynlibJsonVisitor &) ;            // Purposely leave unimplemented
    SynlibJsonVisitor& operator=(const SynlibJsonVisitor &) ; // Purposely leave unimplemented

public:
    // Overrides
    virtual void SYNLIB_VISIT(SynlibAttr, node) ;
    virtual void SYNLIB_VISIT(SynlibComplexAttr, node) ;
    virtual void SYNLIB_VISIT(SynlibGroup, node) ;
    virtual void SYNLIB_VISIT(SynlibExpr, node) ;
    virtual void SYNLIB_VISIT(SynlibReal, node) ;
    virtual void SYNLIB_VISIT(SynlibInt, node) ;
    virtual void SYNLIB_VISIT(SynlibString, node) ;

private:
    nlohmann::json &_json ;
    std::stack<nlohmann::json> _jstack;
    unsigned        _tab ;
} ; // class SynlibJsonVisitor

/* -------------------------------------------------------------- */

#ifdef VERIFIC_NAMESPACE
} // end definitions in verific namespace
#endif

#endif // #ifndef _VERIFIC_SYNLIB_JSON_VISITOR_H_
