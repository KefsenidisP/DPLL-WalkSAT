/*
    Copy of bcsp.c's main function.
	This modified bcsp c file, is used for testing the 
	WalkSAT and DPLL algorithms.

	Kefsenidis Paraskevas, 2023
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "walksat.h"
#include "dpll.h"

void syntax_error(char **argv) {
	printf("Use the following syntax:\n\n");
	printf("%s <method> <inputfile> <outputfile>\n\n", argv[0]);
	printf("where:\n");
	printf("<method> is either 'walk' or 'dpll' (without the quotes)\n");
	printf("<inputfile> is the name of the file with the problem description\n");
	printf("<outputfile> is the name of the output file with the solution\n");
}

int main(int argc, char **argv)                                                                     
{
    int err;

    if (argc != 4) {
		printf("Wrong number of arguments. Now exiting...\n");
		syntax_error(argv);
		return -1;
	}

	if (strcmp(argv[1], "dpll") == 0)
		dpll_satisfaction(argv[2], argv[3]);
	else if(strcmp(argv[1], "walk") == 0)
	{
		walk_init(argv[2]);
		walksat(argv[3]);
	}
	else
		syntax_error(argv);

	return 0;
}