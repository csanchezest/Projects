//////////////////////////////////////////////////////////////////////
//
//    TypeCheckVisitor - Walk the parser tree to do the semantic
//                       typecheck for the Asl programming language
//
//    Copyright (C) 2017-2022  Universitat Politecnica de Catalunya
//
//    This library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU General Public License
//    as published by the Free Software Foundation; either version 3
//    of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Affero General Public License for more details.
//
//    You should have received a copy of the GNU Affero General Public
//    License along with this library; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//
//    contact: José Miguel Rivero (rivero@cs.upc.edu)
//             Computer Science Department
//             Universitat Politecnica de Catalunya
//             despatx Omega.110 - Campus Nord UPC
//             08034 Barcelona.  SPAIN
//
//////////////////////////////////////////////////////////////////////

#include "TypeCheckVisitor.h"
#include "antlr4-runtime.h"

#include "../common/TypesMgr.h"
#include "../common/SymTable.h"
#include "../common/TreeDecoration.h"
#include "../common/SemErrors.h"

#include <iostream>
#include <string>

// uncomment the following line to enable debugging messages with DEBUG*
//#define DEBUG_BUILD
#include "../common/debug.h"

// using namespace std;


// Constructor
TypeCheckVisitor::TypeCheckVisitor(TypesMgr       & Types,
                                   SymTable       & Symbols,
                                   TreeDecoration & Decorations,
                                   SemErrors      & Errors) :
  Types{Types},
  Symbols{Symbols},
  Decorations{Decorations},
  Errors{Errors} {
}

// Accessor/Mutator to the attribute currFunctionType
TypesMgr::TypeId TypeCheckVisitor::getCurrentFunctionTy() const {
  return currFunctionType;
}

void TypeCheckVisitor::setCurrentFunctionTy(TypesMgr::TypeId type) {
  currFunctionType = type;
}

// Methods to visit each kind of node:
//
antlrcpp::Any TypeCheckVisitor::visitProgram(AslParser::ProgramContext *ctx) {
  DEBUG_ENTER();
  SymTable::ScopeId sc = getScopeDecor(ctx);
  Symbols.pushThisScope(sc);
  for (auto ctxFunc : ctx->function()) { 
    visit(ctxFunc);
  }
  if (Symbols.noMainProperlyDeclared()) Errors.noMainProperlyDeclared(ctx);
  Symbols.popScope();
  Errors.print();
  DEBUG_EXIT();
  return 0;
}

antlrcpp::Any TypeCheckVisitor::visitFunction(AslParser::FunctionContext *ctx) {
  DEBUG_ENTER();
  
  SymTable::ScopeId sc = getScopeDecor(ctx);
  Symbols.pushThisScope(sc);
  // Symbols.print();
  std::vector<TypesMgr::TypeId> lParamsTy;
  TypesMgr::TypeId tRet;

  if (ctx->type()) tRet = getTypeDecor(ctx->type());
  else tRet = Types.createVoidTy();

  if (ctx->parameters()) 
      for(unsigned int i = 0; i<ctx->parameters()->ID().size();i++) lParamsTy.push_back(getTypeDecor(ctx->parameters()->type(i)));

  TypesMgr::TypeId tFunc = Types.createFunctionTy(lParamsTy, tRet);
  setCurrentFunctionTy(tFunc);
  visit(ctx->statements());

  Symbols.popScope();
  DEBUG_EXIT();
  return 0;
}

// antlrcpp::Any TypeCheckVisitor::visitDeclarations(AslParser::DeclarationsContext *ctx) {
//   DEBUG_ENTER();
//   antlrcpp::Any r = visitChildren(ctx);
//   DEBUG_EXIT();
//   return r;
// }

// antlrcpp::Any TypeCheckVisitor::visitVariable_decl(AslParser::Variable_declContext *ctx) {
//   DEBUG_ENTER();
//   antlrcpp::Any r = visitChildren(ctx);
//   DEBUG_EXIT();
//   return r;
// }

// antlrcpp::Any TypeCheckVisitor::visitType(AslParser::TypeContext *ctx) {
//   DEBUG_ENTER();
//   antlrcpp::Any r = visitChildren(ctx);
//   DEBUG_EXIT();
//   return r;
// }

antlrcpp::Any TypeCheckVisitor::visitStatements(AslParser::StatementsContext *ctx) {
  DEBUG_ENTER();
  visitChildren(ctx);
  DEBUG_EXIT();
  return 0;
}

antlrcpp::Any TypeCheckVisitor::visitAssignStmt(AslParser::AssignStmtContext *ctx) {
  DEBUG_ENTER();
  visit(ctx->left_expr());
  visit(ctx->expr());
  TypesMgr::TypeId tleft = getTypeDecor(ctx->left_expr());
  TypesMgr::TypeId tright = getTypeDecor(ctx->expr());

//   if(Types.isVoidTy(tright)) Errors.isNotFunction(ctx->expr());
  if(not Types.isErrorTy(tright) and Types.isVoidTy(tright)) Errors.isNotFunction(ctx->expr());
  else if (not Types.isErrorTy(tleft) and not Types.isErrorTy(tright) and
      not Types.copyableTypes(tleft, tright))
    Errors.incompatibleAssignment(ctx->ASSIGN());
  if (not Types.isErrorTy(tleft) and not getIsLValueDecor(ctx->left_expr()))
    Errors.nonReferenceableLeftExpr(ctx->left_expr());
  DEBUG_EXIT();
  return 0;
}

antlrcpp::Any TypeCheckVisitor::visitIfStmt(AslParser::IfStmtContext *ctx) {
  DEBUG_ENTER();
  visit(ctx->expr());
  TypesMgr::TypeId t1 = getTypeDecor(ctx->expr());
  if (not Types.isErrorTy(t1) and not Types.isBooleanTy(t1)) Errors.booleanRequired(ctx);
  visit(ctx->statements(0));
  if (ctx->statements(1)) visit(ctx->statements(1));
  DEBUG_EXIT();
  return 0;
}

antlrcpp::Any TypeCheckVisitor::visitWhileStmt(AslParser::WhileStmtContext *ctx) {
  DEBUG_ENTER();
  visit(ctx->expr());
  TypesMgr::TypeId t1 = getTypeDecor(ctx->expr());
  if (not Types.isErrorTy(t1) and not Types.isBooleanTy(t1)) Errors.booleanRequired(ctx);
  visit(ctx->statements());
  DEBUG_EXIT();
  return 0;
}

antlrcpp::Any TypeCheckVisitor::visitProcCall(AslParser::ProcCallContext *ctx) {
  DEBUG_ENTER();
  visit(ctx->ident());
  TypesMgr::TypeId t1 = getTypeDecor(ctx->ident());
  if(not Types.isErrorTy(t1) and not Types.isFunctionTy(t1)) Errors.isNotCallable(ctx); // Create error, t1 is not error and is not function
  else if (not Types.isErrorTy(t1)) {
    if(Types.getNumOfParameters(t1) > 0) {
      std::vector<TypesMgr::TypeId> lParamsTy = Types.getFuncParamsTypes(t1);
      if (!ctx->list_expr() or ctx->list_expr()->expr().size() != lParamsTy.size()) Errors.numberOfParameters(ctx);
      if(ctx->list_expr()) {
        visit(ctx->list_expr());
//         if(ctx->list_expr()->expr().size() == lParamsTy.size()) {
          TypesMgr::TypeId t2;
//           for (uint i = 0; i < lParamsTy.size(); i++) {
          for (unsigned int i = 0;i < ctx->list_expr()->expr().size() && i < lParamsTy.size();i++) {
            t2 = getTypeDecor(ctx->list_expr()->expr(i));
            if (not Types.isErrorTy(t2) and not Types.isErrorTy(lParamsTy[i]) and not Types.copyableTypes(lParamsTy[i], t2)) 
              Errors.incompatibleParameter(ctx->list_expr()->expr(i),i+1,ctx);
          }
//         }
      }
    } else if (ctx->list_expr()) {
      Errors.numberOfParameters(ctx);
      visit(ctx->list_expr());
    }
  }
  DEBUG_EXIT();
  return 0;
}

antlrcpp::Any TypeCheckVisitor::visitReadStmt(AslParser::ReadStmtContext *ctx) {
  DEBUG_ENTER();
  visit(ctx->left_expr());
  TypesMgr::TypeId t1 = getTypeDecor(ctx->left_expr());
  if (not Types.isErrorTy(t1) and not Types.isPrimitiveTy(t1) and
      not Types.isFunctionTy(t1)) Errors.readWriteRequireBasic(ctx);
  if (not Types.isErrorTy(t1) and not getIsLValueDecor(ctx->left_expr())) Errors.nonReferenceableExpression(ctx);
  DEBUG_EXIT();
  return 0;
}

antlrcpp::Any TypeCheckVisitor::visitWriteExpr(AslParser::WriteExprContext *ctx) {
  DEBUG_ENTER();
  visit(ctx->expr());
  TypesMgr::TypeId t1 = getTypeDecor(ctx->expr());
  if (not Types.isErrorTy(t1) and not Types.isPrimitiveTy(t1)) Errors.readWriteRequireBasic(ctx);
  DEBUG_EXIT();
  return 0;
}

// antlrcpp::Any TypeCheckVisitor::visitWriteString(AslParser::WriteStringContext *ctx) {
//   DEBUG_ENTER();
//   antlrcpp::Any r = visitChildren(ctx);
//   DEBUG_EXIT();
//   return r;
// }

antlrcpp::Any TypeCheckVisitor::visitReturnStmt(AslParser::ReturnStmtContext *ctx) {
  DEBUG_ENTER();
  TypesMgr::TypeId t2 = getCurrentFunctionTy();
  t2 = Types.getFuncReturnType(t2);
  visitChildren(ctx);
  TypesMgr::TypeId t1;
  if (ctx->expr()) t1 = getTypeDecor(ctx->expr());
  else t1 = Types.createVoidTy();
  if (not Types.isErrorTy(t1) and not Types.isErrorTy(t2) and
      not Types.copyableTypes(t2, t1)) Errors.incompatibleReturn(ctx->RETURN());    
  DEBUG_EXIT();
  return 0;
}

antlrcpp::Any TypeCheckVisitor::visitLeft_expr(AslParser::Left_exprContext *ctx) {
  DEBUG_ENTER();
  visit(ctx->ident());
  TypesMgr::TypeId t1 = getTypeDecor(ctx->ident());
  if (!ctx->expr()) putTypeDecor(ctx, t1);
  else {
    bool err = false;
    visit(ctx->expr());
    TypesMgr::TypeId t2 = getTypeDecor(ctx->expr());
    if (Types.isErrorTy(t1)) err = true;
    else if (not Types.isArrayTy(t1)) {
      Errors.nonArrayInArrayAccess(ctx);
      err = true;
    }
    if (not Types.isErrorTy(t2) and not Types.isIntegerTy(t2)) Errors.nonIntegerIndexInArrayAccess(ctx->expr());
    TypesMgr::TypeId t;
    if (!err) t = Types.getArrayElemType(t1);
    else t = Types.createErrorTy();
    putTypeDecor(ctx, t);
  }  
  bool b = getIsLValueDecor(ctx->ident());
  putIsLValueDecor(ctx, b);
  DEBUG_EXIT();
  return 0;
}

antlrcpp::Any TypeCheckVisitor::visitArrayAccess(AslParser::ArrayAccessContext *ctx) {
  DEBUG_ENTER();
  visit(ctx->ident());
  TypesMgr::TypeId t1 = getTypeDecor(ctx->ident());
  visit(ctx->expr());
  TypesMgr::TypeId t2 = getTypeDecor(ctx->expr());
  bool err = false;
  if (Types.isErrorTy(t1)) err = true;
  else if (not Types.isArrayTy(t1)) {
    Errors.nonArrayInArrayAccess(ctx);
    err = true;
  }
  if (not Types.isErrorTy(t2) and not Types.isIntegerTy(t2)) Errors.nonIntegerIndexInArrayAccess(ctx->expr());
  TypesMgr::TypeId t;
  if (!err) t = Types.getArrayElemType(t1);
  else t = Types.createErrorTy();
  putTypeDecor(ctx, t);
  putIsLValueDecor(ctx, false);
  DEBUG_EXIT();
  return 0;
}

antlrcpp::Any TypeCheckVisitor::visitUnaryOps(AslParser::UnaryOpsContext *ctx) {
  DEBUG_ENTER();
  visit(ctx->expr());
  TypesMgr::TypeId t1 = getTypeDecor(ctx->expr());
  std::string oper = ctx->op->getText();
  if (not Types.isErrorTy(t1) and ((Types.isCharacterTy(t1)) or 
    ((oper == "not") and not Types.isBooleanTy(t1)) or 
    ((oper != "not") and (Types.isBooleanTy(t1))))) 
      Errors.incompatibleOperator(ctx->op);
  TypesMgr::TypeId t;
  if (oper=="not") t = Types.createBooleanTy();
  else if (Types.isFloatTy(t1)) t = Types.createFloatTy();
  else t = Types.createIntegerTy();
//   else if (Types.isIntegerTy(t1)) t = Types.createIntegerTy();
//   else t = Types.createErrorTy();
  putTypeDecor(ctx, t);
  putIsLValueDecor(ctx, false);
  DEBUG_EXIT();
  return 0;
}

antlrcpp::Any TypeCheckVisitor::visitArithmetic(AslParser::ArithmeticContext *ctx) {
  DEBUG_ENTER();
  visit(ctx->expr(0));
  TypesMgr::TypeId t1 = getTypeDecor(ctx->expr(0));
  visit(ctx->expr(1));
  TypesMgr::TypeId t2 = getTypeDecor(ctx->expr(1));
  std::string op = ctx->op->getText();
  if ((not Types.isErrorTy(t1) and not Types.isNumericTy(t1)) or
      (not Types.isErrorTy(t2) and not Types.isNumericTy(t2))) 
      Errors.incompatibleOperator(ctx->op);
  else if (not Types.isErrorTy(t1) and not Types.isErrorTy(t2) and op=="%" and not Types.equalTypes(t1,t2)) Errors.incompatibleOperator(ctx->op);
  TypesMgr::TypeId t;
  if (Types.isFloatTy(t1) or Types.isFloatTy(t2)) t = Types.createFloatTy();
  else t = Types.createIntegerTy();
  putTypeDecor(ctx, t);
  putIsLValueDecor(ctx, false);
  DEBUG_EXIT();
  return 0;
}

antlrcpp::Any TypeCheckVisitor::visitRelational(AslParser::RelationalContext *ctx) {
  DEBUG_ENTER();
  visit(ctx->expr(0));
  TypesMgr::TypeId t1 = getTypeDecor(ctx->expr(0));
  visit(ctx->expr(1));
  TypesMgr::TypeId t2 = getTypeDecor(ctx->expr(1));
  std::string oper = ctx->op->getText();
  if (not Types.isErrorTy(t1) and not Types.isErrorTy(t2) and
      not Types.comparableTypes(t1, t2, oper)) 
      Errors.incompatibleOperator(ctx->op);
  TypesMgr::TypeId t = Types.createBooleanTy();
  putTypeDecor(ctx, t);
  putIsLValueDecor(ctx, false);
  DEBUG_EXIT();
  return 0;
}

antlrcpp::Any TypeCheckVisitor::visitLogical(AslParser::LogicalContext *ctx) {
  DEBUG_ENTER();
  visit(ctx->expr(0));
  TypesMgr::TypeId t1 = getTypeDecor(ctx->expr(0));
  visit(ctx->expr(1));
  TypesMgr::TypeId t2 = getTypeDecor(ctx->expr(1));
  if ((not Types.isErrorTy(t1) and not Types.isBooleanTy(t1)) or 
      (not Types.isErrorTy(t2) and not Types.isBooleanTy(t2))) 
      Errors.incompatibleOperator(ctx->op);
  TypesMgr::TypeId t = Types.createBooleanTy();
  putTypeDecor(ctx, t);
  putIsLValueDecor(ctx, false);
  DEBUG_EXIT();
  return 0;
}

antlrcpp::Any TypeCheckVisitor::visitValue(AslParser::ValueContext *ctx) {
  DEBUG_ENTER();
  TypesMgr::TypeId t;
  if (ctx->INTVAL()) t = Types.createIntegerTy();
  else if (ctx->FLOATVAL()) t = Types.createFloatTy();
  else if (ctx->CHARVAL()) t = Types.createCharacterTy();
  else t = Types.createBooleanTy();
  putTypeDecor(ctx, t);
  putIsLValueDecor(ctx, false);
  DEBUG_EXIT();
  return 0;
}

antlrcpp::Any TypeCheckVisitor::visitExprFunc(AslParser::ExprFuncContext *ctx) {
  DEBUG_ENTER();
  visit(ctx->ident());
  TypesMgr::TypeId t1 = getTypeDecor(ctx->ident());
  TypesMgr::TypeId t;
  if(not Types.isErrorTy(t1) and not Types.isFunctionTy(t1)) {
    Errors.isNotCallable(ctx); // Create error, t1 is not error and is not function
    t = Types.createErrorTy();
    if(ctx->list_expr()) visit(ctx->list_expr());
  } else if (not Types.isErrorTy(t1)) {
    t = Types.getFuncReturnType(t1);
    if(Types.getNumOfParameters(t1) > 0) {
      std::vector<TypesMgr::TypeId> lParamsTy = Types.getFuncParamsTypes(t1);
      if (!ctx->list_expr() or ctx->list_expr()->expr().size() != lParamsTy.size()) Errors.numberOfParameters(ctx);
      if(ctx->list_expr()) {
        visit(ctx->list_expr());
        // Potser aquest if sobra
//         if(ctx->list_expr()->expr().size() == lParamsTy.size()) {
        TypesMgr::TypeId t2;
        for (unsigned int i = 0;i < ctx->list_expr()->expr().size() && i < lParamsTy.size();i++) {
//             for (uint i = 0; i < lParamsTy.size(); i++) {
            t2 = getTypeDecor(ctx->list_expr()->expr(i));
            if (not Types.isErrorTy(t2) and not Types.isErrorTy(lParamsTy[i]) and not Types.copyableTypes(lParamsTy[i], t2)) 
            Errors.incompatibleParameter(ctx->list_expr()->expr(i),i+1,ctx);
        }
//         }
      }
    } else if (ctx->list_expr()) {
      Errors.numberOfParameters(ctx);
      visit(ctx->list_expr());
    }
  } else t = Types.createErrorTy(); // Create error, t1 is error
  putTypeDecor(ctx,t);
  DEBUG_EXIT();
  return 0;
}

antlrcpp::Any TypeCheckVisitor::visitExprIdent(AslParser::ExprIdentContext *ctx) {
  DEBUG_ENTER();
  visit(ctx->ident());
  TypesMgr::TypeId t1 = getTypeDecor(ctx->ident());
  putTypeDecor(ctx, t1);
  bool b = getIsLValueDecor(ctx->ident());
  putIsLValueDecor(ctx, b);
  DEBUG_EXIT();
  return 0;
}

antlrcpp::Any TypeCheckVisitor::visitParens(AslParser::ParensContext *ctx) {
  DEBUG_ENTER();
  visit(ctx->expr());
  TypesMgr::TypeId t1 = getTypeDecor(ctx->expr());
  putTypeDecor(ctx, t1);
  putIsLValueDecor(ctx, false);
  DEBUG_EXIT();
  return 0;
}

antlrcpp::Any TypeCheckVisitor::visitIdent(AslParser::IdentContext *ctx) {
  DEBUG_ENTER();
  std::string ident = ctx->getText();
  if (Symbols.findInStack(ident) == -1) {
    Errors.undeclaredIdent(ctx->ID());
    TypesMgr::TypeId te = Types.createErrorTy();
    putTypeDecor(ctx, te);
    putIsLValueDecor(ctx, true);
  }
  else {
    TypesMgr::TypeId t1 = Symbols.getType(ident);
    putTypeDecor(ctx, t1);
    if (Symbols.isFunctionClass(ident)) putIsLValueDecor(ctx, false);
    else putIsLValueDecor(ctx, true);
  }
  DEBUG_EXIT();
  return 0;
}

antlrcpp::Any TypeCheckVisitor::visitList_expr(AslParser::List_exprContext *ctx) {
  DEBUG_ENTER();
  visitChildren(ctx);
  DEBUG_EXIT();
  return 0;
}

// Getters for the necessary tree node atributes:
//   Scope, Type ans IsLValue
SymTable::ScopeId TypeCheckVisitor::getScopeDecor(antlr4::ParserRuleContext *ctx) {
  return Decorations.getScope(ctx);
}
TypesMgr::TypeId TypeCheckVisitor::getTypeDecor(antlr4::ParserRuleContext *ctx) {
  return Decorations.getType(ctx);
}
bool TypeCheckVisitor::getIsLValueDecor(antlr4::ParserRuleContext *ctx) {
  return Decorations.getIsLValue(ctx);
}

// Setters for the necessary tree node attributes:
//   Scope, Type ans IsLValue
void TypeCheckVisitor::putScopeDecor(antlr4::ParserRuleContext *ctx, SymTable::ScopeId s) {
  Decorations.putScope(ctx, s);
}
void TypeCheckVisitor::putTypeDecor(antlr4::ParserRuleContext *ctx, TypesMgr::TypeId t) {
  Decorations.putType(ctx, t);
}
void TypeCheckVisitor::putIsLValueDecor(antlr4::ParserRuleContext *ctx, bool b) {
  Decorations.putIsLValue(ctx, b);
}
