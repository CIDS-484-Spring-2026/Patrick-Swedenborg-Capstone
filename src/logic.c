#include "logic.h"
#include "parser.tab.h"
#include "scanner.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/stat.h>
#include <glob.h>

extern YY_BUFFER_STATE yy_scan_string(const char *str, yyscan_t scanner);
extern void yy_delete_buffer(YY_BUFFER_STATE buffer, yyscan_t scanner);

#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_RESET   "\x1b[0m"

// Global variables which track variables which multiple functions use.
int last_exit_status = 0;
char last_command[1024] = {0};


/*
 * Basic loop which runs the shell
 */
int main(void) 
{
	if (read_history(".shell_history") != 0);
	{
		int fd = open(".shell_history", O_RDWR | O_CREAT, 0600);
		if (fd != -1) close(fd);
	}
	init_signals();
	rl_catch_signals = 0;
	
	// Initialize variables
	struct parser_ctx pctx = {};	// Context Struct to extract data from parser
	pctx.argc = 0;
	yyscan_t scanner;
	
	read_history(".shell_history");
	// Loop which runs the shell
	while (1) 
	{
	char *current_prompt = get_prompt();	
	char *line = readline(current_prompt);

	if (line == NULL)
	{
		printf("\nexit\n");
		break;
	}

	if (*line)
	{
		strncpy(last_command, line, 1023);

		add_history(line);
		write_history(".shell_history");

		//yylex_init(&scanner);
		//yyset_extra(&pctx, scanner);

		//YY_BUFFER_STATE buffer = yy_scan_string(line, scanner);

		//yyparse(scanner);

		//yy_delete_buffer(buffer, scanner);
		//yylex_destroy(scanner);
		execute_line(line, &pctx);
		reset_context(&pctx);
	}
	free(line);

	/*print_prompt();
	yyscan_t scanner;		// Scanner which is required for a reentrant parser

	yylex_init(&scanner);		// Initialize scanner
	yyset_extra(&pctx, scanner);	// Attach the context struct to the Scanner so we can extract argv and argc from parser
	yyset_in(stdin, scanner);
	

	yyparse(scanner);		// Does the parsing
	*
	 * These next three lines are cleanup for next iteration
	 *
	yyrestart(stdin, scanner);
	yylex_destroy(scanner);
	reset_context(&pctx);

	*/
	}
	return 0;
}


void execute_line(char *line, struct parser_ctx *pctx)
{
	if (!line || !*line) return;

	yyscan_t scanner;
	yylex_init(&scanner);
	yyset_extra(pctx, scanner);

	YY_BUFFER_STATE buffer = yy_scan_string(line, scanner);

	yyparse(scanner);

	yy_delete_buffer(buffer, scanner);
	yylex_destroy(scanner);
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
	ctx->dynamically_allocated = 0;
}

/*
 * The function which detects command and executes them
 */
int call(struct parser_ctx *ctx) 
{
	expand_variables(ctx);
	expand_wildcards(ctx);

	int is_background = 0;
	if (ctx->argc > 0 && strcmp(ctx->argv[ctx->argc - 1], "&") == 0)
	{
		is_background = 1;

		free(ctx->argv[ctx->argc - 1]);
		ctx->argv[ctx->argc - 1] = NULL;
		ctx->argc--;
	}
	// Check for pipes
	for (int i = 0; i < ctx->argc; i++) {
		if (strcmp(ctx->argv[i], "|") == 0) {
			pipeline(ctx);
			return 0;
		}
	}

	if (strcmp(ctx->argv[0], "exit") == 0) {
		myexit(ctx);
	} else if (strcmp(ctx->argv[0], "cd") == 0) {
		cd(ctx);
	} else if (strcmp(ctx->argv[0], "pwd") == 0) {
		pwd(ctx);
	} else if (strcmp(ctx->argv[0], "export") == 0) {
		export(ctx);
	} else if (strcmp(ctx->argv[0], "help") == 0) {
		help();
	} else if (strcmp(ctx->argv[0], "fix") == 0) {
		if (last_exit_status == 0)
		{
			printf("\033[1;32mLast command was successful. Nothing to fix!\033[0m\n");
			return 0;
		}

		char sys_cmd[4096];
		snprintf(sys_cmd, sizeof(sys_cmd),
"ollama run qwen2.5-coder:3b \"The command '%s' failed with exit code %d. "
             "Explain why it failed in one sentence and provide the corrected command.\"", 
             last_command, last_exit_status);
		printf("\033[1;33mAI Debugging...\033[0m\n");
    		system(sys_cmd);
    		return 0;
	} else if (strcmp(ctx->argv[0], "ai") == 0 && ctx->argc > 1 && strcmp(ctx->argv[1], "script") == 0) {
		
		char user_prompt[2048] = {0};
		char sys_cmd[4096] = {0};

		for (int i = 2; i < ctx->argc; i++)
		{
			strcat(user_prompt, ctx->argv[i]);
			strcat(user_prompt, " ");
		}


		snprintf(sys_cmd, sizeof(sys_cmd),
"ollama run qwen2.5-coder:3b \"Output ONLY the raw bash code for this request: %s. "
         "Absolutely NO introductory text, NO explanations, and NO backticks. "
         "Start immediately with #!/bin/bash.\"",
			user_prompt);

		printf("\033[1;30mGenerating script...\033[0m\n");

		FILE *fp = popen(sys_cmd, "r");
		FILE *script_file = fopen("ai_generated.sh", "w");

		char line[1024];
		printf("\n--- GENERATED SCRIPT ---\n\033[0;36m");
		while(fgets(line, sizeof(line), fp))
		{
			fputs(line, script_file);
			printf("%s", line);
		}
		printf("\033[0m------------------------\n");

		pclose(fp);
		fclose(script_file);
		
		printf("Execute this script? (y/n): ");
		char choice = getchar();
		while (getchar() != '\n');
		
		if (choice == 'y' || choice == 'Y')
		{
			chmod("ai_generated.sh", 0755);
			system("./ai_generated.sh");
			last_exit_status = 0;
		}
		return 0;
	} else if (strcmp(ctx->argv[0], "ai") ==  0) {
		if (ctx->argc < 2)
		{
			printf("Usage: ai <prompt>\n");
			return 0;
		}

		char user_prompt[1024] = {0};
		for  (int i = 1; i < ctx->argc; i++)
		{
			strcat(user_prompt, ctx->argv[i]);
			strcat(user_prompt, " ");
		}

		char sys_cmd[2048] = {0};
		snprintf(sys_cmd, sizeof(sys_cmd),
			"ollama run qwen2.5-coder:3b \"You are a linux expert. Provide ONLY the command-line syntax for the following request. No explanation, no backticks. Request: %s\"",
			user_prompt);

		FILE *fp = popen(sys_cmd, "r");
		if (fp == NULL)
		{
			perror("open");
			return 1;
		}

		char ai_suggestion[1024] = {0};
		if (fgets(ai_suggestion, sizeof(ai_suggestion), fp) != NULL)
		{
			ai_suggestion[strcspn(ai_suggestion, "\r\n")] = 0;

			printf("\033[1;35mAI Suggestion:\033[0m %s\n", ai_suggestion);
			printf("Execute this command? (y/n): ");

			char choice = getchar();
			while (getchar() != '\n');

			if (choice == 'y' || choice == 'Y')
			{
				printf("Executing...\n");
				int result = system(ai_suggestion);
				last_exit_status = WEXITSTATUS(result);
			}
		}
		pclose(fp);
		return 0;
	} else {
		pid_t pid = fork();

                if (pid < 0)
                {
                        perror("fork");
                        return EXIT_FAILURE;
                }

                if (pid == 0)
                {
			struct sigaction sa_default;
			sa_default.sa_handler = SIG_DFL;
			sigemptyset(&sa_default.sa_mask);
			sa_default.sa_flags = 0;


			sigaction(SIGINT, &sa_default, NULL);
			sigaction(SIGTSTP, &sa_default, NULL);
			sigaction(SIGQUIT, &sa_default, NULL);

			int truncate_at = -1;
			
			// This loop handles file redirection
			for (int i = 0; i < ctx->argc; i++)
			{
				if (ctx->argv[i] == NULL) break;

				if (strcmp(ctx->argv[i], ">") == 0)
				{
					int fd = open(ctx->argv[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
					if (fd < 0) { perror("open >"); exit(EXIT_FAILURE); }
					dup2(fd, STDOUT_FILENO);
					close(fd);
					if (truncate_at == -1) truncate_at = i;
					//ctx->argv[i] = NULL;
					i++;
				} 
				else if (strcmp(ctx->argv[i], ">>") == 0)
				{
					int fd = open(ctx->argv[i+1], O_WRONLY | O_CREAT | O_APPEND, 0644);
					if (fd < 0) { perror("open >>"); exit(EXIT_FAILURE); }
					dup2(fd, STDOUT_FILENO);
					close(fd);
					if (truncate_at == -1) truncate_at = i;
					//ctx->argv[i] = NULL;
					i++;
				}
				else if (strcmp(ctx->argv[i], "<") == 0)
				{
					int fd = open(ctx->argv[i+1], O_RDONLY);
					if (fd < 0) { perror("open <"); exit(EXIT_FAILURE); }
					dup2(fd, STDIN_FILENO);
					close(fd);
					if (truncate_at == -1) truncate_at = i;
					//ctx->argv[i] = NULL;
					i++;
				}
				else if (strcmp(ctx->argv[i], "2>") == 0)
				{
					int fd = open(ctx->argv[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
					if (fd < 0)
					{
						perror("open 2>");
						exit(1);
					}
					dup2(fd, STDERR_FILENO);
					close(fd);
					if (truncate_at == -1) truncate_at = i;
					//ctx->argv[i] = NULL;
					i++;
				}
			}
			if (truncate_at != -1) { ctx->argv[truncate_at] = NULL; }


			if (execvp(ctx->argv[0], ctx->argv) < 0)
                        {
				if (errno = ENOENT) { fprintf(stderr, "%s: command not found\n", ctx->argv[0]); }
				else if (errno = EACCES) { fprintf(stderr, "%s: permission denied\n", ctx->argv[0]); }
				else { perror("execvp"); }
                                exit(127);
                        }
                        exit(0);
                }
                else
                {
			if (ctx->pipeline == 1 || is_background) 
			{
				if (is_background)
				{
					printf("[Process running in background: PID %d]\n", pid);
					fflush(stdout);
				}
				return 0; 
			}
			else 
			{
                        int status = 0;
                        waitpid(pid, &status, 0);

			if (WIFEXITED(status))
			{
				last_exit_status = WEXITSTATUS(status);
			}
			else if (WIFSIGNALED(status))
			{
				last_exit_status = 128 + WTERMSIG(status);
			}
			return last_exit_status;
			}
			return 0;
		}
	}

}

void pipeline(struct parser_ctx *ctx) 
{
	// Variables and Arrays
	int total_length = 0;
	for (int i = 0; i < ctx->argc; i++) {
		total_length += strlen(ctx->argv[i]) + 1;
	}
	char *input = malloc(total_length);
	input[0] = '\0';

	for (int i = 0; i < ctx->argc; i++) {
		strcat(input, ctx->argv[i]);
		if (i < ctx->argc - 1) strcat(input, " ");
	}


	char *cmds[1024];
	int i = 0;
	cmds[i] = strtok(input, "|");
	while (cmds[i] != NULL)
                {
                        i++;
                        cmds[i] = strtok(NULL, "|");
                }

	int stdin_bak = dup(STDIN_FILENO);
	int stdout_bak = dup(STDOUT_FILENO);

	i = 0;
	while (cmds[i + 1] != NULL)
	{
		char* argv[1000];
		int j = 0;
		argv[0] = strtok(cmds[i], " ");
		while (argv[j] != NULL)
		{
			j++;
			argv[j] = strtok(NULL, " ");
		}
		
		int pipefd[2];
		if (pipe(pipefd) < 0)
		{
			perror("pipe");
			exit(EXIT_FAILURE);
		}
		struct parser_ctx pctx_local = {0};
        	pctx_local.argc = j;
		for (int k = 0; k < j; k++) {
			pctx_local.argv[k] = argv[k];
		}
		pctx_local.argv[j] = NULL;
		pctx_local.pipeline = 1;

		// Piping
		dup2(pipefd[1], STDOUT_FILENO);
		call(&pctx_local);
		close(pipefd[1]);
		dup2(pipefd[0], STDIN_FILENO);
		close(pipefd[0]);
		i++;
	}
	dup2(stdout_bak, STDOUT_FILENO);
	
	
	char* argv[1000];
	int j = 0;
	argv[0] = strtok(cmds[i], " ");
	while (argv[j] != NULL)
	{
		j++;
		argv[j] = strtok(NULL, " ");
	}
	struct parser_ctx pctx_local = {0};
	pctx_local.argc = j;
	for (int i = 0; i < j; i++) {
		pctx_local.argv[i] = argv[i];
	}
	pctx_local.argv[j] = NULL;
	pctx_local.pipeline = 1;
	
	
	call(&pctx_local);
	while (wait(NULL) > 0);
	
	dup2(stdin_bak, STDIN_FILENO);
	close(stdin_bak);
	close(stdout_bak);
	free(input); 
}


void signalchild_handler(int sig)
{
	int saved_errno = errno;
	int status;
	pid_t pid;

	while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
	{
		if (WIFEXITED(status))
		{
			char msg[128];
			int len = snprintf(msg, sizeof(msg), "\n[Process %d finished with exit code %d]\n$", pid, WEXITSTATUS(status));
			if (len > 0) { write(STDOUT_FILENO, msg, len); }
		}
	}
	errno = saved_errno;
}

void init_signals()
{
	struct sigaction sa_ignore, sa_child;

	sa_ignore.sa_handler = SIG_IGN;
	sigemptyset(&sa_ignore.sa_mask);
	sa_ignore.sa_flags = 0;

	sigaction(SIGINT, &sa_ignore, NULL);	// Ignore CTRL+C
	sigaction(SIGTSTP, &sa_ignore, NULL);	// Ignore CTRL+Z
	sigaction(SIGQUIT, &sa_ignore, NULL);	// Ignore CTRL+\
	

	sa_child.sa_handler = signalchild_handler;
	sigemptyset(&sa_child.sa_mask);
	sa_child.sa_flags = SA_RESTART | SA_NOCLDSTOP;


	sigaction(SIGCHLD, &sa_child, NULL);
}

void expand_variables(struct parser_ctx *ctx)
{
	for (int i = 0; i < ctx->argc; i++)
	{
		if (ctx->argv[i] == NULL) continue;


		if (strcmp(ctx->argv[i], "$?") == 0)
		{
			char buf[16];
			snprintf(buf, sizeof(buf), "%d", last_exit_status);
			free(ctx->argv[i]);
			ctx->argv[i] = strdup(buf);
		}

		else if (ctx->argv[i][0] == '$' )
	       	{
			char *var_name = &ctx->argv[i][1];
			char *value = getenv(var_name);

			if (value != NULL)
			{
				free(ctx->argv[i]);
				ctx->argv[i] = strdup(value);
			}
			else
			{
				free(ctx->argv[i]);
				ctx->argv[i] = strdup("");
			}
		}
	}
}

void expand_wildcards(struct parser_ctx *ctx)
{
	for (int i = 0; i < ctx->argc; i++)
	{
		if (ctx->argv[i] && (strchr(ctx->argv[i], '*') || strchr(ctx->argv[i], '?')))
		{
			glob_t gstruct;

			if (glob(ctx->argv[i], GLOB_NOCHECK | GLOB_TILDE, NULL, &gstruct) == 0)
			{
				if (gstruct.gl_pathc > 0)
				{
					free(ctx->argv[i]);

					ctx->argv[i] = strdup(gstruct.gl_pathv[0]);

					int matches_to_add = gstruct.gl_pathc - 1;
					if (matches_to_add > 0 && ctx->argc + matches_to_add < 1024)
					{
						for (int j = ctx->argc - 1; j > i; j--)
						{
							ctx->argv[j + matches_to_add] = ctx->argv[j];
						}

						for (int k = 1; k <= matches_to_add; k++)
						{
							ctx->argv[i+k] = strdup(gstruct.gl_pathv[k]);
						}

						ctx->argc += matches_to_add;
						ctx->argv[ctx->argc] = NULL;
						i += matches_to_add;
					}
				}
				globfree(&gstruct);
			}
		}
	}
}


char *get_prompt()
{
	static char prompt_buf[4096];
	char cwd[1024];
	getcwd(cwd, sizeof(cwd));

	char *p = strchr(cwd, '/');
	char *folder = (p) ? p + 1 : cwd;

	char *status_color = (last_exit_status == 0) ? COLOR_GREEN : COLOR_RED;

	snprintf(prompt_buf, sizeof(prompt_buf), "%s[%s]%s %s(%d)%s $ ",
		COLOR_BLUE, folder, COLOR_RESET, status_color, last_exit_status, COLOR_RESET);

	return prompt_buf;
}

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
