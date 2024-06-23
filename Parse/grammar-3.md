# LL(1) cflat grammar

```
# cflat types; `&` is right-associative.
   type ::= `&`* type_ad
type_ad ::= `int`
          | id
          | `(` type_op
type_op ::= `)` type_ar
          | type type_fp
type_fp ::= `)` type_ar?
          | (`,` type)+ `)` type_ar
type_ar ::= `->` rettyp

# function type (used in extern declarations).
funtype ::= `(` (type (`,` type)*)? `)` `->` rettyp
 rettyp ::= type 
          | `_`

# cflat program.
 program ::= toplevel+
toplevel ::= glob      # global variable declaration
           | typedef   # struct type definition
           | extern    # external function declaration
           | fundef    # function definition

# global variable declaration.
glob ::= `let` decls `;`

# struct type definition.
typdef ::= `struct` id `{` decls `}`

# variable / field declaration(s).
 decl ::= id `:` type
decls ::= decl (`,` decl)*

# external function declaration.
extern ::= `extern` id `:` funtype `;`

# function definition.
fundef ::= `fn` id `(` decls? `)` `->` rettyp `{` let* stmt+ `}`

# local variable declaration / initialization.
let ::= `let` decl (`=` exp)? (`,` decl (`=` exp)?)* `;`

# statement.
stmt ::= cond               # conditional
       | loop               # loop
       | assign_or_call `;` # assignment or function call
       | `break` `;`        # break out of a loop
       | `continue` `;`     # continue to next iteration of loop
       | `return` exp? `;`  # return from function

# a conditional with optional else clause.
cond ::= `if` exp block (`else` block)?

# loops: 'while' loop.
loop ::= `while` exp block 

# a block of statements.
block ::= `{` stmt* `}`

# assignment or function call.
assign_or_call ::= lval gets_or_args
  gets_or_args ::= `=` rhs
                 | `(` args? `)`
           rhs ::= exp 
                 | `new` type exp?

# lvalues for assignments and call statements. for lval `*` is right-
# associative, access is left-associative; access binds tighter than `*`, 
# e.g., `*id[2]` means `*(id[2])` and to get `(*id)[2]` we must write 
# `id[0][2]`.
  lval ::= `*`* id access*
access ::= `[` exp `]` 
         | `.` id

# call arguments.
args ::= exp (`,` exp)*

# expression; all binary operators and exp_ac are left-associative, all unary
# operators are right-associative. exp_ac binds tighter than `*`, e.g.,
# `*id[2]` means `*(id[2])`; to get `(*id)[2]` we need to write `id[0][2]`.
   exp ::= exp_p4 (binop_p3 exp_p4)*
exp_p4 ::= exp_p3 (binop_p2 exp_p3)*
exp_p3 ::= exp_p2 (binop_p1 exp_p2)*
exp_p2 ::= unop* exp_p1
exp_p1 ::= num
         | `nil`
         | `(` exp `)`
         | id exp_ac*
exp_ac ::= `[` exp `]`
         | `.` id
         | `(` args? `)`

# unary operators.
unop ::= `*`
       | `-`

# binary operators (from highest to lowest precedence).
binop_p1 ::= `*` | `/`
binop_p2 ::= `+` | `-`
binop_p3 ::= `==` | `!=` | `<` | `<=` | `>` | `>=`
```