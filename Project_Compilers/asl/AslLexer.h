
// Generated from Asl.g4 by ANTLR 4.7.2

#pragma once


#include "antlr4-runtime.h"




class  AslLexer : public antlr4::Lexer {
public:
  enum {
    T__0 = 1, T__1 = 2, T__2 = 3, T__3 = 4, T__4 = 5, T__5 = 6, T__6 = 7, 
    ASSIGN = 8, EQUAL = 9, NEQUAL = 10, LT = 11, LEQ = 12, GT = 13, GEQ = 14, 
    AND = 15, OR = 16, NOT = 17, PLUS = 18, NEG = 19, MUL = 20, DIV = 21, 
    MOD = 22, VAR = 23, ARRAY = 24, OF = 25, INT = 26, FLOAT = 27, BOOL = 28, 
    CHAR = 29, TRUE = 30, FALSE = 31, IF = 32, THEN = 33, ELSE = 34, ENDIF = 35, 
    WHILE = 36, DO = 37, ENDWHILE = 38, RETURN = 39, FUNC = 40, ENDFUNC = 41, 
    READ = 42, WRITE = 43, ID = 44, INTVAL = 45, FLOATVAL = 46, CHARVAL = 47, 
    STRING = 48, COMMENT = 49, WS = 50
  };

  AslLexer(antlr4::CharStream *input);
  ~AslLexer();

  virtual std::string getGrammarFileName() const override;
  virtual const std::vector<std::string>& getRuleNames() const override;

  virtual const std::vector<std::string>& getChannelNames() const override;
  virtual const std::vector<std::string>& getModeNames() const override;
  virtual const std::vector<std::string>& getTokenNames() const override; // deprecated, use vocabulary instead
  virtual antlr4::dfa::Vocabulary& getVocabulary() const override;

  virtual const std::vector<uint16_t> getSerializedATN() const override;
  virtual const antlr4::atn::ATN& getATN() const override;

private:
  static std::vector<antlr4::dfa::DFA> _decisionToDFA;
  static antlr4::atn::PredictionContextCache _sharedContextCache;
  static std::vector<std::string> _ruleNames;
  static std::vector<std::string> _tokenNames;
  static std::vector<std::string> _channelNames;
  static std::vector<std::string> _modeNames;

  static std::vector<std::string> _literalNames;
  static std::vector<std::string> _symbolicNames;
  static antlr4::dfa::Vocabulary _vocabulary;
  static antlr4::atn::ATN _atn;
  static std::vector<uint16_t> _serializedATN;


  // Individual action functions triggered by action() above.

  // Individual semantic predicate functions triggered by sempred() above.

  struct Initializer {
    Initializer();
  };
  static Initializer _init;
};

