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

static char** pp_keys = NULL;
static char** pp_vals = NULL;
static size_t idx = 0;

int run_command(int nr_tokens, char *tokens[])
{
	pid_t pid, pid2;
	int status, cd, pos, i, flag = 0;
	int fd[2];
	char buf;
	char command[4096] = {'\0'};
	//char** pp_left = NULL;
	char** pp_right = NULL;
	char* p_parent = NULL;
	//char* p_child = NULL;
	size_t length, j;
	//static char** pp_keys = NULL;
	//static char** pp_vals = NULL;
	//size_t idx = 0, i;
	//char* env_PATH;
	enum ProcessState {
	    _FORK_ERROR = -1,
	    CHILD_PROCESS = 0,
	    PARENT_PROCESS = 1
	};
	for (i = 0; i < nr_tokens; ++i) {
	    //printf("token (%d) : %s\n", i, tokens[i]);
	    if (!strcmp(tokens[i], "|")) {
	        tokens[i] = NULL;
		//pp_left = tokens;
		pp_right = (tokens + i + 1);
		flag = 1;
		pipe(fd);
		goto _FORK;
	    }
	}
	//setenv("HOME", "/home/osos/os-pa1", 1);
	if (nr_tokens == 0) {
	    //fprintf(stderr, "Unable to execute %s\n", tokens[0]);
	    goto _ERROR;
	} else if (strcmp(tokens[0], "exit") == 0) {
	    goto _ERROR;
	}
	if (!strcmp("cd", tokens[0])){
	    cd = (nr_tokens == 1 || !strcmp("~", tokens[1])) ? chdir(getenv("HOME")) : chdir(tokens[1]);
	    if (cd != 0) { // If CD fails
		//printf("Somehow, cd failed %s\n", tokens[1]);
	    }
	    goto _SUCCESS;
	} else if (!strcmp("alias", tokens[0])) { // If it's alias instruction
	    size_t totLength = 0;
	    char* p_replace = NULL;
	    pos = -1;
	    i = 0;
	    
	    if (nr_tokens == 1) { // Just Print out existing keys and vals
	        while ((size_t)i < idx) {
		    fprintf(stderr, "%s: %s\n", *(pp_keys + i), *(pp_vals + i));
		    i++;
		}
		//printf("PRINT ALL ALIAS COMPLETELY!\n");
		goto _SUCCESS;
	    }
	    // IF allocating a new alias...

	    while ((size_t)i < idx) { //find if KEY DOES EXIST
		char* p_curKey = *(pp_keys + i);
		if (!strcmp(tokens[1], p_curKey)) {
		    pos = i; 
		    break;
		}
		++i;
	    }
	    //printf("pos already exists! : %d\n", pos);
	    if (pos == -1) { // DOES NOT EXIST, ADD_TAIL
		size_t length = strlen(tokens[1]);
		char* newKey = NULL;
	        char** tt = malloc(sizeof(char*) * (idx + 1));
		memcpy(tt, pp_keys, sizeof(char*) * idx);
		free(pp_keys);
		pp_keys = tt;

		tt = malloc(sizeof(char*) * (idx + 1));
		memcpy(tt, pp_vals, sizeof(char*) * idx);
		free(pp_vals);
		pp_vals = tt;

		//printf("CUR ALIAS IDX : %ld\n", idx + 1);
		++idx;

		newKey = malloc(sizeof(char) * (length + 1));
		strcpy(newKey, tokens[1]);
		*(pp_keys + idx - 1) = newKey;
		
		pos = idx - 1;
	    } else {
	        //OVERWRITE Existing value
		char* p_existVal = *(pp_vals + pos);
		free(p_existVal);
	    }
	    
	    for (i = 2; i < nr_tokens; ++i) {
		totLength += strlen(tokens[i]); 
	    }
	    p_replace = malloc(sizeof(char) * (totLength + nr_tokens - 3 + 1));
	    p_replace[0] = '\0';
	    for (i = 2; i < nr_tokens; ++i) {
		strcat(p_replace, tokens[i]);
		if (i != nr_tokens - 1) {
		    strcat(p_replace, " ");
		}
	    }
	    *(pp_vals + pos) = p_replace;
	    //printf("Adding new keys and vals Finished!\n");
            goto _SUCCESS;
	}
_FORK:
	// *************START FROM HERE!if (flag == 1) pipe(pfd);
	pid = fork();

	if (pid < _FORK_ERROR) {
	    goto _FORK;		
	} else if (pid >= PARENT_PROCESS) {
	    waitpid(pid, &status, 0);

	    if (flag != 1) {
	        goto _SUCCESS;
	    }
	    pid2 = fork();

	    if (pid2 == CHILD_PROCESS) {
	        dup2(fd[0], STDIN_FILENO);
		close(fd[1]);

		i = j = 0;
		while (read(fd[0], &buf, 1) > 0) {
		    //write(STDOUT_FILENO, &buf, 1);
		    command[i++] = buf;
		    if (buf == '\0'){
		        //printf("complete! : %s\n", command);
			tokens[j] = realloc(tokens[j], sizeof(char) * (strlen(command) + 1));
			strcpy(tokens[j], command);
			i = 0;
			++j;
		    }
		}
		close(fd[0]);
		tokens[j] = NULL;
	
		goto _EXECUTE;
	    } else {
	        close(fd[0]);
		while (*pp_right != NULL) {
		    p_parent = *pp_right;
		    //printf("copy from parent : %s\n", *pp_right);
		    while (1) {
		        write(fd[1], p_parent, 1);
			if (*p_parent == '\0') break;
			++p_parent;
		    }
		    ++pp_right;
		}
		close(fd[1]);
		waitpid(pid2, &status, 0);
	    }
	    goto _SUCCESS;
	    //printf("Child status: %d\n", status);
	} else if (pid == CHILD_PROCESS) {
_EXECUTE:
	    // CHeck if it's cd command
	    pos = -1;

	    for (i = 0; i < nr_tokens; ++i) { 
	    	for (j = 0; j < idx; ++j) {
		    char* p_k = *(pp_keys + j);
		    if (!strcmp(tokens[i], p_k)) { // if tokens equals existing alias keys
			char* p_v = *(pp_vals + j);
			length = strlen(p_v);
			pos = i;
		        
			free(tokens[i]);
			tokens[i] = malloc(sizeof(char) * (length + 1));
			strcpy(tokens[i], p_v);
			goto _FOUNDKEY;
		    }
	    	}
	    }
_FOUNDKEY:
	    if (pos == 0) { //Things to do for 'll' 'ls -al'
		char* p_a = NULL;
		char* p_b = NULL;
		char* tmp = strtok(tokens[0], " ");
		length = strlen(tmp);
		p_a = malloc(sizeof(char) * (length + 1));
		//printf("front : %s\n", tmp);

		strcpy(p_a, tmp);
		tmp[length] = ' ';

		tmp += (length + 1);
		//tmp = strpbrk(tokens[0], " ");
		//printf("backward : %s\n", tmp);
		length = strlen(tmp);
		p_b = malloc(sizeof(char) * (length + 1));
		strcpy(p_b, tmp);

		free(tokens[0]);
		tokens[0] = p_a;
		for (i = nr_tokens; i >= 2; --i) {
	            tokens[i] = tokens[i - 1];
		}
		tokens[1] = p_b;

		/*for (i = 0; i <= nr_tokens; ++i) {
	  	    printf("token (%d) : %s\n", i, tokens[i]);
		}*/
	    }
	    //printf("My PID : %d\n", getpid()); 
	    if (-1 == execvp(tokens[0], tokens)) { //instructions that are NOT CD
	        fprintf(stderr, "Unable to execute %s\n", tokens[0]);
		exit(EXIT_FAILURE);
	    }
	}
_SUCCESS:
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
	/*
    size_t i;
    char* p_k = NULL;
    char* p_v = NULL;
    for (i = 0; i < idx; ++i) {
        p_k = *(pp_vals + i);
	p_v = *(pp_keys + i);

	free(p_k);
	free(p_v);
    }
    free(pp_vals);
    free(pp_keys);*/
}
