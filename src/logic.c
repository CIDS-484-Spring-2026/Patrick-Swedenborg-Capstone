#include "logic.h"
#include "parser.tab.h"
#include "scanner.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>

/*
 * Basic loop which runs the shell
 */
int main(void) 
{
	// Initialize variables
	char cwd[1024]; 		// Current Working Directory
	struct parser_ctx pctx = {};	// Context Struct to extract data from parser
	pctx.argc = 0;
	
	// Loop which runs the shell
	while (1) {
		
	if (getcwd(cwd, sizeof(cwd)) != NULL) {
		printf("%s%s", cwd, "$ ");
		fflush(stdout);
	}
	yyscan_t scanner;		// Scanner which is required for a reentrant parser

	yylex_init(&scanner);		// Initialize scanner
	yyset_extra(&pctx, scanner);	// Attach the context struct to the Scanner so we can extract argv and argc from parser
	yyset_in(stdin, scanner);
	

	yyparse(scanner);		// Does the parsing
	/*
	 * These next three lines are cleanup for next iteration
	 */
	yyrestart(stdin, scanner);
	yylex_destroy(scanner);
	reset_context(&pctx);

	
	}
	return 0;
}

/*
 * Resets the context struct after each iteration of the loop
 */
void reset_context(struct parser_ctx *ctx)
{
	for (int i = 0; i < ctx->argc; i++)
	{
		free(ctx->argv[i]);
		ctx->argv[i] = NULL;
	}
	ctx->argc = 0;
}

/*
 * The function which detects command and executes them
 */
int call(struct parser_ctx *ctx) 
{
	if (strcmp(ctx->argv[0], "exit") == 0) {
		myexit(ctx);
	} else if (strcmp(ctx->argv[0], "cd") == 0) {
		cd(ctx);
	} else if (strcmp(ctx->argv[0], "pwd") == 0) {
		pwd(ctx);
	} else if (strcmp(ctx->argv[0], "echo") == 0) {
		echo(ctx);
	} else if (strcmp(ctx->argv[0], "export") == 0) {
		export(ctx);
	} else if (strcmp(ctx->argv[0], "help") == 0) {
		help();
	} else {
		pid_t pid = fork();

                if (pid < 0)
                {
                        perror("fork");
                        return EXIT_FAILURE;
                }

                if (pid == 0)
                {
                        if (execvp(ctx->argv[0], ctx->argv) < 0)
                        {
				perror("execvp");
                                exit(EXIT_FAILURE);
                        }
                        exit(0);
                }
                else
                {
                        int status = 0;
                        waitpid(pid, &status, 0);
                        return status;
                }
	}

}
/*
void checkBuiltins(struct parser_ctx *ctx)
{
	//TODO
}
*/

/*
 * This function will be refactored or deleted, probably unnecessary
 */
void myexit(struct parser_ctx *ctx)
{
	exit(EXIT_SUCCESS);
}

/*
 * This is currently not used, the idea was for a more advanced implementation of cd
 */
void update_cwd_env(struct parser_ctx *ctx)
{
	char cwd[1024];
	if (getcwd(cwd, sizeof(cwd)) != NULL)
	{
		char *old_pwd = getenv("PWD");
		if (old_pwd)
		{
			setenv("OLDPWD", old_pwd, 1);
		}
		setenv("PWD", cwd, 1);
	}
}

/*
 * Change directory command
 */
void cd(struct parser_ctx *ctx)
{
	if (ctx->argc < 2)
	{
		char *home = getenv("HOME");
		chdir(home);
	} else {
		if (chdir(ctx->argv[1]) != 0)
		{
			perror("cd");
		}
	}
}

/*
 * Print working directory command
 */
void pwd(struct parser_ctx *ctx)
{
	char cwd[1024];
	if (getcwd(cwd, sizeof(cwd)) != NULL)
	{
		printf("%s\n", cwd);
	} else {
		perror("pwd");
	}
}
/*
 * Echo command, may be unnecessary
 */
void echo(struct parser_ctx *ctx)
{
	for (int i = 1; i < ctx->argc; i++)
	{
		printf("%s%s", ctx->argv[i], (i == ctx->argc - 1) ? "" : " ");
	}
	printf("\n");
}

/*
 * Export command, may be unnecessary
 */
void export(struct parser_ctx *ctx)
{
	if (ctx->argc < 2) return;

	if (putenv(strdup(ctx->argv[1])) != 0)
	{
		perror("export");
	}
}

/*
 * Primitive help menu for now
 */
void help()
{
	printf("Help Page:\n");
	printf("Built ints\n");
	printf("cd PATH to change directory, or just cd to go to home\n");
	printf("pwd to print working directory\n");
	printf("echo [message] to print a message to the screen\n");
	printf("export [V] to set an environment variable\n");
	printf("exit to terminate the shell\n");
}
