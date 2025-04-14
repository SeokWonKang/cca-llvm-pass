%{

static CCAPatternGraph* _G = nullptr;
static int yyerror(const char* s);

%}

%token REGISTER NUMBER ERROR
%token EQ NE LT LE GT GE 

%type <tok> REGISTER NUMBER
%type <node> expr0 expr1 expr2 expr3
%type <compare> condition
%type <subgraph> assignment
%type <subgraphvector> assignment_list

%%

program :  NUMBER ':' assignment_list { _G = new CCAPatternGraph(std::atoi($1.text_.c_str()), $3); }
		;

assignment_list : assignment_list ';' assignment { $$ = $1; $$.push_back($3); }
				| assignment { $$.push_back($1); }
				;

assignment : REGISTER '=' expr0 { $$ = new CCAPatternSubGraph($1.text_, $3); }
		   ;

expr0 : condition '?' expr1 ':' expr1 { $$ = new CCAPatternGraphSelectNode($1, $3, $5); } 
	  | expr1 { $$ = $1; }
	  ;

condition : expr0 EQ expr0 { $$ = new CCAPatternGraphCompareNode("==", $1, $3); }
		  | expr0 NE expr0 { $$ = new CCAPatternGraphCompareNode("!=", $1, $3); }
		  | expr0 LT expr0 { $$ = new CCAPatternGraphCompareNode("<", $1, $3); }
		  | expr0 LE expr0 { $$ = new CCAPatternGraphCompareNode("<=", $1, $3); }
		  | expr0 GT expr0 { $$ = new CCAPatternGraphCompareNode(">", $1, $3); }
		  | expr0 GE expr0 { $$ = new CCAPatternGraphCompareNode(">=", $1, $3); }
		  ;

expr1 : expr1 '+' expr2 { $$ = new CCAPatternGraphOperatorNode("+", $1, $3); }
	  | expr1 '-' expr2 { $$ = new CCAPatternGraphOperatorNode("-", $1, $3); }
	  | expr2 { $$ = $1; }
	  ;

expr2 : expr2 '*' expr3 { $$ = new CCAPatternGraphOperatorNode("*", $1, $3); }
	  | expr2 '/' expr3 { $$ = new CCAPatternGraphOperatorNode("/", $1, $3); }
	  | expr3 { $$ = $1; }

expr3 : '(' expr0 ')' { $$ = $2; }
	   | REGISTER { $$ = new CCAPatternGraphRegisterNode($1.text_); }
	   ;

%%

// Internal Functions
int yyerror(const char* s) { 
	std::cerr << "[PASS] Error in Parser : " << s << '\n';
	std::cerr << "  - Last Token : " << yylval.tok.text_ << std::endl;
	return 0; 
}

typedef struct yy_buffer_state * YY_BUFFER_STATE;
extern YY_BUFFER_STATE yy_scan_string(const char * str);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer);
// External Interfaces
CCAPatternGraph *parsePatternStr(std::string patternStr) {
	YY_BUFFER_STATE buffer = yy_scan_string(patternStr.c_str());
	// setPatternStr(patternStr);
	_G = nullptr;
	yyparse();
	yy_delete_buffer(buffer);
	return _G;
}

