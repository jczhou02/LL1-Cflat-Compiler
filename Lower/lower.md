# Assignment 2: Lowering to LIR

__RELEASED:__ Friday, May 3
__DUE:__ Tuesday, May 14
__LATE DEADLINE 1:__ Tuesday, May 21 (-11%)
__LATE DEADLINE 2:__ Friday, Jun 14 (-31%)

You may work in groups of 1--3 people. If working in a group of 2--3, be sure to indicate that when submitting to Gradescope.

## Description

Implement the compiler stage that lowers an AST data structure into an LIR data structure, using the algorithm described in lecture. The input AST is guaranteed to be valid.

The input will be two files:

- The first input file is the AST of the program to lower, in JSON format. A C++ header for reading in JSON files will be posted for you to use.

- As a convenience, the second input file is the same program as a token stream, in the same format as used for the input of assign-2. If you are confident in your assign-2 solution (the AST construction part, specifically), then you can reuse it to construct the AST from the tokens instead of having to read in the AST in JSON format.

The output will be the resulting LIR data structure, in a format similar (but not identical) to that used for the AST output from assign-2. For all maps (which is most things in the LIR data structure), their contents should be output in lexicographic order of their keys. See the solution binary on CSIL for examples of what the output should look like. Your output must match the newlines in my solution (except for completely blank lines), but otherwise whitespace will be ignored (i.e., the grader will be using `diff -wB` to compare your output to the solution).

## Reference Solution

I have placed an executable of my own solution on CSIL in `~benh/160/lower`. You can use this reference solution to test your code before submitting. If you have any questions about the output format or the behavior of the code generator you can answer them using this reference solution as well; this is the solution that Gradescope will use to test your submissions.

## Submitting to Gradescope

The autograder is running Ubuntu 22.04 and has the latest `build-essential` package installed (including `g++` version 11.4.0 and the `make` utility). You may not use any other libraries or packages for your code. Your submission must meet the following requirements:

- There must be a `makefile` file s.t. invoking the `make` command builds your code, resulting in an executable called `lower`. The `lower` executable must take two arguments (both are filenames, the first containing the AST to lower in JSON format and the second containing tokens that parse to that same AST) and produce its output on standard out.

- If your solution contains sub-directories then the entire submission must be given as a zip file (this is required by Gradescope for submissions that contain a directory structure). The `makefile` should be in the root directory of the solution.

- The submitted files are not allowed to contain any binaries, and your solution is not allowed to use the network.

## Grading

The grading will consist of seven test suites:

- TS1: no {pointers, structs, function calls, continue/break}; a single return : 15 points

- TS2: TS1 but with pointers : 15 points

- TS3: TS2 but with structs : 10 points

- TS4: TS1 but with function calls : 15 points

- TS5: TS1 but with continue/break : 15 points

- TS6: TS5 but with multiple returns : 15 points

- TS7: all valid programs : 15 points
