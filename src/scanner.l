%top{
  #include "logic.h"  // Header containing struct parser_ctx
}
%{
#include "parser.tab.h"
#define YYPARSE_PARAM yyscan_t scanner
#define YYLEX_PARAM scanner
#include <string.h>


#define YY_EXTRA_TYPE struct parser_ctx *
%}

%option interactive
%option noyywrap
%option reentrant
%option bison-bridge
%option header-file="scanner.h"
%option extra-type="struct parser_ctx *"

%%
[a-zA-Z0-9`~!@#$%^&*;:'",.\-_=+\(\)\[\]\{\}\\\/?|]+ 	{ yylval->word = strdup(yytext); return WORD; }
[ \t]+     			{ /* ignore whitespace */ }
"\n" 				{ return 0; }
.				{ return yytext[0]; }

%%


