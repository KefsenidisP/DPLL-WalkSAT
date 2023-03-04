/*
    Input file generator, for the bcsp-mod file.

    Kefsenidis Paraskevas, 2023

    Note: I created this file, because I did not want to upload
          the exercise's given file generator.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define PREFIX_SIZE 20

long M;
long N;
long K;

void syntax_message()
{
    printf("Wrong number of arguments\n");
    printf("Correct syntax is the following:\n");
    printf("./<binary-name> <M> <N> <K> <number-of-problems-to-generate> <file-name>\n");
    printf("File naming is <file-name>_x.txt, where x represnts a number, starting from 1\n");
    printf("The <number-of-problems-to-generate>, must be a positive number\n");
    printf("Note: N must be equal or bigger than K.");
}

// Checks if the random generator, has already created
// this value.
int same_val(int genum, int problem[M][K], int clause_num, int litlim)
{
    for(int j = 0; j < litlim; j++)
        if(abs(problem[clause_num][j]) == genum)
            return 1;

    return 0;
}

// Create a random problem
void random_problem(int problem[M][K])
{
    for(int i = 0; i < M; i++)
    {
        int genum;

        for(int j = 0; j < K; j++)
        {
            do
            {
                genum = rand() % N + 1; // Exclude 0 and reach to N
            } while(same_val(genum, problem, i, j));

            // Randomly select if it will be a negative or positive literal
            int p = rand() % 10;

            problem[i][j] = ((p < 5) ? (-genum) : (genum));
        }
    }
}

// Write the problem to a file.
void create_file(int fnum, char *fprefix, int problem[M][K])
{
    FILE *fp;
    char fname[100];

    snprintf(fname, sizeof(fname), "%s_%d.txt", fprefix, fnum + 1);

    fp = fopen(fname, "w");

    fprintf(fp, "%d %d %d\n", N, M, K);

    for(int i = 0; i < M; i++)
    {
        int j;
        for(j = 0; j < K - 1; j++)
            fprintf(fp, "%d ", problem[i][j]);

        fprintf(fp, "%d\n", problem[i][j]);
    }

    fclose(fp);
}

// Create n random problem files
void problem_creator(int n, char *fname)
{
    for(int i = 0; i < n; i++)
    {
        int problem[M][K];

        random_problem(problem);
        create_file(i, fname, problem);
    }
}

int main(int argc, char **argv)
{
    int n;  // Number of problems to be generated

    srand((unsigned) time(NULL));

    if(argc != 6)
    {
        syntax_message();
        exit(0);
    }

    M = strtol(argv[1], NULL, 10);
    N = strtol(argv[2], NULL, 10);
    K = strtol(argv[3], NULL, 10);
    n = strtol(argv[4], NULL, 10);



    if(n <= 0 || K > N)
        syntax_message();
    else
        problem_creator(n, argv[5]);

    return 0;
}