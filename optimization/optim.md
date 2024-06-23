# Assignment 5: LIR Optimization

__RELEASED:__ Tuesday, May 28
__DUE:__ Tuesday, Jun 11
__LATE DEADLINE:__ Friday, Jun 14 (-11%)

You may work in groups of 1--3 people. If working in a group of 2--3, be sure to indicate that when submitting to Gradescope.

## Description

Implement the integer constant propagation optimization for LIR functions. You may assume there are no globals, structs, pointers, or function calls in the analyzed function, i.e., your implementation only needs to handle integer-typed variables and the instructions `$arith`, `$cmp`, `$copy`, `$branch`, `$jump`, and `$ret`. The LIR program given to optimize will contain a function called `test` (the parameters and return type may vary, but the name will always be the same); this is the function that you should optimize.

The first thing you need to do is to analyze the `test` function using DFA, as described in lecture, but with one tweak that allows us to get the full potential of the constant propagation optimization. Here is the DFA algorithm again, including that tweak:

1. Create `IN_bb` and `OUT_bb` for each basic block `bb`, with `IN_entry` initialized to `init` and all the rest initialied to `bot`.

2. Create a worklist of basic blocks initialized to contain only `entry`, **not** all basic blocks.

3. While the worklist is not empty:

    a. Pop a basic block `bb` from the worklist.

    b. Set `IN_bb` = join(`OUT_pred`) s.t. `pred` is a predecessor of `bb` in the CFG.

    c. Copy `IN_bb` into `store`.

    d. Iterate through all the instruction in `bb` updating `store` as appropriate. Note that you need to create the abstract versions of the arithmetic and relational operators; only `add` and `lte` were given in lecture. Remember to take advantage of arithmetic identities; you can use the solution binary on CSIL to help determine which ones to use.

    e. At the terminal instruction, if `store` is not equal to `OUT_bb`:

        1. Set `OUT_bb` = `store`.

        2. If the terminal is `$jump` then add the target basic block to the worklist.

        3. If the terminal is `$branch` and the operand is a constant, add only the relevant target basic block to the worklist; if the operand is ⊤ then add both target basic blocks to the worklist; if the operand is ⊥ then don't add either target basic block to the worklist.

The effect of this tweak is that, since constant propagation can sometimes allow us to statically determine which branch can be taken, we should only analyze basic blocks that can actually be reached (instead of all basic blocks).

The second thing you need to do is to transform the instructions in `test` based on the results of the analysis:

1. For `x = $arith ...`, `x = $cmp ...`, and `x = $copy ...`: if `x` is a constant value `k` immediately _after_ that instruction is executed, replace that instruction with `x = $copy k`.

2. For `x = $arith ...` if (1) didn't apply: if we're adding or subtracting by 0, or we're multiplying or dividing by 1, replace that instruction with `x = $copy op` where `op` is the relevant operand.

3. For `$branch op tt ff` where `op` is a constant value, replace the `$branch` with `$jump bb` where `bb` is the relevant target basic block (`tt` or `ff` depending on the value of `op`).

4. After transforming all instructions, remove any unreachable basic blocks from the function (this is the same as the last step of lowering from assign-3).

Note that we don't transform instructions based on ⊥ values (e.g., if we have `x = $copy y` and we know `y` is ⊥ we leave it unchanged); note especially that if a `$branch` operand is ⊥ then we leave it unchanged and the target basic blocks are treated as reachable for the purposes of the third step. This is a simplification to make the assignment easier---we could potentially create an `error` basic block and transform such instructions into jumps to that `error` block, but we won't bother with that here.

## Input and Output

The input will be three files given as command-line arguments:

- The first input file is the LIR in JSON format. You can use the same C++ header for reading in JSON files as for assignments 3 and 4 (see the #assign-3 channel on Slack). Some notes on the LIR JSON format:

    - Every variable has a name, type, and optional scope. If the scope is present then it specifies the function that the variable belongs to. If the scope is not present then the variable is a global.

    - The Alloc instruction has an extra field called `id`; you can ignore this field, it isn't relevant for what we're doing.

- As a convenience, the second input file is the same program as a token stream, in the same format as used for the input of assign-2. If you are confident in your assign-2 and assign-3 solutions (the AST construction part of assign-2 and the lowering from assign-3), then you can reuse them to construct the LIR from the tokens instead of having to read in the LIR in JSON format.

- As a convenience, the third input file is the same program as an AST in JSON format, as used for assign-3. If you are confident in your assign-3 solution, then you can reuse it to construct the LIR from the AST instead of having to read in the LIR in JSON format.

All three files will be given as command-line arguments (e.g., `./codegen file_lir file_toks file_ast`); you only need to use one of them, they are all the same program just in different formats.

The output is the optimized version of the `test` function as an LIR data structure (the same output format as used for assign-3); it should be printed to standard out.

## Reference Solution

I have placed an executable of my own solution on CSIL in `~benh/160/opt`. My solution only takes one file as an argument; that file must contain LIR in either JSON format _or_ the human-readable format used in lecture. Recall that the `lower` executable on CSIL from assign-3 has been modified to take either the `-json` flag (output the LIR as JSON) or the `-hr` flag (output the LIR in human-readable format). You can use this reference solution to test your code before submitting. If you have any questions about the output format or the behavior of the code generator you can answer them using this reference solution as well; this is the solution that Gradescope will use to test your submissions.

## Submitting to Gradescope

The autograder is running Ubuntu 22.04 and has the latest `build-essential` package installed (including `g++` version 11.4.0 and the `make` utility). You may not use any other libraries or packages for your code. Your submission must meet the following requirements:

- There must be a `makefile` file s.t. invoking the `make` command builds your code, resulting in an executable called `opt`. The `opt` executable must take three arguments (the files containing the program to run the optimizer on, in the three different formats) and produce its output on standard out.

- If your solution contains sub-directories then the entire submission must be given as a zip file (this is required by Gradescope for submissions that contain a directory structure). The `makefile` should be in the root directory of the solution.

- The submitted files are not allowed to contain any binaries, and your solution is not allowed to use the network.

## Grading

The grading will consist of seven test suites:

- TS1: no `$cmp`, `$branch`, or `$jump`, using `add` and `sub` : 10 points
- TS2: no `$cmp`, `$branch`, or `$jump`, using `mul` and `div` : 10 points
- TS3: no `$arith`, `$branch`, or `$jump`, using `eq` and `neq` : 10 points
- TS4: no `$arith`, `$branch`, or `$jump`, using `lt` and `lte` : 10 points
- TS5: no `$arith`, `$branch`, or `$jump`, using `gt` and `gte` : 10 points
- TS6: all instructions and operators, no loops : 25 points
- TS7: all instructions and operators, with loops : 25 points
