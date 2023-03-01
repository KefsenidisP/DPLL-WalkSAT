/*
    Header file containing the DPLL implementation, as described in the links below.
    This is a recursive version of the algorithm.

    In order for this method to be used, the user has to call the dpll_satisfaction
    function, passing in it the file containing the SAT problem, described in the CNF
    format and the name of the output file, the user wants the solution to the problem
    to be stored (note that it writes on the file, not appends).
    The two files, have the same format as the ones from the bcsp.c file.

	Input File:
	1st row: N M K
	then M rows follow, with K integers each, from -N to N, excluding 0

	Output File:
	The assignment of the literals' truth value, which was found and can
	solve the problem (with 1 for true and -1 for false). The first integer corresponds
	to P1, the second to P2 etc.

    The algorithm's pseudo code can be found here: 
    https://github.com/aimacode/aima-pseudocode/blob/master/md/DPLL-Satisfiable.md,
    while the below link was also used for the final implementation:
    https://www.cs.miami.edu/home/geoff/Courses/CSC648-12S/Content/DPLL.shtml
    I also used this site in order to create some of the structs I used:
    https://spectrum.library.concordia.ca/id/eprint/976566/1/MR63140.pdf

    Note: Some helper functions, originate from the bcsp.c file, but have been
		  modifiied for the needs of the DPLL algorithm.

    More on the Documentation

    Kefsenidis Paraskevas, 2023
*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// For backtracing in the dpll algorithm, when the current assignment
// does not statisfy some clause
#define _backtrack(sym) set(*sym); sym->_assigned = 0;

// Defines whether a literal is negative and the symbol false or
// the literal positive and the symbol true
#define _lit_truth(sym, lit) ((lit > 0 && sym._truth_val) ||    \
                               lit < 0 && !sym._truth_val)

int K;      // The number of literals in a disjunctive clause
int M;      // The nuumber of disjunctive clauses in the CNF clause
int N;      // The total number of Pi literals used.

// See clause struct
enum situation
{
    SET = 1,
    UNSET = 0
};

// Information about symbols 
typedef struct symbol
{
    int _id;                 // Symbols Pi are defined by i, which we declare as id
    int *_in_clauses;        // The clauses the symbol is in
    int *_in_clauses_loc;    // The position of the symbol in the corresponding clause
    int _truth_val;          // The symbol's truth value
    int _assigned;           // If the symbol has been assigned a truth value or not
} symbol;

// Struct for clause information. 
typedef struct clause
{
    int *_literals;
    int *_lit_situation;        // Tracks which literals in the clause are set (1) or unset (0)
    int _clause_situation;      // If clause is set (1) or unset (0)
} clause;
/*
    UNSET clauses, are clauses that are deleted because 
    they are no longer false, while SET clauses, are clauses
    that have a false value. Note that UNSET clauses are ignored
    when searching for a symbol, on which to assign a truth value.
    
    UNSET literals on the other hand, are literals deleted from a
    clause, because their not occurance has a truth value that
    returns true. Set literals, are literals with their symbol assigned
    a truth value, so that they return true. So if symbol P1 is assigned
    a truth value of 1 then the literal P1 will be SET, while the literal
    !P1 will be UNSET.
*/

clause *clauses;                // The problem clauses in the CNF.
symbol *symbols;                // The problem's existing symbols. 0 for P1, 1 for P2 etc
int *sol;                       // The solution found

// The readfile from bcsp.c, but modified, for
// the needs of the walksat algorithm.
int dpll_init(char *filename) {
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
	clauses = (clause* ) calloc(M, sizeof(clause));
    symbols = (symbol *) calloc(N, sizeof(symbol));
    sol = (int *) calloc(N, sizeof(int));

    for(int k = 0; k < N; k++)
    {
        symbols[k]._in_clauses = (int *) calloc(M, sizeof(int));
        symbols[k]._in_clauses_loc = (int *) calloc(M, sizeof(int));
    }

	// ...and read them
	for (i=0;i<M;i++)
    {
        clauses[i]._literals = (int *) calloc(K, sizeof(int));
        clauses[i]._lit_situation = (int *) calloc(K, sizeof(int));
        clauses[i]._clause_situation = SET;

		for(j=0;j<K;j++) {
			err=fscanf(infile,"%d", &(clauses[i]._literals[j]));
			if (err<1) {
				printf("Cannot read the #%d proposition of the #%d sentence. Now exiting...\n",j+1,i+1);
				fclose(infile);
				return -1;
			}
			if (clauses[i]._literals[j] == 0 || clauses[i]._literals[j] > N || clauses[i]._literals[j] < -N) {
				printf("Wrong value for the #%d proposition of the #%d sentence. Now exiting...\n",j+1,i+1);
				fclose(infile);
				return -1;
			}

            clauses[i]._lit_situation[j] = SET;

            symbols[abs(clauses[i]._literals[j]) - 1]._id = (abs(clauses[i]._literals[j]) - 1);
            symbols[abs(clauses[i]._literals[j]) - 1]._in_clauses[i] = 1;
            symbols[abs(clauses[i]._literals[j]) - 1]._in_clauses_loc[i] = j;
            symbols[abs(clauses[i]._literals[j]) - 1]._assigned = 0;
		}
    }
	
	fclose(infile);

	return 0;
}

// Check if all clauses are UNSET, meaning that they are 
// satisfied, by the current truth value assignment
int empty(void)
{
    for(int i = 0; i < M; i++)
        if(clauses[i]._clause_situation == SET)
            return 0;

    // No clause is SET, so clauses is "empty"
    return 1;
}

// Checks if there is an unsatisfiable clause, because of the 
// current assignment. As unsatisfiable, I mean clauses that have 
// all of their K literals assigned a value, but they remain 
// unsatisfied.
int false_exists(void)
{
    for(int i = 0; i < M; i++)
    {   
        int set_found = 0;          // If it contains a SET literal

        // Check only set clauses. We know that unset
        // clauses are true
        if(clauses[i]._clause_situation == SET)
        {
            for(int j = 0; j < K; j++)
            {
                // False literals, are unset literals
                if(clauses[i]._lit_situation[j] == SET) 
                {
                    set_found = 1;
                    break;
                }

            }
        
            // All literals, are assigned and return false  
            if(!set_found)
                return 1;
        }
    }

    // False does not exist
    return 0; 
}

// Unset clauses containing the literal sym and unset the
// literal !sym
void unset(symbol sym)
{
    if(sym._truth_val)
    {
        // For each clause containing the symbol
        for(int i = 0; i < M; i++)
            if(sym._in_clauses[i])
            {
                // If the symbol appears as a positive literal in the clause
                // unset the clause.
                if(clauses[i]._literals[sym._in_clauses_loc[i]] > 0)
                    clauses[i]._clause_situation = UNSET;

                // If the symbol appears as a negative literal in the clause
                // unset the literal.
                if(clauses[i]._literals[sym._in_clauses_loc[i]] < 0)
                    clauses[i]._lit_situation[sym._in_clauses_loc[i]] = UNSET;
            }
        
    }

    // The opposite is done if the truth value is false
    else
    {
        // For each clause containing the symbol
        for(int i = 0; i < M; i++)
            if(sym._in_clauses[i])
            {
                // If the symbol appears as a negative literal in the clause
                // unset the clause.
                if(clauses[i]._literals[sym._in_clauses_loc[i]] < 0)
                    clauses[i]._clause_situation = UNSET;

                // If the symbol appears as a positive literal in the clause
                // unset the literal.
                if(clauses[i]._literals[sym._in_clauses_loc[i]] > 0)
                    clauses[i]._lit_situation[sym._in_clauses_loc[i]] = UNSET;
            }
    }
}

// Count how many literals in a clause return a truth value
// of true. Needed by set function, as we can only set an 
// entire clause if its truth value depends solely one literal
int true_lit_num(int clause_num)
{
    int true_lits = 0;

    for(int i = 0; i < K; i++)
        if(symbols[abs(clauses[clause_num]._literals[i]) - 1]._assigned)
            if(_lit_truth(symbols[abs(clauses[clause_num]._literals[i]) - 1], clauses[clause_num]._literals[i]))
                true_lits++;

    return true_lits;
}

// Reverse of unset function. Set clauses containing
// the literal sym and set the literal !sym
void set(symbol sym)
{
    if(sym._truth_val)
    {
        // For each clause containing the symbol
        for(int i = 0; i < M; i++)
            if(sym._in_clauses[i])
            {
                // If the symbol appears as a positive literal in the clause
                // set the clause.
                if(clauses[i]._literals[sym._in_clauses_loc[i]] > 0 && true_lit_num(i) == 1)
                    clauses[i]._clause_situation = SET;

                // If the symbol appears as a negative literal in the clause
                // set the literal.
                if(clauses[i]._literals[sym._in_clauses_loc[i]] < 0)
                    clauses[i]._lit_situation[sym._in_clauses_loc[i]] = SET;
            }
    }

    // The opposite is done if the truth value is false
    else
    {
        // For each clause containing the symbol
        for(int i = 0; i < M; i++)
            if(sym._in_clauses[i])
            {
                // If the symbol appears as a negative literal in the clause
                // set the clause.
                if(clauses[i]._literals[sym._in_clauses_loc[i]] < 0 && true_lit_num(i) == 1)
                    clauses[i]._clause_situation = SET;

                // If the symbol appears as a positive literal in the clause
                // set the literal.
                if(clauses[i]._literals[sym._in_clauses_loc[i]] > 0)
                    clauses[i]._lit_situation[sym._in_clauses_loc[i]] = SET;
            }
    }
}

// Selects the first not assigned symbol
symbol *pick_first(void)
{
    for(int i = 0; i < N; i++)
        if(!symbols[i]._assigned)
            return &(symbols[i]);

    return NULL;
}

// Returns the first occurence of the symbol's
// literal in clauses
int *first_occurence(symbol sym)
{
    for(int i = 0; i < M; i++)
        if(clauses[i]._clause_situation == SET)
            if(sym._in_clauses[i])
                return &(clauses[i]._literals[sym._in_clauses_loc[i]]);
    
    return NULL;
}

// Find a pure symbol in the clauses. Pure is a symbol
// that is only encoutered as a positive xor a negative
// literal
symbol *find_pure_symbol(void)
{
    int *lit;                           // To check if the symbol appears as both negative and positive

    for(int i = 0; i < N; i++)
    {
        int pure = 1;                                   // Start by stating that the symbol is true

        // We want to find unassigned symbols
        if(!symbols[i]._assigned)
        {
            lit = first_occurence(symbols[i]);                        

            if(lit == NULL)
                continue;

            for(int j = 0; j < M; j++)
                if(clauses[j]._clause_situation == SET && symbols[i]._in_clauses[j])           // Only SET clauses
                    if(*lit != clauses[j]._literals[symbols[i]._in_clauses_loc[j]])     // Literal changed (from positive to negative or vice versa)
                    {
                        pure = 0;
                        break;
                    }   

            // Return the symbol, after making some calculations, like
            // assigning a truth value
            if(pure)
            {
                symbols[i]._truth_val = ((*lit > 0) ? (1) : (0));
                symbols[i]._assigned = 1;

                return &(symbols[i]);
            }
        }
    }

    // No pure symbol
    return NULL;
}

// Find unitary clauses. A clause is considered unitary
// when all its literals return false, exept for one
// that it is not yet assigned.
symbol *find_unit_clause(void)
{
    // For each SET clause, check which is unitary
    for(int i = 0; i < M; i++)
    {
        int set_in_clause = 0;                              // If a SET literal exists in the clause
        int set_lit_num = 0;                                // Number of set literals in clause
        int lit;                                            // The only literal of the unitary clause (if there is)

        if(clauses[i]._clause_situation == SET)
        {
            // A clause can be unitary if all, but one of its
            // literals is both unassigned and SET, while the rest,
            // are UNSET and of course assigned
            for(int j = 0; j < K; j++)
            {
                // Note that SET also means assigned and true or not assigned literals
                // In this, the not assigned literals.
                if(clauses[i]._lit_situation[j] == SET)
                {
                    set_in_clause = 1;
                    set_lit_num++;

                    if(set_lit_num == 1)
                        lit = clauses[i]._literals[j];
                    else
                        break;
                }
            }
            
            // All but one literals are SET
            if(set_in_clause && set_lit_num == 1)
            {
                symbols[abs(lit) - 1]._truth_val = ((lit > 0) ? (1) : (0));
                symbols[abs(lit) - 1]._assigned = 1;

                return &(symbols[abs(lit) - 1]);
            }
        }
    }

    return NULL;
}

// The dpll algorithm implementation, based on this site:
// https://www.cs.miami.edu/home/geoff/Courses/CSC648-12S/Content/DPLL.shtml
// and primarily on this site: 
// https://github.com/aimacode/aima-pseudocode/blob/master/md/DPLL-Satisfiable.md
// (also on the book)
int dpll(void)
{
    symbol *sym;

    if(empty())
        return 1;

    if(false_exists())
        return 0;

    // If there exists a pure symbol or a unit clause,
    // select them
    if((sym = find_pure_symbol()) != NULL)
    {
        unset(*sym);

        if(dpll())
            return 1;
        else
        {
            _backtrack(sym);
            return 0;
        }
    }

    if((sym = find_unit_clause()) != NULL)
    {
        unset(*sym);

        if(dpll())
            return 1;
        else
        {
            _backtrack(sym);
            return 0;
        }
    }

    // Or any other symbol
    sym = pick_first();

    // The problem remains unsatisfiable and no more
    // literals available for assgnment
    if(sym == NULL)
        return 0;

    // Assign true
    sym->_truth_val = 1;
    sym->_assigned = 1;
    unset(*sym);

    if(dpll())
        return 1;
    
    // Assignment failed
    else
    {
        // Reset the clause to previous state and
        // assign false to the symbol
        _backtrack(sym);

        // Assign false
        sym->_truth_val = 0;
        sym->_assigned = 1;
        unset(*sym);

        if(dpll())
            return 1;
        
        else
        {
            _backtrack(sym);
            return 0;
        }
    }
}

// Writes solution to a file. False as -1 and true as 1
int write_to_file(char *outfname)
{
    FILE *outf;
    int error;

    outf = fopen(outfname, "w");

    if(outf == NULL)
    {
        fprintf(stderr, "An error occured opening the file!");

        return 0;
    }

    for(int i = 0; i < N; i++)
    {
        if(symbols[i]._truth_val)
            fprintf(outf, "%d ", 1);
        else
            fprintf(outf, "%d ", -1);
    }

    return 1;
}

// Prints a solution 
void print_sol(void)
{
    for(int i = 0; i < N; i++)
    {
        if(symbols[i]._truth_val)
            printf("P%d=%s ", i + 1, "true");
        else
            printf("P%d=%s ", i + 1, "false");
    }
}

// The dpll algorithm constructor. It is a wrapper around the dpll
// algorithm, used for initialization and printing the solution if found
void dpll_satisfaction(char *infname, char *outfname)
{
    int err;
    clock_t t1, t2;

    // Initialize all DPLL variables
    err = dpll_init(infname);

    if(err)
        exit(-1);
   
    // Call DPLL main body
    t1 = clock();
    int found = dpll();
    t2 = clock();

    if(found)
    {
        printf("\n\nSolution found with DPLL!\n"); print_sol(); printf("\n");
		printf("Time spent: %f secs\n",((float) t2 - t1) / CLOCKS_PER_SEC);
        write_to_file(outfname);
    }

    else
    {
        printf("\n\nThere is no solution to the problem...\n");
		printf("Time spent: %f secs\n",((float) t2 - t1) / CLOCKS_PER_SEC);
    }
}