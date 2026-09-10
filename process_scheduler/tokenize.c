#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "tokenize.h"

char ** tokenize(const char* string) {
	char *copy, *copy2, **args, *tok;
	int counter;
	
	copy = (char *) calloc(strlen(string)+1, sizeof(char));
	strcpy(copy, string);
	
	copy2 = (char *) calloc(strlen(string)+1, sizeof(char));
	strcpy(copy2, string);
	
	counter = 1;
	tok = strtok((char *)copy, " ");
	while(tok != NULL){
		if (strlen(tok) > 0) counter++;
		tok = strtok(NULL, " ");
	}
	
	args  = (char **) calloc(counter, sizeof(char *));
	
	counter = 0;
	tok = strtok(copy2, " ");
	while(tok != NULL){
		if(strlen(tok) > 0){
			args[counter] = (char *) calloc(strlen(tok)+1, sizeof(char));
			strcpy(args[counter], tok);
			counter++;
		}
		tok = strtok(NULL, " ");
	}
	args[counter] = NULL;
	
	return (args);
}

void free_args(char **args){
	int i = 0;
	
	while (args[i] != NULL){
		free(args[i]);
		i++;
	}
	free(args);
}