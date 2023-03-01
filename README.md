# DPLL-WalkSAT
Implementation of the WalkSAT and DPLL algorithms, designed for the AI class which I attended in University.

## Compilation
The main program is the bcsp-mod.c. The math libary is used so it needs to be compiled as follows:

`gcc bcsp-mod.c -lm -o <out-name>`

so that it can be linked into the binary.

**Note**: that the two algorithms are implemented in two seperate header files, included in the main program.

## Usage
For the compiled main program to run properly, 3 arguments must be provided via the command line. The syntax is as follows:

`<out-name> <type-of-algorithm> <input-file> <output-file>`

Where `<type-of-algorithm>` can either be `dpll` or `walk`. The input file must have a very specific format. In its first line, 3 numbers must be present seperated by a blank space character, named N, M, K accordingly, where M determines the number of clauses contained in the conjuctive normal form (CNF), K the number of literals in each clause and N determines the number of the problem's symbols. 

So an example of such an input file is:

```
4 3 3
-1 2 3
-4 1 2
-3 -1 -2
```

Symbols are represented by a value from 1 to N, while literals are represented by a number from -N to N excluding 0. The `-` mark means that the symbol appears in the clause as a negative literal, while the absence of the same mark means that the symbol appears as a positive literal in the clause.
