# AST for CFlat

- type aliases (to make things easier to understand)

```
StructId = string
FuncId = string
```

- the AST data types

```
Type
| Int
| Struct { name: StructId }
| Fn { prms: vector<Type>, ret: option<Type> }
| Ptr { ref: Type }

Program
- globals: vector<Decl>
- structs: vector<Struct>
- externs: vector<Decl>
- functions: vector<Function>

Decl
- name: string
- type: Type

Struct
- name: StructId
- fields: vector<Decl>

Function
- name: FuncId
- params: vector<Decl>
- rettyp: option<Type>
- locals: vector<(Decl, option<Exp>)>
- stmts: vector<Stmt>

Stmt
| Break
| Continue
| Return { exp: option<Exp> }
| Assign { lhs: Lval, rhs: Rhs }
| Call { callee: Lval, args: vector<Exp> }
| If { guard: Exp, tt: vector<Stmt>, ff: vector<Stmt> }
| While { guard: Exp, body: vector<Stmt> }

Rhs
| RhsExp { exp: Exp }
| New { type: Type, amount: Exp }

Lval
| Id { name: string }
| Deref { lval: Lval }
| ArrayAccess { ptr: Lval, index: Exp }
| FieldAccess { ptr: Lval, field: string }

Exp
| Num { n: int32_t }
| Id { name: string }
| Nil
| UnOp { op: UnaryOp, operand:Exp }
| BinOp { op: BinaryOp, left: Exp, right: Exp }
| ArrayAccess { ptr: Exp, index: Exp }
| FieldAccess { ptr: Exp, field: string }
| Call { callee: Exp, args: vector<Exp> }

UnaryOp
| Neg
| Deref

BinaryOp
| Add
| Sub
| Mul
| Div
| Equal
| NotEq
| Lt
| Lte
| Gt
| Gte
```
