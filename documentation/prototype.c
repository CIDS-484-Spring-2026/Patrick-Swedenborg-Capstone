#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>

/*
 * Code from Professor Hendricks to debloat main.
 */
void printHelp() 
{
    printf("\n*** SHELL FEATURES ***"
           "\nList of Built-Ins:"
           "\n> exit <optional exit code>"
           "\n> cd <directory>"
           "\n> pwd"
           "\n> help"
           "\n\nYou can press Ctrl-C to terminate this shell.\n");
}

/*
 * Function to print the welcome screen to debloat main.
 */
void printWelcome() 
{
	printf("******************************************\n");
	printf("*                                        *\n");
	printf("*      ****Welcome to my shell!****      *\n");
	printf("*                                        *\n");
	printf("******************************************\n");
}

/*
 * Function based on code from Professor Hendricks canvas example, modified with my own code.
 * Checks for built-ins else runs execvp.
 */
int call(char *argv[])
{
	if (strcmp(argv[0], "help") == 0)
        {
                printHelp();
        } else if (strcmp(argv[0], "pwd") == 0)
        {
                char cwd[1024];
                getcwd(cwd, sizeof(cwd));
                printf("%s\n", cwd);
        } else if (strcmp(argv[0], "cd") == 0)
        {
                if (argv[1] != NULL)
                {
                        if (chdir(argv[1]) != 0)
                        {
                                fprintf(stderr, "cd failed: No such file or directory\n");
                        }
                } else
                {
                        chdir(getenv("HOME"));
                }
        } else if (strcmp(argv[0], "exit") == 0)
        {
                if (argv[1] != NULL)
                {
                        /*
                         * This bit of code is based on Professor Hendricks implementation of strtol on canvas.
                         */
                        char* endptr = NULL;
                        int base = 10;
                        errno = 0;
                        long exitCode = strtol(argv[1], &endptr, base);

                        if (errno != 0)
                        {
                                perror("strtol");
                                exit(EXIT_FAILURE);
                        }

                        if (*endptr != '\0')
                        {
                                fprintf(stderr, "%s%s\n", "Exit code must be a number. You entered: ", argv[1]);
                        } else if ((exitCode < 0 == 1) || (exitCode > 255 == 1))
                        {
                                fprintf(stderr, "%s%s\n", "Exit code must be a number from 0 to 255! You entered: ", argv[1]);
                        } else
                        {
                                printf("%s\n", "Goodbye!");
                                exit(exitCode);
                        }
                } else
                {
                        printf("%s\n", "Goodbye!");
                        exit(EXIT_SUCCESS);
                }
        } else { 
		pid_t pid = fork();

		if (pid == -1)
		{
			perror("fork");
			return EXIT_FAILURE;
		}

		if (pid == 0)
		{
			if (execvp(argv[0], argv) < 0)
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


/**
 * Function from Professor Hendricks.
 */
void removeTrailingSpaces(char *str) {
    int i = strlen(str) - 1;
    while (str[i] == ' ') i--;
    str[i+1] = '\0';
}


/**
 * Function from Professor Hendricks.
 */
char* getFilenamesReturnCommand(char *buffer, char *infile, char *outfile) {
    char* ptr = buffer;
    infile[0] = '\0';  // start with empty string infile and outfile
    outfile[0] = '\0';

    while(*ptr != '<' && *ptr != '>' && *ptr != '\0') ptr++;  // look for delimiter

    if (*ptr == '<') {  // if infile is present
        *(ptr++) = '\0';
        while(*ptr == ' ') ptr++;
        while(*ptr != '>' && *ptr != '\0') *(infile++) = *(ptr++);
        *infile = '\0';
        removeTrailingSpaces(infile);
    }


    if (*ptr == '>') {  // if outfile is present
        *(ptr++) = '\0';
        while(*ptr == ' ') ptr++;
        while(*ptr != '\0') *(outfile++) = *(ptr++);
        *outfile = '\0';
        removeTrailingSpaces(outfile);
    }

    removeTrailingSpaces(buffer);
    return buffer;  // at this point, buffer contains the command string

}

int main() 
{
	/*
	 * Initialize variables.
	 */
	char buffer[1000];
	char cwd[1024];
	char* cmds[1000];
	
	printWelcome(); // Print the welcome screen.

	/*
	 * Loop which runs the shell.
	 * Runs infinitely with the sole exit condition being CTRL + C or typing exit.
	 */
	while (1) 
	{
		/*
		 * Print working directory.
		 */
		if (getcwd(cwd, sizeof(cwd)) != NULL) {
		fflush(stdout);	
		printf("%s%s", cwd, "$ ");
		fflush(stdout); // Fix given by Professor Hendricks, forces any data in the buffer to be written to the output stream.
		}

		fgets(buffer, sizeof(buffer), stdin); // Get input.
		buffer[strcspn(buffer, "\n")] = '\0'; // Replace newline with null terminator
		
		char infile[100];
		char outfile[100];
		char* cmd = getFilenamesReturnCommand(buffer, infile, outfile);
		
		int stdin_backup = dup(STDIN_FILENO);
		int stdout_backup = dup(STDOUT_FILENO);

		mode_t mask;
		mask = umask(0000);

		if (infile[0] != '\0' && outfile[0] == '\0')
		{
			int fd_in = open(infile, O_RDWR);
			if (fd_in < 0) 
			{
				perror("open");
				exit(EXIT_FAILURE);
			}
			dup2(fd_in, STDIN_FILENO);
			close(fd_in);
			
		} else if (infile[0] == '\0' && outfile[0] != '\0')
		{
			
			int fd_out = open(outfile, O_RDWR | O_CREAT, 0777);
			if (fd_out < 0)
			{
				perror("open");
				exit(EXIT_FAILURE);
			}
			dup2(fd_out, STDOUT_FILENO);
			close(fd_out);
		} else if (infile[0] != '\0' && outfile[0] != '\0')
		{
			int fd_in = open(infile, O_RDWR);
			int fd_out = open(outfile, O_RDWR | O_CREAT, 0777);
			if (fd_in < 0 || fd_out < 0)
			{
				perror("open");
				exit(EXIT_FAILURE);
			}
			dup2(fd_in, STDIN_FILENO);
			dup2(fd_out, STDOUT_FILENO);
			close(fd_in);
			close(fd_out);
		}
		
		mask = umask(0022);
		/*
		dup2(stdin_backup, STDIN_FILENO);
		dup2(stdout_backup, STDOUT_FILENO);
		close(stdin_backup);
		close(stdout_backup); */

		/*
		 * All of the code below this comes from Professor Hendricks example code on canvas.
		 */
		int i = 0;
		cmds[i] = strtok(buffer, "|");
		while (cmds[i] != NULL)
		{
			i++;
			cmds[i] = strtok(NULL, "|");
		}

		int stdin_bak = dup(STDIN_FILENO);
		int stdout_bak = dup(STDOUT_FILENO);

		i = 0;
		while (cmds[i+1] != NULL)
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

			dup2(pipefd[1], STDOUT_FILENO);
			call(argv);
			dup2(pipefd[0], STDIN_FILENO);

			close(pipefd[1]);
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

		call(argv);
		dup2(stdin_bak, STDIN_FILENO);
		
		dup2(stdin_backup, STDIN_FILENO);
        	dup2(stdout_backup, STDOUT_FILENO);
        	close(stdin_backup);
        	close(stdout_backup); 
	}
}
