#include <stdio.h>
#include <stdlib.h>
#include "parser.tab.h"
#include <unistd.h>
#include <string.h>

extern FILE *yyin;


int main(void) {
	// Initialize variables
	char cwd[1024]; // Current Working Directory
	
	// Loop which runs the shell
	while (1) {
		
	if (getcwd(cwd, sizeof(cwd)) != NULL) {
		printf("%s%s", cwd, "$ ");
		}
	int n = yyparse();
	printf("yyparse returned %d\n", n);
	return EXIT_SUCCESS;
	}
	
}
