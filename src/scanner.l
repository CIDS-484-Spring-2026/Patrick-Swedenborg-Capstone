%{
#include "parser.tab.h"
#include <string.h>
%}

%%

"|"		{return PIPE; }
">"		{return REDIRECT_OUT; }
"<"		{return REDIRECT_IN; }
">>"		{return APPEND; }
[a-zA-Z0-9]+ 	{yylval.str = strdup(yytext); return WORD; }
[ \t\n]+     	{ /* ignore whitespace */ }
\n 		{yywrap();}
.		{return yytext[0]; }

%%

int yywrap(){ return 1; }
