%option noyywrap

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
