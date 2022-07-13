//////////////////////////////////////////////////////////////////////
//
//    Asl - Another simple language (grammar)
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

grammar Asl;

//////////////////////////////////////////////////
/// Parser Rules
//////////////////////////////////////////////////

// A program is a list of functions
program : function+ EOF
        ;

// A function has a name, a list of parameters and a list of statements
function
        : FUNC ID '(' (parameters)? ')' (':' type)? declarations statements ENDFUNC
        ;

declarations
        : (variable_decl)*
        ;

variable_decl
        : VAR ID ( ',' ID)* ':' type
        ;

parameters
        : ID ':' type (',' ID ':' type)*
        ;

type
        : basic_type
        | array_type
        ;

basic_type
        : INT
        | FLOAT
        | BOOL
        | CHAR
        ;

array_type
        : ARRAY '[' INTVAL ']' OF basic_type
        ;

statements
        : (statement)*
        ;

// The different types of instructions
statement
          // Assignment
        : left_expr ASSIGN expr ';'                                 # assignStmt
          // if-then-else statement (else is optional)
        //| IF expr THEN statements else_opt? ENDIF       # ifStmt
        | IF expr THEN statements (ELSE statements)? ENDIF          # ifStmt
          // while-do statement
        | WHILE expr DO statements ENDWHILE                         # whileStmt
          // A function/procedure call has a list of arguments in parenthesis (possibly empty)
        | ident '(' (list_expr)? ')' ';'                            # procCall
          // Read a variable
        | READ left_expr ';'                                        # readStmt
          // Write an expression
        | WRITE expr ';'                                            # writeExpr
          // Write a string
        | WRITE STRING ';'                                          # writeString
          // May return something
        | RETURN (expr)? ';'    		                            # returnStmt
        ;

// Grammar for left expressions (l-values in C++)
left_expr
        : ident ('[' expr ']')?
        ;

// Grammar for expressions with boolean, relational and aritmetic operators
expr    : ident '[' expr ']'	                        # arrayAccess
        | op=(NOT|PLUS|NEG) expr        		        # unaryOps
        | expr op=(MUL|DIV|MOD) expr                    # arithmetic
        | expr op=(PLUS|NEG) expr                       # arithmetic
        | expr op=(EQUAL|NEQUAL|GT|GEQ|LT|LEQ) expr     # relational
        | expr op=AND expr      		                # logical
        | expr op=OR expr       		                # logical
        | (INTVAL|FLOATVAL|TRUE|FALSE|CHARVAL)          # value
        | ident '(' (list_expr)? ')'                    # exprFunc
        | ident                                         # exprIdent
        | '(' expr ')'      			                # parens
        ;

// Identifiers
ident   : ID
        ;
// List of identifiers
list_expr
        : expr (',' expr)*
        ;

//////////////////////////////////////////////////
/// Lexer Rules
//////////////////////////////////////////////////

ASSIGN    : '=' ;
EQUAL     : '==' ;
NEQUAL	  : '!=';
LT  	  : '<' ;
LEQ 	  : '<=';
GT  	  : '>' ;
GEQ 	  : '>=';
AND 	  : 'and';
OR  	  : 'or';
NOT 	  : 'not';
PLUS      : '+';
NEG 	  : '-';
MUL       : '*';
DIV 	  : '/';
MOD 	  : '%';
VAR       : 'var';
ARRAY	  : 'array';
OF  	  : 'of';
INT       : 'int';
FLOAT	  : 'float';
BOOL	  : 'bool' ;
CHAR	  : 'char' ;
TRUE	  : 'true';
FALSE	  : 'false';
IF        : 'if' ;
THEN      : 'then' ;
ELSE      : 'else' ;
ENDIF     : 'endif' ;
WHILE	  : 'while';
DO  	  : 'do';
ENDWHILE  : 'endwhile';
RETURN	  : 'return';
FUNC      : 'func' ;
ENDFUNC   : 'endfunc' ;
READ      : 'read' ;
WRITE     : 'write' ;
ID        : ('a'..'z'|'A'..'Z') ('a'..'z'|'A'..'Z'|'_'|'0'..'9')* ;
INTVAL    : ('0'..'9')+ ;
FLOATVAL  : ('0'..'9')+ '.' ('0'..'9')+;
CHARVAL   : '\'' (' ' .. '~'| '\\n' | '\\t' | '\\\'' )? '\'';

// Strings (in quotes) with escape sequences
STRING    : '"' ( ESC_SEQ | ~('\\'|'"') )* '"' ;

fragment
ESC_SEQ   : '\\' ('b'|'t'|'n'|'f'|'r'|'"'|'\''|'\\') ;

// Comments (inline C++-style)
COMMENT   : '//' ~('\n'|'\r')* '\r'? '\n' -> skip ;

// White spaces
WS        : (' '|'\t'|'\r'|'\n')+ -> skip ;
// Alternative description
// WS        : [ \t\r\n]+ -> skip ;
