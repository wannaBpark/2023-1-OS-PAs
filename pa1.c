/**********************************************************************
 * Copyright (c) 2020-2023
 *  Sang-Hoon Kim <sanghoonkim@ajou.ac.kr>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTIABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 **********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
/***********************************************************************
 * run_command()
 *
 * DESCRIPTION
 *   Implement the specified shell features here using the parsed
 *   command tokens.
 *
 * RETURN VALUE
 *   Return 1 on successful command execution
 *   Return 0 when user inputs "exit"
 *   Return <0 on error
 */
int run_command(int nr_tokens, char *tokens[])
{
	pid_t pid;
	int status;
	enum ProcessState {
	    _FORK_ERROR = -1,
	    CHILD_PROCESS = 0,
	    PARENT_PROCESS = 1
	};
	//printf("number of tokens : %d\n", nr_tokens);
	if (nr_tokens == 0) {
	    //fprintf(stderr, "Unable to execute %s\n", tokens[0]);
	    goto _ERROR;
	} else if (strcmp(tokens[0], "exit") == 0) {
	    goto _ERROR;
	}

_FORK:
	pid = fork();

	if (pid < _FORK_ERROR) {
	    goto _FORK;		
	} else if (pid >= PARENT_PROCESS) {
	    waitpid(pid, &status, 0);
	    //printf("Child status: %d\n", status);
	} else if (pid == CHILD_PROCESS) {
	    if (-1 == execvp(tokens[0], tokens)){
	        fprintf(stderr, "Unable to execute %s\n", tokens[0]);
		exit(EXIT_FAILURE);
	    }
	}
	return 1;
_ERROR:
	return 0;
}


/***********************************************************************
 * initialize()
 *
 * DESCRIPTION
 *   Call-back function for your own initialization code. It is OK to
 *   leave blank if you don't need any initialization.
 *
 * RETURN VALUE
 *   Return 0 on successful initialization.
 *   Return other value on error, which leads the program to exit.
 */
int initialize(int argc, char * const argv[])
{
		int a = 0;
		return a;
}


/***********************************************************************
 * finalize()
 *
 * DESCRIPTION
 *   Callback function for finalizing your code. Like @initialize(),
 *   you may leave this function blank.
 */
void finalize(int argc, char * const argv[])
{
}
