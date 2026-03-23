
%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "logic.h"
#include "parser.tab.h" 

extern int yylex();
extern FILE *yyin;
extern YY_FLUSH_BUFFER;
void yyerror(const char *s);

char *argv[256][64];
int argc = 0;

%}

%union { 
	char *str;
	 
}
%type <str> commands
%token <str> WORD
%token PIPE REDIRECT_IN REDIRECT_OUT APPEND NEWLINE
%token END

%start input

%%
input:
     commands {
		for(int i = 0; i < argc; i++) {
		printf("%s ", (argv[i]));
	}
	YY_FLUSH_BUFFER;
}
;

commands:
	  /* BASE CASE: Empty */
	  {
	  	$$ = NULL;
	  }
	| commands WORD 
	  {
		strcpy(argv[argc], $2);
		argc++;		
	  }


%%

void yyerror(const char *s) {
	fprintf(stderr, "Error: %s\n", s);
}
