/*
	Header file containing the implementation of the WalkSAT algorithm.
	This version of the algorithm, uses the 0-break values.
	It also contains some helper functions.

	The user needs to first call the walk_init function, with a file
	name (the input file) as a parameter, so that information concerning 
	the problem will be stored in the global arrays. Then call the walksat
	function with a file name (the output file).
	The two files, have the same format as the ones from the bcsp.c file.

	Input File:
	1st row: N M K
	then M rows follow, with K integers each, from -N to N, excluding 0

	Output File:
	The assignment of the literals' truth value, which was found and can
	solve the problem (with 1 for true and -1 for false). The first integer corresponds
	to P1, the second to P2 etc.

	Liink used:
	https://lcs.ios.ac.cn/~caisw/Paper/Faster_WalkSAT.pdf
	https://iiis.tsinghua.edu.cn/uploadfile/2015/1022/20151022155124653.pdf.

	Note: Some helper functions, originate from the bcsp.c file, but have been
		  modifiied for the needs of the WalkSAT algorithm.

	More in the Documentation

	Kefsenidis Paraskevas, 2023.
*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define _empty(head) head == NULL		// Check if list empty
#define _reset(head) head = NULL		// Completely empty a list
#define _create_node(nd, lit) nd->_lit = lit; nd->_next = NULL;

#define _liti(lit) lit._lit_num			// It's is similar to saying Pi, where Pi is the literal
#define _truth_val(lit) lit._truth_val	// The truth value af a literal

// Checks whether a literal is positive and true or negativee and false. The lit_opt
// is the literal's number in the Problem and it can define if there is a not sign 
// in front of the lteral or not.
#define _neg_pos_truth_istrue(lit_opt, lit_assign) ((lit_opt > 0 && _truth_val(lit_assign[lit_opt - 1])) ||		\
													(lit_opt < 0 && !_truth_val(lit_assign[-lit_opt - 1])))
// Flip the truth value of a literal (from false to true and from true to false)
#define _flip_truth_val(lit) ((_truth_val(lit)) ?		\
							  (_truth_val(lit) = 0) :	\
							  (_truth_val(lit) = 1))

#define MAXP	1 		// The max value of a probability
#define P		0.567	// The first link verifies this as a good value.

int N;			// Number of literals
int K;			// Number of literals in disjunctive clauses
int M;			// Number of disjunctive clauses

int min_brk;	// The minimum break value

int *Problem;	// Deescription of the problem.

/*
	Below are two arrays, of size M * N (later allocated). These two
	are used for determining in which clause is each literal present.
	The first one refers to literals without the not sign, while the
	second refers to literals with the not sign. For example if 
	pos_lit_in[2 * N + 1] = true, then the clause number 3 (starting from 0)
	contains the literal P2, without a not sign.
*/
int *pos_lit_in;
int *neg_lit_in;

// Struct for literal info
typedef struct literal
{
	int _lit_num;
	int _truth_val;
} literal;

typedef struct node
{
	literal _lit;
	struct node *_next;
} node;

// A list node is a node, currently part of a list.
typedef node list_node;

// The head of the lst. t oints to the first noode of thee list.
typedef struct list_head
{
	list_node *_first;
	int _size;
} list_head;

// List for storing literals with the same
// break val, that is also the minimum one.
list_head *min_brk_lits = NULL;

// Add node at the head of a list
int shift(literal lit)
{
	node *nd = (node *) malloc(sizeof(node));

	_create_node(nd, lit);

	if(_empty(min_brk_lits))
	{
		min_brk_lits = (list_head *) malloc(sizeof(list_head));

		min_brk_lits->_first = nd;
		min_brk_lits->_size = 1;

		return 1;
	}

	// Insert new node in the list, at the head:
	nd->_next = min_brk_lits->_first;
	min_brk_lits->_first = nd;
	min_brk_lits->_size++;

	return 1;
}

// Select a node and return its value.
// First node is at index 0
literal select_nd(int pos)
{
	list_node *lnd = min_brk_lits->_first;

	for(int i = 0; i < pos; i++)
		lnd = lnd->_next;

	return lnd->_lit;
}

// Calculate the number of true literals in a clause. 
// The function can also be used to check the truth value
// of entire clauses.
int true_lits_in(int *clause, literal *lit_assign)
{
	int true_lits = 0;

	for(int i = 0; i < K; i++)
		if(_neg_pos_truth_istrue(clause[i], lit_assign))
			true_lits++;

	return true_lits;
}

// Calculate the break value of a literal
int break_count(int *clause, literal *lit_assign, int lit_num)
{
	int break_val = 0;

	/*
		The concept here is to find the number of clauses
		whose truth values are solely depedent on the selected 
		literal. This can happen when the selected literal is the only
		literal in the clause that returns a truth value of true, whether
		it is a positive or a negative literal, which differ only in the 
		search front.
	*/

	// Check whether the literal has a truth value of true, or false
	if(_truth_val(lit_assign[abs(lit_num)]))
	{
		// If t is true, then use the pos_lit_in array:
		for(int i = 0; i < M; i++)
		{
			// If literal is in clause i
			if(pos_lit_in[i * N + abs(lit_num)])
			{
				if(true_lits_in(Problem + i * K, lit_assign) == 1)
					break_val++;

				if(break_val > min_brk)
					return break_val;
			}
		}
	}
	// The truth value of the literal is false:
	else
	{
		for(int i = 0; i < M; i++)
		{
			if(neg_lit_in[i * N + abs(lit_num)])
			{
				if(true_lits_in(Problem + i * K, lit_assign) == 1)
					break_val++;

				if(break_val > min_brk)
					return break_val;
			}
		}
	}

	return break_val;
}

// Randomly pick a literal. We have 2 options
// from where to pick the literal: from the selected
// clause or from the min_brk_lits list.
literal pick_lit(int *clause, literal *lit_assign)
{
	literal lit;			// A tmp literal.
	literal ret_lit;		// The picked literal, that will be returned.
	int lit_brk;
	float p;

	// Initialized to M (the maximum break value)
	min_brk = M;

	// Fill the min_brk_lits list with literals
	// of the minimum break value, search for the minimum
	// and calculate each literal's break value, that is in the
	// selected clause.
	for(int i = 0; i < K; i++)
	{
		_liti(lit) = abs(clause[i]) - 1;
		lit_brk = break_count(clause, lit_assign, _liti(lit));
	
		if(lit_brk < min_brk)
		{
			min_brk = lit_brk;
			
			// Reset the list, so that it contains
			// only literals with the minimum break value
			_reset(min_brk_lits);
			shift(lit_assign[_liti(lit)]);
		}

		else 
			if(lit_brk == min_brk)
				shift(lit_assign[_liti(lit)]);	// Add literals of the minimum break value
	}

	// Randomly select a literal, either from a clause
	// or from the min_brk_lits list.
	
	// the 0-break condition:
	if(min_brk == 0)
		ret_lit = select_nd(rand() % min_brk_lits->_size);
	
	else
	{
		// Reduce to just 3 floating point numbers:
		p = (rand() / (float) RAND_MAX);

		p = roundf(p * 1000) / 1000;
		
		// Randomly select from clause
		if(p >= P)
		{
			int lit_num = abs(clause[rand() % K]) - 1;
			ret_lit = lit_assign[lit_num];
		}

		// Randomly select from min_brk_lits
		else
			if(p < P)
				ret_lit = select_nd(rand() % min_brk_lits->_size);
	}

	_reset(min_brk_lits);

	return ret_lit;
}

// Check whether the Problem is satisfied,
// with the current truth value assignment
int satisfiable(literal *lit_assign)
{
	int truth_val;

	// Check if all clauses return a truth value
	// of true. I will use the true_lits_in function
	for(int i = 0; i < M; i++)
	{
		truth_val = 0;
		
		if(true_lits_in(Problem + i * K, lit_assign) == 0)
			break;
		
		truth_val = 1;
	}

	// If all clauses are satisfied, 1 (true) will be returned
	return truth_val;
}

// Writes the assignment of the literals, that was found
// and can solve the problem
void write_sol(literal *lit_assign, char *outfname)
{
	FILE *outf;

	outf = fopen(outfname, "w");

	for(int i = 0; i < N; i++)
	{	
		// Parse the truth values to the output truth values
		// (0 (false) becomes -1 and 1 (true) remains true)
		if(_truth_val(lit_assign[i]))
			fprintf(outf, "%d ", 1);
		
		else if(!_truth_val(lit_assign[i]))
			fprintf(outf, "%d ", -1);
	}
}

// Displays the solution literal assignment found
void display(literal *lit_assign)
{
	for(int i = 0; i < N; i++)
	{
		if(_truth_val(lit_assign[i]))
			printf("P%d=%s ", i, "true");
		
		else if(!_truth_val(lit_assign[i]))
			printf("P%d=%s", i, "false");
	}
}

// Check if a clause has a truth value of true or false.
// The method returns this truth value. (the clause is 
// a disjunctive clause)
int clause_satisfaction(int *clause, literal *lit_assign)
{
	int truth_val = 0;

	for(int i = 0; i < K; i++)
		if(_neg_pos_truth_istrue(clause[i], lit_assign))
		{
			truth_val = 1;
			break;
		}

	return truth_val;
}

// The main body of the WalkSAT algorrithm
void walksat(char *outfname)
{
	int max_steps = 20000;		// I found on the internet that 100000 was used.
								// I will use 20000, because the execution time is
								// close with the time limit, of the other 2 algorithms.
	
	int clause_num;
	int found = 0;
	int steps = 0;
	literal lit_assign[N];
	literal lit;
	clock_t t1, t2;

	srand((unsigned) time(NULL));

	t1 = clock();

	// Randomly generate an assgnment for all literals
	for(int i = 0; i < N; i++)
	{
		_truth_val(lit_assign[i]) = rand() % 2;
		_liti(lit_assign[i]) = i;
	}

	// Find solution, or terminate, after max_steps tries.
	for(; steps < max_steps; steps++)
	{
		// WalkSAT found a solution
		if(satisfiable(lit_assign))
		{
			write_sol(lit_assign, outfname);
			found = 1;
			
			break;
		}

		// No solution, so randomly choose a literal from
		// a non satisfied clause.
		do
		{
			clause_num = rand() % M;
		} while(clause_satisfaction(Problem + clause_num * K, lit_assign));
		// The above while checks if the randomly chosen clause is satisfied
		// by the assignment or not.

		// Flip the randomly selected, from the randomly slectd clause, 
		// literal's truth value
		lit = pick_lit(Problem + clause_num * K, lit_assign);
		_flip_truth_val(lit_assign[_liti(lit)]);
	}

	t2 = clock();

	if(found)
	{
		printf("\n\nSolution found with WalkSAT!\n"); display(lit_assign); printf("\n");
		printf("Time spent: %f secs\n",((float) t2-t1)/CLOCKS_PER_SEC);
		printf("Number of steps: %d\n",steps);
	}

	else
	{
		printf("\n\nNO SOLUTION found with WalkSAT...\n");
		printf("Time spent: %f secs\n",((float) t2-t1)/CLOCKS_PER_SEC);
		printf("Number of steps: %d\n",steps);
	}
}

// The readfile from bcsp.c, but modified, for
// the needs of the walksat algorithm.
int walk_init(char *filename) {
	int i,j;

	FILE *infile;
	int err;
	
	// Opening the input file
	infile=fopen(filename,"r");
	if (infile==NULL) {
		printf("Cannot open input file. Now exiting...\n");
		fclose(infile);
		return -1;
	}

	// Reading the number of propositions
	err=fscanf(infile, "%d", &N);
	if (err<1) {
		printf("Cannot read the number of propositions. Now exiting...\n");
		fclose(infile);
		return -1;
	}

	if (N<1) {
		printf("Small number of propositions. Now exiting...\n");
		fclose(infile);
		return -1;
	}

	// Reading the number of sentences
	err=fscanf(infile, "%d", &M);
	if (err<1) {
		printf("Cannot read the number of sentences. Now exiting...\n");
		fclose(infile);
		return -1;
	}

	if (M<1) {
		printf("Low number of sentences. Now exiting...\n");
		fclose(infile);
		return -1;
	}

	// Reading the number of propositions per sentence
	err=fscanf(infile, "%d", &K);
	if (err<1) {
		printf("Cannot read the number of propositions per sentence. Now exiting...\n");
		fclose(infile);
		return -1;
	}

	if (K<2) {
		printf("Low number of propositions per sentence. Now exiting...\n");
		fclose(infile);
		return -1;
	}

	// Allocating memory for the sentences...
	Problem=(int*) malloc(M*K*sizeof(int));
	pos_lit_in = (int *) malloc(M * N * sizeof(int));
	neg_lit_in = (int *) malloc(M * N * sizeof(int));

	// ...and read them
	for (i=0;i<M;i++)
		for(j=0;j<K;j++) {
			err=fscanf(infile,"%d", Problem+i*K+j);
			if (err<1) {
				printf("Cannot read the #%d proposition of the #%d sentence. Now exiting...\n",j+1,i+1);
				fclose(infile);
				return -1;
			}
			if (Problem[i*K+j]==0 || Problem[i*K+j]>N || Problem[i*K+j]<-N) {
				printf("Wrong value for the #%d proposition of the #%d sentence. Now exiting...\n",j+1,i+1);
				fclose(infile);
				return -1;
			}

			// walk_init pos_lit_in and neg_lit_in:
			if(Problem[i * K + j] > 0)
				pos_lit_in[i * N + Problem[i * K + j] - 1] = 1;
			
			else if(Problem[i * K + j] < 0)
				neg_lit_in[i * N + (abs(Problem[i * K + j]) - 1)] = 1;
		}
	
	fclose(infile);

	return 0;
}