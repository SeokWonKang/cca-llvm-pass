%{
#include <string>
#include <sstream>
/* lexeme of identifier or reserved word */
#define FLEX_LEXER
#include "parser.hpp"
#include "cca.tab.hpp"
#undef YY_DECL
#define YY_DECL llvm::cca::parser::Token llvm::cca::parser::Scanner::getToken(void)

%}

%option c++
%option noyywrap
%option yyclass="llvm::cca::parser::Scanner"

digit       [0-9]
number      {digit}+
letter      [a-zA-Z]
register    [iot]{number}
whitespace  [ \t]+

%%

":"          { return yylval.tok = {':', ":"}; }
";"          { return yylval.tok = {';', ";"}; }
"?"          { return yylval.tok = {'?', "?"}; }
"("          { return yylval.tok = {'(', "("}; }
")"          { return yylval.tok = {')', ")"}; }
"+"          { return yylval.tok = {'+', "+"}; }
"-"          { return yylval.tok = {'-', "-"}; }
"*"          { return yylval.tok = {'*', "*"}; }
"/"          { return yylval.tok = {'/', "/"}; }
"="          { return yylval.tok = {'=', "="}; }
"=="         { return yylval.tok = {EQ, "=="};}
"!="         { return yylval.tok = {NE, "!="};}
"<"          { return yylval.tok = {LT, "<"};}
"<="         { return yylval.tok = {LE, "<="};}
">"          { return yylval.tok = {GT, ">"};}
">="         { return yylval.tok = {GE, ">="};}
{register}   { return yylval.tok = {REGISTER, std::string(yytext)}; }
{number}     { return yylval.tok = {NUMBER, std::string(yytext)}; }
{whitespace} { /* skip whitespace */}
.            { return yylval.tok = {ERROR, std::string(yytext)}; }

%%

namespace llvm {
namespace cca {
namespace parser {

static std::istringstream _patternStrSS;
static Scanner _scanner(_patternStrSS, std::cerr);
static Token _lastTok(ERROR);

void setPatternStr(std::string str) {
	_patternStrSS.str(str);
	_patternStrSS.clear();
}

int yylex(void) { 
	_lastTok = _scanner.getToken(); 
	return _lastTok;
}

Token lastTok(void) {
	return _lastTok;
}

} // namespace parser
} // namespace cca
} // namespace llvm
