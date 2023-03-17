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

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "types.h"
#include "parser.h"

int parse_command(char *command, int *nr_tokens, char *tokens[])
{
	const char *delimiters = " \t\r\f\r\n\v";
	char *curr;
	*nr_tokens = 0;

	while ((curr = strtok(command, delimiters))) {
		*tokens++ = strdup(curr);
		(*nr_tokens)++;
		command = NULL;
	}
	*tokens = NULL;

	return (*nr_tokens > 0);
}

void free_command_tokens(char *tokens[])
{
	for (char *token = *tokens; token; token = *(++tokens)) {
		free(token);
	}
}
