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
int find_alias_index(char* p_tok);
int get_value_wordCnt(int index);
void replace_token_with_alias(char*** ppp_tok, int tokIdx, int valIdx);
void space_back_wordCnt(char**, int, int, int);

static char** pp_keys = NULL;
static char** pp_vals = NULL;
static int v_wordCnt[33];
static size_t idx = 0;


int run_command(int nr_tokens, char *tokens[])
{
	pid_t pid, pid2;
	int status, cd, pos, i = 0, flag = 0, nr_tokens_right, nr_tokens_left;
	int fd2[2];
	char** pp_right = NULL;
	enum ProcessState {
	    _FORK_ERROR = -1,
	    CHILD_PROCESS = 0,
	    PARENT_PROCESS = 1
	};

	// CHECK IF IT's PIPE | COMMAND 
	for (i = 0; i < nr_tokens; ++i) {
	    if (!strcmp(tokens[i], "|")) {
	        tokens[i] = NULL;
		pp_right = (tokens + i + 1);
		flag = 1;
		pipe(fd2);

		nr_tokens_left = i; nr_tokens_right = nr_tokens - nr_tokens_left - 1;
		goto _FORK;
	    }
	}
	if (nr_tokens == 0) {
	    fprintf(stderr, "Unable to execute %s\n", tokens[0]);
	    goto _ERROR;
	} else if (strcmp(tokens[0], "exit") == 0) {
	    goto _ERROR;
	}
	
	if (!strcmp("cd", tokens[0])){
	    cd = (nr_tokens == 1 || !strcmp("~", tokens[1])) ? chdir(getenv("HOME")) : chdir(tokens[1]);
	    if (cd == -1) {
	    	fprintf(stderr, "CD FAILED\n");
	    }
	    goto _SUCCESS;
	} else if (!strcmp("alias", tokens[0])) { // If it's alias instruction
	    size_t totLength = 0;
	    char* p_newValue = NULL;
	    
	    if (nr_tokens == 1) { // Just Print out existing keys and vals
	        while ((size_t)i < idx) {
		    fprintf(stderr, "%s: %s\n", *(pp_keys + i), *(pp_vals + i));
		    i++;
		}
		goto _SUCCESS;
	    }
	    // IF allocating a new alias...

	    // FIND if KEY DOES EXIST
	    pos = find_alias_index(tokens[1]);
	    if (pos == -1) { // DOES NOT EXIST, ADD_TAIL - FIRST, ADD NEW KEY
		size_t length = strlen(tokens[1]);
		char* newKey = NULL;
	              
		pp_keys = realloc(pp_keys, sizeof(char*) * (idx + 1));	
		pp_vals = realloc(pp_vals, sizeof(char*) * (idx + 1));

		newKey = malloc(sizeof(char) * (length + 1));
		strcpy(newKey, tokens[1]);
		pp_keys[idx] = newKey;	
		pos = idx;

		++idx;
	    } else {
	        //OVERWRITE Existing value, free existing value
		char* p_existVal = pp_vals[pos];
		free(p_existVal);
	    }
	    
	    for (i = 2; i < nr_tokens; ++i) 
	        totLength += strlen(tokens[i]);

	    p_newValue= malloc(sizeof(char) * (totLength + nr_tokens - 3 + 1));
	    p_newValue[0] = '\0';
	    for (i = 2; i < nr_tokens; ++i) {
		strcat(p_newValue, tokens[i]);
		if (i != nr_tokens - 1) {
		    strcat(p_newValue, " ");
		}
	    }
	    pp_vals[pos] = p_newValue;
	    v_wordCnt[pos] = get_value_wordCnt(pos);
            
	    goto _SUCCESS;
	}
_FORK:	
	pid = fork();
	if (pid < _FORK_ERROR) {
	    goto _FORK;		
	} else if (pid >= PARENT_PROCESS) {
	    if (flag != 1 ) {
		waitpid(pid, &status, 0);
		goto _SUCCESS;
            }

	    pid2 = fork();
	    if (pid2 == CHILD_PROCESS) {
	        dup2(fd2[1], STDOUT_FILENO);
		nr_tokens = nr_tokens_left;
		goto _ADDED_TOKENS;
	    }
	    
	    waitpid(pid2, &status, 0); close(fd2[1]);
	    waitpid(pid, &status, 0); close(fd2[0]);
	    goto _SUCCESS;
	} else if (pid == CHILD_PROCESS) {
	    if (flag == 1) {
	    	dup2(fd2[0], STDIN_FILENO); close(fd2[1]);
	    	tokens = pp_right; nr_tokens = nr_tokens_right;
	    }
_ADDED_TOKENS:
	    for (i = 0; i < nr_tokens; ++i) {
		int pos = find_alias_index(tokens[i]);
		if (pos != -1) {
	       	    // Space back sizeof WordCNT - 1
		    space_back_wordCnt(tokens, nr_tokens, i, pos);
		    free(tokens[i]);
		    //tokens[nr_tokens + v_wordCnt[j] - 1] = NULL;
                    replace_token_with_alias(&tokens, i, pos);			
		    // REPLACE with p_vals wordCnt Times, which is alias 
		    nr_tokens += v_wordCnt[pos] - 1;
		    i += v_wordCnt[pos] - 1;	
		}
	    }
	    goto _FOUNDKEY;
_FOUNDKEY: 
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

int get_value_wordCnt(int idx)
{
    char* p_v = *(pp_vals + idx);
    char* p_v_tok;
    size_t j, length = strlen(p_v);
    size_t ret = 0;
    p_v_tok = strtok(p_v, " ");
    while (p_v_tok != NULL) {
        ++ret;
	p_v_tok = strtok(NULL, " ");
    }
    for (j = 0; j < length; ++j) {
        *(p_v + j) = (*(p_v + j) == '\0') ? ' ' : *(p_v + j);
    }

    return ret;
}

int find_alias_index(char* p_tok)
{
    int ret = -1;
    size_t i = 0;    
    while (i < idx) {
        char* p_keys = *(pp_keys + i);
	if (!strcmp(p_keys, p_tok)) return ret = i;
	++i;
    }
    return ret;
}

void space_back_wordCnt(char** pp_tok, int cur_nr_tokens, int tokIdx, int valIdx)
{
    int i, wordCnt = v_wordCnt[valIdx];
    // Space back sizeof WordCNT - 1
    for (i = cur_nr_tokens + wordCnt - 2; i >= tokIdx + wordCnt; --i) { 
        pp_tok[i] = pp_tok[i - wordCnt + 1];
    }
    pp_tok[cur_nr_tokens + wordCnt - 1] = NULL;

    return;
}
void replace_token_with_alias(char*** ppp_tok, int tokIdx, int valIdx)
{
    size_t j, length, length_tok;
    //int wordCnt = v_wordCnt[valIdx];
    char** pp_tok = *ppp_tok;
    char* p_v = *(pp_vals + valIdx);
    char* p_v_tok;
    char* tmp;

    length = strlen(p_v);
    p_v_tok = strtok(p_v, " ");

    for (j = 0; p_v_tok != NULL; ++j) {
	length_tok = strlen(p_v_tok);
	
	tmp = malloc(sizeof(char) * (length_tok + 1));
	strncpy(tmp, p_v_tok, (length_tok + 1));
	pp_tok[tokIdx + j] = tmp;

	p_v_tok = strtok(NULL, " ");
    }

    for (j = 0; j < length; ++j) {
        *(p_v + j) = (*(p_v + j) == '\0') ? ' ' : *(p_v + j); 
    }

    return;
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
