#ifndef STRINGPARSER_PARSER_HPP_
#define STRINGPARSER_PARSER_HPP_

#include "Instrumentation/CCAPatternGraph.hpp"
#include <string>
#include <vector>
#ifndef FLEX_LEXER
	#define FLEX_LEXER
	#include <FlexLexer.h>
#endif

namespace llvm {
namespace cca {
namespace parser {

//--------------------------------------------------
// External Interfaces
//--------------------------------------------------
CCAPatternGraph *parsePatternStr(std::string patternStr);

//--------------------------------------------------
// Internal Structure /  Functions for Lex and Yacc
//--------------------------------------------------
// Token Structure
struct Token {
	int type_;
	std::string text_;
	Token() : type_(0), text_() {}
	Token(int t) : type_(t), text_() {}
	Token(int t, std::string s) : type_(t), text_(s) {}
	operator int() const { return type_; }
};

// YYSTYPE
struct _YYSTYPE {
	llvm::cca::parser::Token tok;
	llvm::cca::CCAPatternGraphNode *node;
	llvm::cca::CCAPatternSubGraph *subgraph;
	llvm::cca::CCAPatternGraphCompareNode *compare;
	std::vector<llvm::cca::CCAPatternSubGraph *> subgraphvector;
};
typedef struct _YYSTYPE YYSTYPE;
#define YYSTYPE_IS_DECLARED 1

// Scanner
class Scanner : public yyFlexLexer {
  public:
	Scanner(std::istream &is, std::ostream &os) : yyFlexLexer(is, os) {}
	Token getToken(void);
};

void setPatternStr(std::string patternStr);
int yylex(void);
int yyerror(const char *s);
Token lastTok(void);

}; // namespace parser
}; // namespace cca
}; // namespace llvm

#endif // STRINGPARSER_PARSER_HPP_
