#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE 80 /* The maximum length command */
char *last_command[MAX_LINE / 2 + 1];

int main(void)
{
	char *args1[MAX_LINE / 2 + 1]; /* command line arguments */
	char *args2[MAX_LINE / 2 + 1]; /* command line arguments */

	int should_run = 1; /* flag to determine when to exit program */
	while (should_run)
	{
		printf("osh>");
		fflush(stdout);

		/**
		 * After reading user input, the steps are:
		 * (1) fork a child process using fork()
		 * (2) the child process will invoke execvp()
		 * (3) parent will invoke wait() unless command included & */
		//		printf("this is the beggining %s\n", last_command[0]);

		char str[MAX_LINE];
		fgets(str, MAX_LINE, stdin); // get user input

		char *token = strtok(str, "|\n"); // split token before and after |
		char *command1 = token;			  // get left side of pipe
		token = strtok(NULL, "|");
		char *command2 = token; // get right side of pipe

		// assign args1
		int i = 0;
		token = strtok(command1, " \n"); // tokenize
		while (token != NULL)
		{
			args1[i] = token;
			i++;
			token = strtok(NULL, " \n");
		}
		args1[i] = NULL; // null at the end

		// assign args2
		int q = 0;
		token = strtok(command2, " \n");
		while (token != NULL)
		{
			args2[q] = token;
			q++;
			token = strtok(NULL, " \n");
		}
		args2[q] = NULL; // null at the end

		int concurrent = 0;
		// check if parent and child will run concurrently
		if (strcmp(args1[i - 1], "&") == 0)
			concurrent = 1;

		// count size of args1 array
		int argsSize = 0;
		for (int i = 0; args1[i] != NULL; i++)
		{
			argsSize++;
		}

		// check if user types exit
		if (strcmp(args1[0], "exit") == 0)
		{
			should_run = 0;
			continue; // exit loop
		}

		// create pipe
		int fd[2];
		if (pipe(fd) == -1)
		{
			perror("pipe failed");
			return 1;
		}

		// create child process
		int pid1, pid2;
		pid1 = fork();

		// if child process
		if (pid1 == 0)
		{

			// history
			if (strcmp(args1[0], "!!") == 0)
			{
				if (last_command[0] == NULL)
					printf("No commands in history.\n");
				else
				{
					execvp(last_command[0], last_command);
					perror("execvp failed"); // Print error message if execvp fails
				}
			}
			else
			{
				if (argsSize == 3)
				{
					if (strcmp(args1[1], ">") == 0)
					{
						int console = dup(STDOUT_FILENO);							 // save console output
						int fd = open(args1[2], O_WRONLY | O_CREAT | O_TRUNC, 0644); // open file/create

						// Redirect input to the file descriptor
						if (dup2(fd, STDOUT_FILENO) == -1)
						{
							perror("dup2 failed");
							return 1;
						}

						// close the original file descriptor
						close(fd);

						// print to outfile
						execlp(args1[0], args1[0], NULL);

						fflush(stdout);				  // flush data
						close(STDOUT_FILENO);		  // close existing fd
						dup2(console, STDOUT_FILENO); // restore output to console
					}
					else if (strcmp(args1[1], "<") == 0)
					{
						int fd = open(args1[2], O_RDONLY); // open input file

						if (fd == -1)
						{
							perror("Failed to open input file");
							exit(1);
						}
						dup2(fd, STDIN_FILENO); // redirect input to fd
						close(fd);

						execlp(args1[0], args1[0], args1[2], NULL); // Execute sort command on input file
					}
				}
				else
				{
					pid2 = fork(); // create another child
					if (pid2 == 0)
					{
						close(fd[1]); // close write end

						dup2(fd[0], STDIN_FILENO); // redirect stdin to read end of pipe
						close(fd[0]);

						execvp(args2[0], args2); // execute process
						perror("execvp failed"); // failed
					}
					else if (pid2 > 0)
					{
						if (args2[0] != NULL)
						{								// check if there is pipe
							close(fd[0]);				// close read end
							dup2(fd[1], STDOUT_FILENO); // redirect stdout to write
							close(fd[1]);
						}
						execvp(args1[0], args1); // execute arguments
						perror("execvp failed"); // Print error message if execvp fails
												 // wait(NULL);
					}
				}
			}
		} // if (pid == 0)
		else if (pid1 > 0)
		{
			if (concurrent == 0)
			{ // no ampersand, run separately
				wait(NULL);
			}
		}

		else
		{
			perror("error."); // error
		}

		// if input is not "!!"
		if (strcmp(args1[0], "!!") != 0)
		{
			int j = 0;
			int l = 0;
			// clear the last_command array
			while (last_command[j] != NULL)
			{
				last_command[j] = NULL;
				j++;
			}
			// update last_command to user input
			while (args1[l] != NULL)
			{
				last_command[l] = malloc(strlen(args1[l]) + 1);
				strcpy(last_command[l], args1[l]);
				l++;
			}
			last_command[l] = NULL;
		}

	} // end of while(should_run)

	return 0;
}
