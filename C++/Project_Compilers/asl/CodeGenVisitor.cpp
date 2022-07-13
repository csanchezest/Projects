//////////////////////////////////////////////////////////////////////
//
//    CodeGenVisitor - Walk the parser tree to do
//                     the generation of code
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

#include "CodeGenVisitor.h"
#include "antlr4-runtime.h"

#include "../common/TypesMgr.h"
#include "../common/SymTable.h"
#include "../common/TreeDecoration.h"
#include "../common/code.h"

#include <string>
#include <cstddef>    // std::size_t

// uncomment the following line to enable debugging messages with DEBUG*
// #define DEBUG_BUILD
#include "../common/debug.h"

// using namespace std;


// Constructor
CodeGenVisitor::CodeGenVisitor(TypesMgr       & Types,
                               SymTable       & Symbols,
                               TreeDecoration & Decorations) :
  Types{Types},
  Symbols{Symbols},
  Decorations{Decorations} {
}

// Accessor/Mutator to the attribute currFunctionType
TypesMgr::TypeId CodeGenVisitor::getCurrentFunctionTy() const {
  return currFunctionType;
}

void CodeGenVisitor::setCurrentFunctionTy(TypesMgr::TypeId type) {
  currFunctionType = type;
}

// Methods to visit each kind of node:
antlrcpp::Any CodeGenVisitor::visitProgram(AslParser::ProgramContext *ctx) {
  DEBUG_ENTER();
  code my_code;
  SymTable::ScopeId sc = getScopeDecor(ctx);
  Symbols.pushThisScope(sc);
  for (auto ctxFunc : ctx->function()) { 
    subroutine subr = visit(ctxFunc);
    my_code.add_subroutine(subr);
  }
  Symbols.popScope();
  DEBUG_EXIT();
  return my_code;
}

antlrcpp::Any CodeGenVisitor::visitFunction(AslParser::FunctionContext *ctx) {
  DEBUG_ENTER();
  SymTable::ScopeId sc = getScopeDecor(ctx);
  Symbols.pushThisScope(sc);
  subroutine subr(ctx->ID()->getText());
  codeCounters.reset();
  std::vector<var> && lvars = visit(ctx->declarations());
  for (auto & onevar : lvars) subr.add_var(onevar);
  if(ctx->type()) {
      subr.add_param("_result");
      TypesMgr::TypeId tRet = getTypeDecor(ctx->type());
      setCurrentFunctionTy(tRet);
  }
  //bool fparam = false;
  instructionList code;
  if (ctx->parameters()) {
    std::vector<var> && lparams = visit(ctx->parameters());
    for (auto & oneparam : lparams) subr.add_param(oneparam.name);
  }
    
  code = visit(ctx->statements());
  code = code || instruction(instruction::RETURN());
  subr.set_instructions(code);
  Symbols.popScope();
  DEBUG_EXIT();
  return subr;
}

antlrcpp::Any CodeGenVisitor::visitDeclarations(AslParser::DeclarationsContext *ctx) {
  DEBUG_ENTER();
  std::vector<var> lvars;
  for (auto & varDeclCtx : ctx->variable_decl()) {
    std::vector<var> vars = visit(varDeclCtx);
    for(auto & onevar : vars) lvars.push_back(onevar);
//     var onevar = visit(varDeclCtx);
//     lvars.push_back(onevar);
  }
  DEBUG_EXIT();
  return lvars;
}

antlrcpp::Any CodeGenVisitor::visitVariable_decl(AslParser::Variable_declContext *ctx) {
  DEBUG_ENTER();
  std::vector<var> lvars;
  TypesMgr::TypeId   t1 = getTypeDecor(ctx->type());
  std::size_t      size = Types.getSizeOfType(t1);
  for (uint i = 0; i < ctx->ID().size(); i++) lvars.push_back(var{ctx->ID(i)->getText(), size});
  DEBUG_EXIT();
  return lvars;
//   return var{ctx->ID(0)->getText(), size};
}

antlrcpp::Any CodeGenVisitor::visitParameters(AslParser::ParametersContext *ctx) {
  DEBUG_ENTER();
  std::vector<var> lparams;
  for (uint i = 0; i < ctx->ID().size(); i++) {
    TypesMgr::TypeId   t1 = getTypeDecor(ctx->type(i));
    std::size_t      size = Types.getSizeOfType(t1);
    lparams.push_back(var{ctx->ID(i)->getText(), size});
  }
  DEBUG_EXIT();
  return lparams;
}

antlrcpp::Any CodeGenVisitor::visitStatements(AslParser::StatementsContext *ctx) {
  DEBUG_ENTER();
  instructionList code;
  for (auto stCtx : ctx->statement()) {
    instructionList && codeS = visit(stCtx);
    code = code || codeS;
  }
  DEBUG_EXIT();
  return code;
}

antlrcpp::Any CodeGenVisitor::visitAssignStmt(AslParser::AssignStmtContext *ctx) {
  DEBUG_ENTER();
  instructionList code;
  CodeAttribs     && codAtsE1 = visit(ctx->left_expr());
  std::string           addr1 = codAtsE1.addr;
  std::string           offs1 = codAtsE1.offs;
  instructionList &     code1 = codAtsE1.code;
  TypesMgr::TypeId tid1 = getTypeDecor(ctx->left_expr());
  CodeAttribs     && codAtsE2 = visit(ctx->expr());
  std::string           addr2 = codAtsE2.addr;
  std::string           offs2 = codAtsE2.offs;
  instructionList &     code2 = codAtsE2.code;
  TypesMgr::TypeId tid2 = getTypeDecor(ctx->expr());
  if (ctx->left_expr()->expr()) {
    CodeAttribs     && codAtsE3 = visit(ctx->left_expr()->expr());
    std::string           addr3 = codAtsE3.addr;
    instructionList &     code3 = codAtsE3.code;
    code = code1 || code3 || code2;
    if(Types.isFloatTy(tid1) and Types.isIntegerTy(tid2)) {
        std::string temp = "%"+codeCounters.newTEMP();
        code = code || instruction::FLOAT(temp, addr2) || instruction::XLOAD(addr1, addr3 , temp);
    } else code = code || instruction::XLOAD(addr1, addr3 , addr2);
  } else if (Types.isFloatTy(tid1) and Types.isIntegerTy(tid2) ) {
    std::string temp = "%"+codeCounters.newTEMP();
    code = code1 || code2 || instruction::FLOAT(temp, addr2) || instruction::LOAD(addr1, temp);
  } else if (Types.isArrayTy(tid1) and Types.isArrayTy(tid2)) {
    std::string temp = "%"+codeCounters.newTEMP();
    std::string i = "%"+codeCounters.newTEMP();
    std::string k = "%"+codeCounters.newTEMP();
    std::string cond = "%"+codeCounters.newTEMP();
    std::string size = "%"+codeCounters.newTEMP();
    std::string label = "while"+codeCounters.newLabelWHILE();
    std::string labelEndWhile = "end"+label;
    unsigned int length = Types.getArraySize(tid2);
        
    code = code1 || code2;
    code = code || instruction::ILOAD(size, std::to_string(length)) || instruction::ILOAD(i, "0") || instruction::ILOAD(k, "1");
    code = code || instruction::LABEL(label) || instruction::LT(cond, i, size) || instruction::FJUMP(cond, labelEndWhile);
    code = code || instruction::LOADX(temp, addr2, i) || instruction::XLOAD(addr1, i, temp);
    code = code || instruction::ADD(i, i, k) || instruction::UJUMP(label) || instruction::LABEL(labelEndWhile);
  } else code = code1 || code2 || instruction::LOAD(addr1, addr2);
  DEBUG_EXIT();
  return code;
}

antlrcpp::Any CodeGenVisitor::visitIfStmt(AslParser::IfStmtContext *ctx) {
  DEBUG_ENTER();
  instructionList code;
  CodeAttribs     && codAtsE = visit(ctx->expr());
  std::string          addr1 = codAtsE.addr;
  instructionList &    code1 = codAtsE.code;
  instructionList &&   code2 = visit(ctx->statements(0));
  std::string label = codeCounters.newLabelIF();
  std::string labelElse = "else"+label;
  std::string labelEndIf = "endif"+label;
  if(ctx->statements(1)) {
    instructionList &&   code3 = visit(ctx->statements(1));
    code = code1 || instruction::FJUMP(addr1, labelElse) || code2 || instruction::UJUMP(labelEndIf) || instruction::LABEL(labelElse) || code3;
  } else {
      code = code1 || instruction::FJUMP(addr1, labelEndIf) || code2 ;
  }
  code = code || instruction::LABEL(labelEndIf) ; 
  DEBUG_EXIT();
  return code;
}

antlrcpp::Any CodeGenVisitor::visitWhileStmt(AslParser::WhileStmtContext *ctx) {
  DEBUG_ENTER();
  instructionList code;
  CodeAttribs     && codAtsE = visit(ctx->expr());
  std::string          addr1 = codAtsE.addr;
  instructionList &    code1 = codAtsE.code;
  instructionList &&   code2 = visit(ctx->statements());
  std::string label = "while"+codeCounters.newLabelWHILE();
  std::string labelEndWhile = "end"+label;
//   std::string temp = "%"+codeCounters.newTEMP();
  code = instruction::LABEL(label) || code1 || instruction::FJUMP(addr1, labelEndWhile) ||
         code2 || instruction::UJUMP(label) || instruction::LABEL(labelEndWhile);
  DEBUG_EXIT();
  return code;
}

antlrcpp::Any CodeGenVisitor::visitProcCall(AslParser::ProcCallContext *ctx) {
  DEBUG_ENTER();
  instructionList code = instruction::PUSH("");
  if (ctx->list_expr()) {
    TypesMgr::TypeId tid = getTypeDecor(ctx->ident());
    std::vector<TypesMgr::TypeId> params =  Types.getFuncParamsTypes(tid);
    for (uint i = 0; i < ctx->list_expr()->expr().size(); i++) {
        TypesMgr::TypeId tidp = getTypeDecor(ctx->list_expr()->expr(i));
        CodeAttribs     && codAt = visit(ctx->list_expr()->expr(i));
        instructionList &   exprCode = codAt.code;
        std::string         addr = codAt.addr;
        code = code || exprCode;
//         if (Types.isFloatTy(params[i]) && Types.isIntegerTy(tidp) && not Types.isArrayTy(params[i])) {
        if (Types.isFloatTy(params[i]) && Types.isIntegerTy(tidp)) {
            std::string temp = "%"+codeCounters.newTEMP();
            code = code || instruction::FLOAT(temp,addr) || instruction::PUSH(temp);
        } 
        else if (Types.isArrayTy(params[i])) {
            std::string name = ctx->list_expr()->expr(i)->getText();
            bool isParam = Symbols.isParameterClass(name);
            std::string temp = "%"+codeCounters.newTEMP();
            if (isParam) code = code || instruction::LOAD(temp, addr);
            else code = code || instruction::ALOAD(temp, addr);
            code = code || instruction::PUSH(temp);
        } else code = code || instruction::PUSH(addr);
    }
    code = code || instruction::CALL(ctx->ident()->getText());
    for (uint i = 0;i < ctx->list_expr()->expr().size(); i++) code = code || instruction::POP("");
  } else {
    std::string name = ctx->ident()->getText();
    code = code || instruction::CALL(name);
  }
  code = code || instruction::POP("");
  DEBUG_EXIT();
  return code;
}

antlrcpp::Any CodeGenVisitor::visitReadStmt(AslParser::ReadStmtContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs     && codAtsE = visit(ctx->left_expr());
  std::string          addr1 = codAtsE.addr;
  instructionList &    code1 = codAtsE.code;
  instructionList &     code = code1;
  TypesMgr::TypeId tid1 = getTypeDecor(ctx->left_expr());
  if (ctx->left_expr()->expr()) {
    std::string temp = "%"+codeCounters.newTEMP();
    if (Types.isIntegerTy(tid1) || Types.isBooleanTy(tid1)) code = code1 || instruction::READI(temp);
    else if (Types.isFloatTy(tid1)) code = code1 || instruction::READF(temp);
    else code = code1 || instruction::READC(temp);
    CodeAttribs     && codAtsE3 = visit(ctx->left_expr()->expr());
    std::string           addr3 = codAtsE3.addr;
    instructionList &     code3 = codAtsE3.code;
    code = code || code3 || instruction::XLOAD(addr1, addr3, temp);
  } else {
    if (Types.isIntegerTy(tid1) || Types.isBooleanTy(tid1)) code = code || instruction::READI(addr1);
    else if (Types.isFloatTy(tid1)) code = code || instruction::READF(addr1);
    else code = code || instruction::READC(addr1);
  }
  DEBUG_EXIT();
  return code;
}

antlrcpp::Any CodeGenVisitor::visitWriteExpr(AslParser::WriteExprContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs     && codAt1 = visit(ctx->expr());
  std::string         addr1 = codAt1.addr;
  instructionList &   code1 = codAt1.code;
  instructionList &    code = code1;
  TypesMgr::TypeId tid1 = getTypeDecor(ctx->expr());
  if(Types.isCharacterTy(tid1)) code = code || instruction::WRITEC(addr1);
  else if(Types.isFloatTy(tid1)) code = code || instruction::WRITEF(addr1);
  else if(Types.isIntegerTy(tid1) || Types.isBooleanTy(tid1)) code = code || instruction::WRITEI(addr1);
  DEBUG_EXIT();
  return code;
}

antlrcpp::Any CodeGenVisitor::visitWriteString(AslParser::WriteStringContext *ctx) {
  DEBUG_ENTER();
  instructionList code;
  std::string s = ctx->STRING()->getText();
  code = code || instruction::WRITES(s);
  DEBUG_EXIT();
  return code;
}

antlrcpp::Any CodeGenVisitor::visitReturnStmt(AslParser::ReturnStmtContext *ctx) {
  DEBUG_ENTER();
  instructionList code;
  if(ctx->expr()) {
    CodeAttribs     && codAt1 = visit(ctx->expr());
    std::string         addr1 = codAt1.addr;
    TypesMgr::TypeId t2 = getCurrentFunctionTy();
    TypesMgr::TypeId t = getTypeDecor(ctx->expr());
    if (Types.isBooleanTy(t2) or Types.isIntegerTy(t2)) code = codAt1.code || instruction::ILOAD("_result", addr1);
    else if (Types.isCharacterTy(t2) and not Types.isCharacterTy(t)) code = codAt1.code || instruction::CHLOAD("_result", addr1);
    else {
        std::string temp = "%"+codeCounters.newTEMP();
        if (Types.isIntegerTy(t)) code = codAt1.code  || instruction::FLOAT(temp, addr1) || instruction::FLOAD("_result", temp);
        else code = codAt1.code  || instruction::FLOAD("_result", addr1);
    }
  } 
  code = code || instruction::RETURN();
  DEBUG_EXIT();
  return code;
}

antlrcpp::Any CodeGenVisitor::visitLeft_expr(AslParser::Left_exprContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs && codAts = visit(ctx->ident());
  DEBUG_EXIT();
  return codAts;
}

antlrcpp::Any CodeGenVisitor::visitArrayAccess(AslParser::ArrayAccessContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs && codAts1 = visit(ctx->ident());
  CodeAttribs     && codAts2 = visit(ctx->expr());

  std::string temp = "%"+codeCounters.newTEMP();
  std::string         addr1 = codAts1.addr;
  //std::string           offs1 = codAts1.offs;
  instructionList &   code1 = codAts1.code;
  instructionList &   code2 = codAts2.code;
  std::string         addr2 = codAts2.addr;

  instructionList code = code1 || code2 || instruction::LOADX(temp, addr1, addr2);
  CodeAttribs codAts(temp, "", code);
  DEBUG_EXIT();
  return codAts;
}

antlrcpp::Any CodeGenVisitor::visitUnaryOps(AslParser::UnaryOpsContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs     && codAt = visit(ctx->expr());
  instructionList &   code = codAt.code;
  std::string         addr = codAt.addr;
  std::string temp = "%"+codeCounters.newTEMP();
  if(ctx->NOT()) code = code || instruction::NOT(temp, addr);
  else if(ctx->NEG()) {
    TypesMgr::TypeId texpr = getTypeDecor(ctx->expr());
    if (Types.isFloatTy(texpr)) code = code || instruction::FNEG(temp, addr);
    else code = code || instruction::NEG(temp, addr);
  } else {
    TypesMgr::TypeId texpr = getTypeDecor(ctx->expr());
    std::string temp1 = "%"+codeCounters.newTEMP();
    if (Types.isFloatTy(texpr)) code = code || instruction::FLOAD(temp1, "0.0") || instruction::FADD(temp, temp1, addr);
    else code = code || instruction::ILOAD(temp1, "0") || instruction::ADD(temp, temp1, addr);
  }
  CodeAttribs codAts(temp, "", code);
  DEBUG_EXIT();
  return codAts;
}

antlrcpp::Any CodeGenVisitor::visitArithmetic(AslParser::ArithmeticContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs     && codAt1 = visit(ctx->expr(0));
  std::string         addr1 = codAt1.addr;
  instructionList &   code1 = codAt1.code;
  CodeAttribs     && codAt2 = visit(ctx->expr(1));
  std::string         addr2 = codAt2.addr;
  instructionList &   code2 = codAt2.code;
  instructionList &&   code = code1 || code2;
  TypesMgr::TypeId t1 = getTypeDecor(ctx->expr(0));
  TypesMgr::TypeId t2 = getTypeDecor(ctx->expr(1));
  TypesMgr::TypeId  t = getTypeDecor(ctx);
  std::string temp = "%"+codeCounters.newTEMP();
  std::string temp1 = "%"+codeCounters.newTEMP(), temp2 = "%"+codeCounters.newTEMP();
  if (ctx->MUL()) {
    if(Types.isFloatTy(t)) {
        if(not Types.isFloatTy(t1)) code = code || instruction::FLOAT(temp1, addr1);
        else temp1 = addr1;
        if(not Types.isFloatTy(t2)) code = code || instruction::FLOAT(temp2, addr2);
        else temp2 = addr2;
        code = code || instruction::FMUL(temp, temp1, temp2);
    }
    else code = code || instruction::MUL(temp, addr1, addr2);
  }
  else if (ctx->PLUS()) {
    if(Types.isFloatTy(t)) {
        if(not Types.isFloatTy(t1)) code = code || instruction::FLOAT(temp1, addr1);
        else temp1 = addr1;
        if(not Types.isFloatTy(t2)) code = code || instruction::FLOAT(temp2, addr2);
        else temp2 = addr2;
        code = code || instruction::FADD(temp, temp1, temp2);
    }
    else code = code || instruction::ADD(temp, addr1, addr2);
  }
  else if (ctx->NEG()) {
    if(Types.isFloatTy(t)) {
        if(not Types.isFloatTy(t1)) code = code || instruction::FLOAT(temp1, addr1);
        else temp1 = addr1;
        if(not Types.isFloatTy(t2)) code = code || instruction::FLOAT(temp2, addr2);
        else temp2 = addr2;
        code = code || instruction::FSUB(temp, temp1, temp2);
    }
    else code = code || instruction::SUB(temp, addr1, addr2);
  }
  else if (ctx->MOD()) {
    std::string tempM1 = "%"+codeCounters.newTEMP(), tempM2 = "%"+codeCounters.newTEMP();
    if(Types.isFloatTy(t)) {
        if(not Types.isFloatTy(t1)) code = code || instruction::FLOAT(temp1, addr1);
        else temp1 = addr1;
        if(not Types.isFloatTy(t2)) code = code || instruction::FLOAT(temp2, addr2);
        else temp2 = addr2;
        code = code || instruction::FDIV(tempM1, temp1, temp2) || instruction::FMUL(tempM2, tempM1, temp2) || instruction::FSUB(temp, temp1, tempM2);
    }
    else code = code || instruction::DIV(tempM1, addr1, addr2) || instruction::MUL(tempM2, tempM1, addr2) || instruction::SUB(temp, addr1, tempM2);
  }
  else {
    if(Types.isFloatTy(t)) {
        if(not Types.isFloatTy(t1)) code = code || instruction::FLOAT(temp1, addr1);
        else temp1 = addr1;
        if(not Types.isFloatTy(t2)) code = code || instruction::FLOAT(temp2, addr2);
        else temp2 = addr2;
        code = code || instruction::FDIV(temp, temp1, temp2);
    }
    else code = code || instruction::DIV(temp, addr1, addr2);
  }
  CodeAttribs codAts(temp, "", code);
  DEBUG_EXIT();
  return codAts;
}

antlrcpp::Any CodeGenVisitor::visitRelational(AslParser::RelationalContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs     && codAt1 = visit(ctx->expr(0));
  std::string         addr1 = codAt1.addr;
  instructionList &   code1 = codAt1.code;
  CodeAttribs     && codAt2 = visit(ctx->expr(1));
  std::string         addr2 = codAt2.addr;
  instructionList &   code2 = codAt2.code;
  instructionList &&   code = code1 || code2;
  TypesMgr::TypeId t1 = getTypeDecor(ctx->expr(0));
  TypesMgr::TypeId t2 = getTypeDecor(ctx->expr(1));
  //TypesMgr::TypeId  t = getTypeDecor(ctx);
  std::string temp = "%"+codeCounters.newTEMP();
  std::string temp1 = "%"+codeCounters.newTEMP(), temp2 = "%"+codeCounters.newTEMP();
  
  if(ctx->EQUAL()) {
    if(not Types.isFloatTy(t1) and not Types.isFloatTy(t2)) code = code || instruction::EQ(temp, addr1, addr2);
    else {
        if(not Types.isFloatTy(t1)) code = code || instruction::FLOAT(temp1, addr1);
        else temp1 = addr1;
        if(not Types.isFloatTy(t2)) code = code || instruction::FLOAT(temp2, addr2);
        else temp2 = addr2;
        code = code || instruction::FEQ(temp, temp1, temp2);
    }
  } else if(ctx->GEQ()) {
    if(not Types.isFloatTy(t1) and not Types.isFloatTy(t2)) code = code || instruction::LE(temp, addr2, addr1);
    else {
        if(not Types.isFloatTy(t1)) code = code || instruction::FLOAT(temp1, addr1);
        else temp1 = addr1;
        if(not Types.isFloatTy(t2)) code = code || instruction::FLOAT(temp2, addr2);
        else temp2 = addr2;
        code = code || instruction::FLE(temp, temp2, temp1);
    }
  } else if(ctx->GT()) {
    if(not Types.isFloatTy(t1) and not Types.isFloatTy(t2)) code = code || instruction::LT(temp, addr2, addr1);
    else {
        if(not Types.isFloatTy(t1)) code = code || instruction::FLOAT(temp1, addr1);
        else temp1 = addr1;
        if(not Types.isFloatTy(t2)) code = code || instruction::FLOAT(temp2, addr2);
        else temp2 = addr2;
        code = code || instruction::FLT(temp, temp2, temp1);
    }
  } else if(ctx->LEQ()) {
    if(not Types.isFloatTy(t1) and not Types.isFloatTy(t2)) code = code || instruction::LE(temp, addr1, addr2);
    else {
        if(not Types.isFloatTy(t1)) code = code || instruction::FLOAT(temp1, addr1);
        else temp1 = addr1;
        if(not Types.isFloatTy(t2)) code = code || instruction::FLOAT(temp2, addr2);
        else temp2 = addr2;
        code = code || instruction::FLE(temp, temp1, temp2);
    }
  } else if(ctx->LT()) {
    if(not Types.isFloatTy(t1) and not Types.isFloatTy(t2)) code = code || instruction::LT(temp, addr1, addr2);
    else {
        if(not Types.isFloatTy(t1)) code = code || instruction::FLOAT(temp1, addr1);
        else temp1 = addr1;
        if(not Types.isFloatTy(t2)) code = code || instruction::FLOAT(temp2, addr2);
        else temp2 = addr2;
        code = code || instruction::FLT(temp, temp1, temp2);
    }
  } else {
    if(not Types.isFloatTy(t1) and not Types.isFloatTy(t2)) code = code || instruction::EQ(temp, addr1, addr2);
    else {
        if(not Types.isFloatTy(t1)) code = code || instruction::FLOAT(temp1, addr1);
        else temp1 = addr1;
        if(not Types.isFloatTy(t2)) code = code || instruction::FLOAT(temp2, addr2);
        else temp2 = addr2;
        code = code || instruction::FEQ(temp, temp1, temp2);
    }
    code = code || instruction::NOT(temp, temp);
  }
  CodeAttribs codAts(temp, "", code);
  DEBUG_EXIT();
  return codAts;
}

antlrcpp::Any CodeGenVisitor::visitLogical(AslParser::LogicalContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs     && codAt1 = visit(ctx->expr(0));
  std::string         addr1 = codAt1.addr;
  instructionList &   code1 = codAt1.code;
  CodeAttribs     && codAt2 = visit(ctx->expr(1));
  std::string         addr2 = codAt2.addr;
  instructionList &   code2 = codAt2.code;
  instructionList &&   code = code1 || code2;
  // TypesMgr::TypeId t1 = getTypeDecor(ctx->expr(0));
  // TypesMgr::TypeId t2 = getTypeDecor(ctx->expr(1));
  //TypesMgr::TypeId  t = getTypeDecor(ctx);
  std::string temp = "%"+codeCounters.newTEMP();
  if(ctx->AND()) code = code || instruction::AND(temp, addr1, addr2);
  else code = code || instruction::OR(temp, addr1, addr2);
  CodeAttribs codAts(temp, "", code);
  DEBUG_EXIT();
  return codAts;
}
  
antlrcpp::Any CodeGenVisitor::visitValue(AslParser::ValueContext *ctx) {
  DEBUG_ENTER();
  instructionList code;
  //TypesMgr::TypeId  t = getTypeDecor(ctx);
  std::string temp = "%"+codeCounters.newTEMP();
  if(ctx->FLOATVAL()) code = instruction::FLOAD(temp, ctx->getText());
  else if(ctx->CHARVAL()) code = instruction::LOAD(temp, ctx->getText());
  else if(ctx->INTVAL()) code = instruction::ILOAD(temp, ctx->getText());
  else if(ctx->TRUE()) code = instruction::ILOAD(temp, "1");
  else code = instruction::ILOAD(temp, "0");
  CodeAttribs codAts(temp, "", code);
  DEBUG_EXIT();
  return codAts;
}

antlrcpp::Any CodeGenVisitor::visitExprFunc(AslParser::ExprFuncContext *ctx) {
  DEBUG_ENTER();
  instructionList code = instruction::PUSH("");
  if (ctx->list_expr()) {
    TypesMgr::TypeId tid = getTypeDecor(ctx->ident());
    std::vector<TypesMgr::TypeId> params =  Types.getFuncParamsTypes(tid);
    for (unsigned int i = 0;i < ctx->list_expr()->expr().size(); i++) {
        TypesMgr::TypeId tidp = getTypeDecor(ctx->list_expr()->expr(i));
        CodeAttribs     && codAt = visit(ctx->list_expr()->expr(i));
        instructionList &   exprCode = codAt.code;
        std::string         addr = codAt.addr;  
        code = code || exprCode;
//         if (Types.isFloatTy(params[i]) && Types.isIntegerTy(tidp) && not Types.isArrayTy(params[i])) {
        if (Types.isFloatTy(params[i]) && Types.isIntegerTy(tidp)) {
            std::string temp = "%"+codeCounters.newTEMP();
            code = code || instruction::FLOAT(temp,addr) || instruction::PUSH(temp);
        } else if (Types.isArrayTy(params[i])) {
            std::string name = ctx->list_expr()->expr(i)->getText();
            bool isParam = Symbols.isParameterClass(name);
            std::string temp = "%"+codeCounters.newTEMP();
            if (isParam) {
                code = code || instruction::LOAD(temp, addr);
            } else code = code || instruction::ALOAD(temp, addr);
            code = code || instruction::PUSH(temp);
        } else code = code || instruction::PUSH(addr);
    }
    code = code || instruction::CALL(ctx->ident()->getText());
    for (unsigned int i = 0;i < ctx->list_expr()->expr().size(); i++) code = code || instruction::POP("");
  } else {
      code = code || instruction::CALL(ctx->ident()->getText());
  }
  std::string temp = "%"+codeCounters.newTEMP();
  code = code || instruction::POP(temp);
  CodeAttribs codAts(temp, "", code);
  DEBUG_EXIT();
  return codAts;
}

antlrcpp::Any CodeGenVisitor::visitExprIdent(AslParser::ExprIdentContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs && codAts = visit(ctx->ident());
  DEBUG_EXIT();
  return codAts;
}

antlrcpp::Any CodeGenVisitor::visitParens(AslParser::ParensContext *ctx) {
  DEBUG_ENTER();
  CodeAttribs     && codAt = visit(ctx->expr());
  instructionList &   code = codAt.code;
  std::string         addr = codAt.addr;
//   std::string temp = "%"+codeCounters.newTEMP();
  CodeAttribs codAts(addr, "", code);
  DEBUG_EXIT();
  return codAts;
}

antlrcpp::Any CodeGenVisitor::visitIdent(AslParser::IdentContext *ctx) {
  DEBUG_ENTER();
  instructionList code;
  TypesMgr::TypeId tid = getTypeDecor(ctx);
  std::string name = ctx->ID()->getText();
  bool isParam = Symbols.isParameterClass(name);
  
  if (isParam && Types.isArrayTy(tid)) {
      std::string temp = "%"+codeCounters.newTEMP();
      code = instruction::LOAD(temp, name);
      name = temp;
  } else code = instructionList();
  CodeAttribs codAts(name, "", code);
  DEBUG_EXIT();
  return codAts;
}


// Getters for the necessary tree node atributes:
//   Scope and Type
SymTable::ScopeId CodeGenVisitor::getScopeDecor(antlr4::ParserRuleContext *ctx) const {
  return Decorations.getScope(ctx);
}
TypesMgr::TypeId CodeGenVisitor::getTypeDecor(antlr4::ParserRuleContext *ctx) const {
  return Decorations.getType(ctx);
}


// Constructors of the class CodeAttribs:
//
CodeGenVisitor::CodeAttribs::CodeAttribs(const std::string & addr,
                                         const std::string & offs,
                                         instructionList & code) :
  addr{addr}, offs{offs}, code{code} {
}

CodeGenVisitor::CodeAttribs::CodeAttribs(const std::string & addr,
                                         const std::string & offs,
                                         instructionList && code) :
  addr{addr}, offs{offs}, code{code} {
}
