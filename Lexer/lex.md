# Assignment 1: Lexer

__RELEASED:__ Tuesday, Mar 9
__DUE:__ Friday, Mar 19
__LATE DEADLINE 1:__ Friday, Mar 26 (-11%)
__LATE DEADLINE 2:__ Friday, Jun 14 (-31%)

You may work in groups of 1--3 people. If working in a group of 2--3, be sure to indicate that when submitting to Gradescope.

## Description

Implement a lexer for the Cflat language. Your lexer should contain an NFA data structure that translates lexemes to tokens (skipping whitespace and comments) using the maximal munch and priority mechanisms for dealing with ambiguity. The token desciptions are given as regular expressions; you can either manually convert them to NFA and then transcribe the NFA into code, or you can create a regular expression data structure and transcribe the regular expressions directly using that, then automatically convert the regular expression data structures to NFAs using the standard algorithm. While most of the regular expressions are simple, the comment regular expressions are fairly complicated (still doable manually, but not easily).

There is a `<regex>` header in the C++ standard library for regular expressions; you are _not_ allowed to use it (and the autograder will be checking).

Here are the tokens and their regular expression descriptions; the tokens include an `Error` token to use for invalid lexemes (i.e., anything that doesn't match one of the token descriptions):

```
TOKEN        REGEX
---------------------------------------------
Num          [0-9]+
Id           [a-zA-Z][a-zA-Z0-9]*
Int          int
Struct       struct
Nil          nil
Break        break
Continue     continue
Return       return
If           if
Else         else
While        while
New          new
Let          let
Extern       extern
Fn           fn
Address      &
Colon        :
Semicolon    ;
Comma        ,
Underscore   _
Arrow        ->
Plus         +
Dash         -
Star         *
Slash        /
Equal        ==
NotEq        !=
Lt           <
Lte          <=
Gt           >
Gte          >=
Dot          .
Gets         =
OpenParen    (
CloseParen   )
OpenBracket  [
CloseBracket ]
OpenBrace    {
CloseBrace   }
Error        <no description>
```

Here are the regular expression descriptions of whitespace and comments (in the C-style comment regexes `*` is used for a literal asterisk and `⋆` is used for kleene star):

```
        whitespace = (' ' | '\t' | '\n' | '\r')+
       c++-comment = //[^'\n']*
         c-comment = /*([^*]|*⋆[^*/])⋆*+/
unclosed-c-comment = /*([^*]|(*+[^*/]))⋆*⋆
```

The `Id` and `Num` tokens should contain the matched lexeme (i.e., what the identifier/number actually was) as a pair `(index, length)` giving the starting position of the lexeme in the input string and how many characters it is.

## Input/Output Specifications

The input will be a file containing a sequence of ASCII characters (i.e., the `char` type). Any valid ASCII characters may be present in the file. You are to read in the contents of this file as a string and then attempt to lex the string.

The output will be a series of tokens (one per line), with the `Num` and `Id` tokens displaying their matching lexeme (e.g., `Num(10)` or `Id(foo)`).

## Reference Solution

I have placed an executable of my own solution on CSIL in `~benh/160/lex`. You can use this reference solution to test your code before submitting. If you have any questions about the output format or the behavior of the lexer you can answer them using this reference solution as well; this is the solution that Gradescope will use to test your submissions.

## Submitting to Gradescope

The autograder is running Ubuntu 22.04 and has the latest `build-essential` package installed (including `g++` version 11.4.0 and the `make` utility). You may not use any other libraries or packages for your code. Your submission must meet the following requirements:

- There must be a `makefile` file s.t. invoking the `make` command builds your code, resulting in an executable called `lex`. The `lex` executable must take a single argument (the file to run the lexer on) and produce its output on standard out.

- If your solution contains sub-directories then the entire submission must be given as a zip file (this is required by Gradescope for submissions that contain a directory structure). The `makefile` should be in the root directory of the solution.

- The submitted files are not allowed to contain any binaries, and your solution is not allowed to use the network.

## Grading

The grading will be done across a series of test suites, each worth a certain percentage of the overall grade. Here are the test suites for this assignment:

- TS1: files containing unambiguous keywords, punctuation, and whitespace: 20 points

- TS2: files containing potentially ambiguous keywords, punctuation, and whitespace: 20 points

- TS3: files containing keywords, punctuation, whitespace, and numbers: 20 points

- TS4: files containing keywords, punctuation, whitespace, numbers, and identifiers: 20 points

- TS5: files containing keywords, punctuation, whitespace, numbers, identifiers, and C++-style comments: 10 points

- TS6: files containing keywords, punctuation, whitespace, numbers, identifiers, and C/C++-style comments: 10 points

All test suites may contain files with invalid lexemes (i.e., characters that do not match descriptions of Cflat tokens).
