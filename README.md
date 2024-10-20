# Sudoku Solver using DPLL Algorithm

This project implements a Sudoku solver using the DPLL (Davis–Putnam–Logemann–Loveland) algorithm. It can parse Sudoku puzzles provided via command-line inputs or from a BNF (Backus–Naur Form) input file. The solver converts Sudoku constraints into CNF (Conjunctive Normal Form) clauses and then applies the DPLL algorithm to find a solution.

## Files

- `cnf_library.c`, `cnf_library.h`: Functions to handle CNF input parsing, conversion from BNF to CNF, and related data structures.
- `dpll_solver.c`, `dpll_solver.h`: Implementation of the DPLL algorithm for solving SAT (Satisfiability) problems.
- `main.c`: The entry point of the program that manages input parsing and runs the solver.
- `ex_bnf.txt`: Example input file in BNF format demonstrating logical constraints.
- `CMakeLists.txt`: Configuration file for building the project using CMake.

## Compilation

To compile the project, you need to have CMake and a C compiler (such as GCC) installed on your system.

```bash
mkdir build
cd build
cmake ..
make
```

This will generate an executable called `sudoku` in the `build` directory.

## Usage

The solver 'sudoku' can take various forms of input, including command-line arguments or a BNF file. Below are the examples of how to run the solver with different types of input.

### 1. Direct variable assignments

```bash
./sudoku 11=4 12=2 14=8 15=7 16=5 17=3 19=6 21=3 22=9 23=8 25=4 26=6 27=7 29=1 33=5 37=8 39=2 41=9 43=6 44=4 45=5 56=1 58=8 61=1 62=4 67=6 75=2 76=4 77=5 78=3 82=3 84=1 85=8 88=6 93=9 97=4
```


### 2. Verbose mode with direct variable assignments

```bash
./sudoku -v 11=9 14=6 15=7 16=2 21=2 26=1 27=4 39=8 44=1 51=7 52=4 54=3 55=9 58=8 63=6 66=4 78=2 79=9 89=1 91=5 92=6 93=1 97=7
```

### 3.BNF conversion

```bash
./sudoku -bnf 11=5 12=3 14=6 19=9 22=4 27=7 37=3 38=1 39=6 45=2 48=6 49=5 51=9 53=2 56=5 58=7 63=5 66=3 67=9 71=4 73=6 74=1 75=5 82=5 84=2 85=8 87=6 92=1 95=9 97=5
```

### 4. Verbose mode with BNF conversion

```bash
./sudoku -v -bnf 11=5 13=7 15=6 16=1 17=9 26=9 29=4 31=8 42=4 45=1 52=8 57=7 61=7 63=1 64=5 69=3 71=9 73=6 75=5 77=4 82=1 94=3 98=2
```

### 4. BNF input from a file

```bash
./sudoku -bnf ../ex_bnf.txt
```

### 5. Verbose mode with BNF input from a file

```bash
./sudoku -v -bnf ../ex_bnf.txt
```