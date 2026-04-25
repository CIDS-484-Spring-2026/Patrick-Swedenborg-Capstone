%code requires {
  #ifndef YY_TYPEDEF_YYSCAN_T
  #define YY_TYPEDEF_YYSCAN_T
  typedef void* yyscan_t;
  #endif
  #include "logic.h"
}

%{

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "logic.h"
#include "parser.tab.h"
#include <stdbool.h>
#include "scanner.h" 

void yyerror(void *scanner, const char *msg);
int yylex(YYSTYPE *yylval_param, void *yyscanner);

%}

%define api.pure full
%lex-param {yyscan_t scanner}
%parse-param {yyscan_t scanner}



%union { char *word; char *str; };
//%token CONSTANT
%type <str> commands
%token <word> WORD
// %token PIPE REDIRECT_IN REDIRECT_OUT APPEND
//%token END

%start input

%%
input:
     commands 
	{
		struct parser_ctx *extra = yyget_extra(scanner);
		if (extra->argc == 0) {
			YYACCEPT;
		} else {
		call(extra);
		}
	};

commands:
	{ $$ = NULL; } 
	| commands WORD	 
	{
		struct parser_ctx *extra = yyget_extra(scanner);
		extra->argv[extra->argc] = strdup($2);
		extra->argc++;
	}

%%

void yyerror(void *scanner, const char *msg) {
    fprintf(stderr, "Error at line : %s\n",  msg);
}
