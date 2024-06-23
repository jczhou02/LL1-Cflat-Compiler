# Assignment 2: Parser and Validator

__RELEASED:__ Friday, Apr 19
__DUE:__ Friday, May 3 [EXTENDED DEADLINE]
__LATE DEADLINE 1:__ Friday, May 10 (-11%) [EXTENDED DEADLINE]
__LATE DEADLINE 2:__ Friday, Jun 14 (-31%)

You may work in groups of 1--3 people. If working in a group of 2--3, be sure to indicate that when submitting to Gradescope.

## Description

Implement a LL(1) recursive descent parser for Cflat, using the provided grammar (which is guaranteed to be LL(1)). The parser takes a series of tokens and should either (1) emit an error message for a syntactically incorrect input, or (2) emit an AST for a syntactically correct input. If the result is an AST, the parser should also type-check the AST and output a list of any type-checking errors.

The input will be a file containing a sequence of tokens in textual form, one per line. There will be no Error tokens in the input. All syntactically correct inputs will obey the naming and miscellaneous validation rules, you only need to check for typing errors.

The output will be one of:

1. The AST of the parsed program for a syntactically correct input, along with any typing errors (with the typing errors listed in lexicographic order).

2. An error message for a syntactically incorrect input, specifying the index of the token where the error was detected. NOTE: do not use an abnormal exit for a parsing error (e.g., `exit(1)`); the autograder will think the abnormal exit is due to a segfault (or similar error) and will count the test as incorrect.

Your output will be compared against the reference solution using `diff -wB`, so whitespace is not important (e.g., you don't need to nicely indent your ASTs to match the reference solution in order to get a correct result).

You should find all possible type errors, not stopping at the first one. If a rule fails, then after emitting an error you can assume the result has whatever type it was supposed to have for the purposes of continuing the type checking process. An easy way to do this when the rule doesn't make clear what type is expected as the result is to create a special type `ANY` that will test as equal to any type. Then the type of `nil` can be `&ANY`. If you need to print out the `ANY` type it should appear as `_`, and the type of `nil` should appear as `&_`.

## Typing Errors

The typing errors output by your type checker are based on the typing rules in the `validation.pdf` handout:

- `[BINOP-REST] in function {}: operand has type {} instead of int`

- `[BINOP-EQ] in function {}: operands with different types: {} vs {}`

- `[BINOP-EQ] in function {}: operand has non-primitive type {}`

- `[ID] in function {}: variable {} undefined`

- `[NEG] in function {}: negating type {} instead of int`

- `[DEREF] in function {}: dereferencing type {} instead of pointer`

- `[ARRAY] in function {}: array index is type {} instead of int`

- `[ARRAY] in function {}: dereferencing non-pointer type {}`

- `[FIELD] in function {}: accessing field of incorrect type {}`

- `[FIELD] in function {}: accessing non-existent field {} of struct type {}`

- `[FIELD] in function {}: accessing field of non-existent struct type {}`

- `[ECALL-INTERNAL] in function {}: calling main`

- `[ECALL-INTERNAL] in function {}: calling a function with no return value`

- `[ECALL-INTERNAL] in function {}: call number of arguments ({}) and parameters ({}) don't match`

- `[ECALL-INTERNAL] in function {}: call argument has type {} but parameter has type {}`

- `[ECALL-EXTERN] in function {}: calling a function with no return value`

- `[ECALL-EXTERN] in function {}: call number of arguments ({}) and parameters ({}) don't match`

- `[ECALL-EXTERN] in function {}: call argument has type {} but parameter has type {}`

- `[ECALL-*] in function {}: calling non-function type {}`

- `[BREAK] in function {}: break outside of loop`

- `[CONTINUE] in function {}: continue outside of loop`

- `[RETURN-1] in function {}: should return nothing but returning {}`

- `[RETURN-2] in function {}: should return {} but returning nothing`

- `[RETURN-2] in function {}: should return {} but returning {}`

- `[ASSIGN-EXP] in function {}: assignment to struct or function`

- `[ASSIGN-EXP] in function {}: assignment lhs has type {} but rhs has type {}`

- `[ASSIGN-NEW] in function {}: allocating function type {}`

- `[ASSIGN-NEW] in function {}: assignment lhs has type {} but we're allocating type {}`

- `[ASSIGN-NEW] in function {}: allocation amount is type {} instead of int`

- `[SCALL-INTERNAL] in function {}: calling main`

- `[SCALL-INTERNAL] in function {}: call number of arguments ({}) and parameters ({}) don't match`

- `[SCALL-INTERNAL] in function {}: call argument has type {} but parameter has type {}`

- `[SCALL-EXTERN] in function {}: call number of arguments ({}) and parameters ({}) don't match`

- `[SCALL-EXTERN] in function {}: call argument has type {} but parameter has type {}`

- `[SCALL-*] in function {}: calling non-function type {}`

- `[IF] in function {}: if guard has type {} instead of int`

- `[WHILE] in function {}: while guard has type {} instead of int`

- `[GLOBAL] global {} has a struct or function type`

- `[STRUCT] struct {} field {} has a struct or function type`

- `[FUNCTION] in function {}: variable {} has a struct or function type`

- `[FUNCTION] in function {}: variable {} with type {} has initializer of type {}`

## Examples

- EXAMPLE 1

Input
```
Fn
Id(main)
OpenParen
CloseParen
Arrow
Int
OpenBrace
Return
Num(42)
Semicolon
CloseBrace
```

Output
```
Program(
  globals = [],
  structs = [],
  externs = [],
  functions = [
    Function(
      name = main,
      params = [],
      rettyp = Int,
      locals = [],
      stmts = [
        Return(
          Num(42)
        )
      ]
    )
  ]
)
```

- EXAMPLE 2

Input
```
Fn
Id(main)
OpenParen
CloseParen
Arrow
Int
OpenBrace
Return
Nil
Semicolon
CloseBrace
```

Output
```
Program(
  globals = [],
  structs = [],
  externs = [],
  functions = [
    Function(
      name = main,
      params = [],
      rettyp = Int,
      locals = [],
      stmts = [
        Return(
          Nil
        )
      ]
    )
  ]
)

[RETURN-2] in function main: should return int but returning &_
```

- EXAMPLE 3

Input
```
Fn
Id(main)
OpenParen
CloseParen
Gt
Int
OpenBrace
Return
Num(42)
Semicolon
CloseBrace
```

Output
```
parse error at token 4
```

## Reference Solution

I have placed an executable of my own solution on CSIL in `~benh/160/parse`. You can use this reference solution to test your code before submitting. If you have any questions about the output format or the behavior of the parser you can answer them using this reference solution as well; this is the solution that Gradescope will use to test your submissions.

## Submitting to Gradescope

The autograder is running Ubuntu 22.04 and has the latest `build-essential` package installed (including `g++` version 11.4.0 and the `make` utility). You may not use any other libraries or packages for your code. Your submission must meet the following requirements:

- There must be a `makefile` file s.t. invoking the `make` command builds your code, resulting in an executable called `parse`. The `parse` executable must take a single argument (the file to run the parser on) and produce its output on standard out.

- If your solution contains sub-directories then the entire submission must be given as a zip file (this is required by Gradescope for submissions that contain a directory structure). The `makefile` should be in the root directory of the solution.

- The submitted files are not allowed to contain any binaries, and your solution is not allowed to use the network.

## Grading

The grading will consist of three test suites:

- TS1: syntactically correct and valid programs: 40 points

- TS2: syntactically correct but invalid programs: 50 points

- TS3: syntactically incorrect programs: 10 points
