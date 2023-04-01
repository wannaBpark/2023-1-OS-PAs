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
static int v_wordCnt[33];
static size_t idx = 0;

int run_command(int nr_tokens, char *tokens[])
{
	pid_t pid, pid2;
	int status, cd, pos, i, k, flag = 0, nr_tokens_right, nr_tokens_left;
	int fd[2], fd2[2];
	char buf;
	char command[4096] = {'\0'};
	char** pp_left = NULL;
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
		pp_left = tokens;
		pp_right = (tokens + i + 1);
		flag = 1;
		pipe(fd);
		pipe(fd2);

		nr_tokens_left = i;
		nr_tokens_right = nr_tokens - nr_tokens_left - 1;

		//printf("left : %d right : %d\n", nr_tokens_left, nr_tokens_right);
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
	    if (pos == -1) { // DOES NOT EXIST, ADD_TAIL - FIRST, ADD NEW KEY
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
	        //OVERWRITE Existing value, free existing value
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
	    v_wordCnt[pos] = nr_tokens - 2;
	    //printf("Adding new keys and vals Finished!\n");
            goto _SUCCESS;
	}
_FORK:
	
	pid = fork();
	buf += 1;
	if (pid < _FORK_ERROR) {
	    goto _FORK;		
	} else if (pid == CHILD_PROCESS && flag == 1) {
	    close(fd[0]); close(fd[1]);
	    dup2(fd2[0], STDIN_FILENO);
	    close(fd2[1]);
	    tokens = pp_right;
	    nr_tokens = nr_tokens_right;
	    goto _ADDED_TOKENS;
	} else if (pid >= PARENT_PROCESS) {
	    if (flag != 1 ) {
		waitpid(pid, &status, 0);
		goto _SUCCESS;
            }
	    //pipe(fd2);
	    //close(fd[0]); close(fd2[0]); close(fd2[1]);
	    //close(fd[1]);
	    close(fd[0]);
	    close(fd[1]);
	    pid2 = fork();
	    if (pid2 == CHILD_PROCESS) {
	        dup2(fd2[1], STDOUT_FILENO);
		nr_tokens = nr_tokens_left;
		goto _ADDED_TOKENS;
	    }

	    p_parent = p_parent;
	    command[0] = command[0];
	    buf += 1;
	    pp_left = pp_left;
	    
	    waitpid(pid2, &status, 0);
	    close(fd2[1]);
	    waitpid(pid, &status, 0);
	    //close(fd2[1]);
	    close(fd2[0]);
	    flag = 0;
	    goto _SUCCESS;
	    //printf("Child status: %d\n", status);
	} else if (pid == CHILD_PROCESS) {
//_EXECUTE:

_ADDED_TOKENS:
	    pos = -1;
	    i = 0;
	    for (; i < nr_tokens; ++i) {
		for (k = 0; tokens[k] != NULL; ++k){
		   // printf("cur tokens[%d] : %s\n", k, tokens[k]);
		}
		for (j = 0; j < idx; ++j) {
		    char* p_k = *(pp_keys + j);
		    if (flag == 1) {
		        //printf("tokens: %s p_keys : %s\n", tokens[i], p_k);
		    }
		    if (!strcmp(tokens[i], p_k)) { // if tokens equals existing alias keys
			char* p_v = *(pp_vals + j);
			char* p_v_tok;
			length = strlen(p_v);
			pos = i;
			v_wordCnt[j] = 0;
			//printf("%s\n", p_v); 
			// xyz Hello world : v_wordCnt = 2;
			p_v_tok = strtok(p_v, " ");
			while(p_v_tok != NULL) {  
			    ++v_wordCnt[j];
			    p_v_tok = strtok(NULL, " ");
			}
			for (k = 0; k < (int)length; ++k) {
			    *(p_v + k) = (*(p_v + k) == '\0') ? ' ' : *(p_v + k);
			}	        

			//printf("%s FOUND word Count !! : %d\n", p_v, v_wordCnt[j]);
			p_v_tok = strtok(p_v, " ");
			for (k = nr_tokens + v_wordCnt[j] - 2; k >= i + v_wordCnt[j]; --k) { // Space back sizeof WordCNT - 1
			    tokens[k] = tokens[k - v_wordCnt[j] + 1];
			}
			free(tokens[i]);
			
			for (k = 0; p_v_tok != NULL; ++k) { // REPLACE with p_vals wordCnt Times, which is alias 
			    tokens[i + k] = malloc(sizeof(char) * (strlen(p_v_tok) + 1) );
			    strncpy(tokens[i + k], p_v_tok, strlen(p_v_tok) + 1); // Deep COPY for tokens
			    //printf("COPYING TOKEN[j] : %s\n", p_v_tok);
			    p_v_tok = strtok(NULL, " ");			    
			}
			for (k = 0; k < (int)length; ++k) { 
			    *(p_v + k) = (*(p_v + k) == '\0') ? ' ' : *(p_v + k);
			}
		        nr_tokens += v_wordCnt[j] - 1;
		        i += v_wordCnt[j] - 1;
	                //printf("cur token[i] : %s wordCNT: %d\n", tokens[i + 1 - v_wordCnt[j]], v_wordCnt[j]);
			
			break;
			//tokens[i] = malloc(sizeof(char) * (length + 1));
			//strcpy(tokens[i], p_v);
			//goto _FOUNDKEY;
		    }
	    	}
	    }
	   // printf("cur nr_tokens : %d\n", nr_tokens);
	    if (pid == CHILD_PROCESS && flag == 1)  {
	    for (i = 0; i < nr_tokens; ++i) {
	       // printf("After Aliased : %s\n", *(tokens + i));
	    }
	    }
	
	    goto _FOUNDKEY;
_FOUNDKEY: /*
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

		//for (i = 0; i <= nr_tokens; ++i) {
	  	//    printf("token (%d) : %s\n", i, tokens[i]);
		//}
	    }*/
	    //printf("My PID : %d\n", getpid()); 
	    if (-1 == execvp(tokens[0], tokens)) { //instructions that are NOT CD
	        fprintf(stderr, "Unable to execute %s\n", tokens[0]);
		exit(EXIT_FAILURE);
	    }

	    //if (pid2 == CHILD_PROCESS) close(fd2[1]);
	    //else if (pid == CHILD_PROCESS) close(fd2[0]);
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
