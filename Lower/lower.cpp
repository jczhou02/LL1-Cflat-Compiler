#include "json.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
using namespace std;
using json = nlohmann::json;

namespace AST {
struct Type {
  std::string name;
  std::vector<Type> params;
};

struct Expr;
struct Stmt;

struct Lval {
  std::string name;
  std::unique_ptr<Expr> array_index;
  std::unique_ptr<Expr> array_ptr;
  std::string field_name;
};

struct Expr {
  enum class Kind {
    Num,
    Id,
    Nil,
    UnOp,
    BinOp,
    Call,
    ArrayAccess,
    FieldAccess,
    New

  } kind;
  int num;
  std::string id;
  std::string unop;
  std::string binop;
  std::unique_ptr<Expr> left;
  std::unique_ptr<Expr> right;
  std::string callee;
  std::vector<std::unique_ptr<Expr>> args;
  std::unique_ptr<Expr> array_ptr;
  std::unique_ptr<Expr> array_index;
  std::unique_ptr<Expr> field_ptr;
  std::string field_name;
  std::unique_ptr<Expr> new_size;
  AST::Type new_type;
};

struct Stmt {
  enum class Kind { Assign, If, While, Call, Continue, Break, Return } kind;
  std::unique_ptr<Lval> assign_lhs;
  std::unique_ptr<Expr> assign_rhs;
  std::unique_ptr<Expr> if_guard;
  std::vector<std::unique_ptr<Stmt>> if_then;
  std::vector<std::unique_ptr<Stmt>> if_else;
  std::unique_ptr<Expr> while_guard;
  std::vector<std::unique_ptr<Stmt>> while_body;
  std::string call_callee;
  std::vector<std::unique_ptr<Expr>> call_args;
  std::unique_ptr<Expr> return_expr;
};

struct Function {
  std::string name;
  std::vector<std::pair<std::string, Type>> params;
  Type ret_type;
  std::vector<std::pair<std::string, Type>> locals;
  std::vector<std::unique_ptr<Stmt>> stmts;
};

struct Program {
  std::unordered_map<std::string, Type> globals;
  std::unordered_map<std::string, std::pair<std::vector<Type>, Type>> externs;
  std::unordered_map<std::string, std::vector<std::pair<std::string, Type>>>
      structs;
  std::vector<Function> functions;
};
bool compareTypes(const Type &t1, const Type &t2) { return t1.name < t2.name; }
} // namespace AST

namespace LIR {
struct Operand {
  enum class Kind { Var, Const } kind;
  std::string var;
  int constant;
};

struct Instruction {
  enum class Kind {
    Label,
    Branch,
    Jump,
    Copy,
    Alloc,
    Store,
    Load,
    Gep,
    Gfp,
    CallExt,
    CallDir,
    CallInd,
    Ret,
    Arith,
    Cmp
  } kind;
  std::string label;
  Operand branch_guard;
  std::string branch_tt;
  std::string branch_ff;
  std::string jump_target;
  Operand copy_lhs;
  Operand copy_rhs;
  Operand alloc_lhs;
  Operand alloc_size;
  Operand store_addr;
  Operand store_val;
  Operand load_lhs;
  Operand load_addr;
  Operand gep_lhs;
  Operand gep_ptr;
  Operand gep_idx;
  Operand gfp_lhs;
  Operand gfp_ptr;
  std::string gfp_field;
  Operand callext_lhs;
  std::string callext_name;
  std::vector<Operand> callext_args;
  Operand calldir_lhs;
  std::string calldir_name;
  std::vector<Operand> calldir_args;
  std::string calldir_next;
  Operand callind_lhs;
  Operand callind_ptr;
  std::vector<Operand> callind_args;
  std::string callind_next;
  Operand ret_val;
  Operand arith_lhs;
  std::string arith_op;
  Operand arith_op1;
  Operand arith_op2;
  Operand cmp_lhs;
  std::string cmp_op;
  Operand cmp_op1;
  Operand cmp_op2;
};

struct BasicBlock {
  std::string label;
  std::vector<Instruction> instructions;
};

struct FunctionBody {
  std::unordered_map<std::string, BasicBlock> basic_blocks;
};

struct Function {
  std::string name;
  std::vector<std::pair<std::string, std::string>> params;
  std::string ret_type;
  std::unordered_map<std::string, AST::Type> locals;
  FunctionBody body;
};

struct Program {
  std::unordered_map<std::string, std::string> globals;
  std::unordered_map<std::string,
                     std::pair<std::vector<std::string>, std::string>>
      externs;
  std::unordered_map<std::string,
                     std::vector<std::pair<std::string, AST::Type>>>
      structs;
  std::unordered_map<std::string, Function> functions;
};
} // namespace LIR
std::string freshLabel(int &counter);
void lowerProgram(const AST::Program &ast, LIR::Program &lir);
void lowerStatements(const std::vector<std::unique_ptr<AST::Stmt>> &stmts,
                     std::vector<LIR::Instruction> &translationVector,
                     int &counter, const std::string &loopStart = "",
                     const std::string &loopEnd = "");
LIR::Operand lowerExpression(const AST::Expr &expr,
                             std::vector<LIR::Instruction> &translationVector,
                             int &counter);
LIR::Operand lowerLval(const AST::Lval &lval,
                       std::vector<LIR::Instruction> &translationVector,
                       int &counter);
LIR::Operand lowerLvalAsExpr(const AST::Lval &lval,
                             std::vector<LIR::Instruction> &translationVector,
                             int &counter);
void constructCFG(const std::vector<LIR::Instruction> &translationVector,
                  LIR::FunctionBody &functionBody);
void outputLIR(const LIR::Program &lir);
AST::Program parseAST(const json &ast_json);
std::string formatType(const AST::Type &type);
// Helper functions for creating fresh variables and labels
std::vector<std::pair<std::string, std::string>> tempVars;
std::unordered_map<std::string, AST::Type> varTypes;
LIR::Program lir;
AST::Type defaultIntType() {
  AST::Type intType;
  intType.name = "Int";
  return intType;
}

std::string freshVar(int &counter, const AST::Type &type = defaultIntType()) {
  static int countert = 1;
  std::string tempVar = "_t" + std::to_string(countert++);
  varTypes[tempVar] = type;
  // cout<<"tempVar: "<<tempVar<<endl;
  // cout<<"type: "<<formatType(type)<<endl;
  tempVars.push_back(std::make_pair(tempVar, formatType(type)));
  return tempVar;
}

AST::Type getVarType(const std::string &var) {
  if (varTypes.find(var) != varTypes.end()) {
    return varTypes[var];
  }
  throw std::runtime_error("Type not found for variable: " + var);
}
AST::Type getType(const AST::Expr &expr) {
  switch (expr.kind) {
  case AST::Expr::Kind::Num:
    return {"Int"};
  case AST::Expr::Kind::Id:
    return getVarType(expr.id);
  case AST::Expr::Kind::ArrayAccess: {
    AST::Type ptrType = getType(*expr.array_ptr);
    if (ptrType.name == "Ptr" && !ptrType.params.empty()) {
      return ptrType.params[0];
    }
    throw std::runtime_error("Invalid ArrayAccess type.");
  }
  case AST::Expr::Kind::FieldAccess:
    // Handle struct field access type
    return getVarType(expr.field_name);
  case AST::Expr::Kind::New:
    if (expr.new_type.name == "Ptr") {
      return {"Ptr", {getType(*expr.new_size)}};
    }
    return {"Ptr", {{"Int"}}}; // Default to Int if no type provided
  default:
    throw std::runtime_error("Unknown expression type.");
  }
}

std::string freshLabel(int &counter) {
  return "lbl" + std::to_string(counter++);
}
AST::Type getFieldTypeFromStruct(const std::string &variable, const std::string &field);
// Lowering functions for programs, statements, expressions, and lvals
void lowerProgram(const AST::Program &ast, LIR::Program &lir) {
  // Copy globals, externs, and structs to LIR
  for (const auto &global : ast.globals) {
    lir.globals[global.first] = global.second.name;
  }

  for (const auto &extern_ : ast.externs) {
    std::vector<std::string> param_types;
    for (const auto &param : extern_.second.first) {
      param_types.push_back(param.name);
    }
    lir.externs[extern_.first] =
        std::make_pair(param_types, extern_.second.second.name);
  }

  for (const auto &struct_ : ast.structs) {
    std::vector<std::pair<std::string, AST::Type>> fields;
    for (const auto &field : struct_.second) {
      fields.push_back(std::make_pair(field.first, field.second));
    }
    lir.structs[struct_.first] = fields;
  }

  // Lower each function
  for (const auto &func : ast.functions) {
    LIR::Function lirFunc;
    lirFunc.name = func.name;

    // Copy function parameters to LIR
    for (const auto &param : func.params) {
      lirFunc.params.push_back(std::make_pair(param.first, param.second.name));
    }

    // Set the return type of the function
    lirFunc.ret_type = func.ret_type.name;

    // Copy function locals to LIR
    for (const auto &local : func.locals) {
      varTypes[local.first] = local.second;
      lirFunc.locals[local.first] = local.second;
      //  cout<<"AJ"<<formatType(local.second)<<endl;
    }
    // Create a fresh variable counter for the function
    int varCounter = 1;

    // Create a translation vector to hold the lowered instructions
    std::vector<LIR::Instruction> translationVector;
    translationVector.emplace_back(
        LIR::Instruction{LIR::Instruction::Kind::Label, "entry"});

    // Eliminate local variable initializations

    std::vector<std::unique_ptr<AST::Stmt>> stmts;

    for (auto &stmt : func.stmts) {
      stmts.emplace_back(std::make_unique<AST::Stmt>(std::move(*stmt)));
    }

    // Lower the function statements
    lowerStatements(stmts, translationVector, varCounter);

    // // Enforce having a single Return instruction
    // std::string exitLabel;
    // if (!translationVector.empty() && translationVector.back().kind ==
    // LIR::Instruction::Kind::Label) {
    //     exitLabel = translationVector.back().label;
    // } else {
    //     exitLabel = freshLabel(varCounter);
    //     translationVector.emplace_back(LIR::Instruction{LIR::Instruction::Kind::Label,
    //     exitLabel});
    // }

    // bool hasReturn = false;
    // for (auto& instr : translationVector) {
    //     if (instr.kind == LIR::Instruction::Kind::Ret) {
    //         hasReturn = true;
    //         break;
    //     }
    // }

    // if (hasReturn) {
    //     cout<<"has return"<<endl;
    //     LIR::Operand retVal;
    //     if (func.ret_type.name != "nil") {
    //         cout<<"ret type is not nil"<<endl;
    //         std::string retVarName = freshVar(varCounter);
    //         retVal = LIR::Operand{LIR::Operand::Kind::Var, retVarName};
    //         cout<<"retVal: "<<retVal.constant<<endl;
    //         lirFunc.locals[retVarName] = func.ret_type.name;

    //     } else {
    //         translationVector.emplace_back(LIR::Instruction{LIR::Instruction::Kind::Ret});
    //     }

    //     for (auto it = translationVector.begin(); it !=
    //     translationVector.end(); ++it) {
    //         if (it->kind == LIR::Instruction::Kind::Ret) {
    //             if (it->ret_val.kind == LIR::Operand::Kind::Var) {
    //                 std::string retVarName = it->ret_val.var;
    //                 it = translationVector.insert(it,
    //                 LIR::Instruction{LIR::Instruction::Kind::Copy,
    //                 retVal.var, LIR::Operand{LIR::Operand::Kind::Var,
    //                 retVarName}});
    //                 ++it;
    //             }
    //             it = translationVector.erase(it);
    //             translationVector.insert(it,
    //             LIR::Instruction{LIR::Instruction::Kind::Jump, exitLabel});
    //         }
    //     }
    // } else {
    //     translationVector.emplace_back(LIR::Instruction{LIR::Instruction::Kind::Ret});
    // }

    // Construct the CFG for the function body
    constructCFG(translationVector, lirFunc.body);

    // Add the lowered function to the LIR program
    lir.functions[lirFunc.name] = std::move(lirFunc);
  }
}
bool isId(const AST::Lval& lval) {
    return lval.array_index == nullptr && lval.field_name.empty() && lval.name != "Deref";
}

void lowerStatements(const std::vector<std::unique_ptr<AST::Stmt>> &stmts,
                     std::vector<LIR::Instruction> &translationVector,
                     int &counter, const std::string &loopStart,
                     const std::string &loopEnd) {
  for (const auto &stmt : stmts) {
    switch (stmt->kind) {
      case AST::Stmt::Kind::Assign: {
        if (stmt->assign_rhs->kind == AST::Expr::Kind::New) {
          // Check if lhs is Id(name)
          if (isId(*stmt->assign_lhs))  {
            LIR::Operand size = lowerExpression(*stmt->assign_rhs->new_size,
                                                translationVector, counter);
            translationVector.emplace_back(LIR::Instruction{
                LIR::Instruction::Kind::Alloc,
                .alloc_lhs = {LIR::Operand::Kind::Var, stmt->assign_lhs->name},
                .alloc_size = size});
          } else {
            // Create fresh variable w for allocation
            AST::Type ptrType = {"Ptr", {stmt->assign_rhs->new_type}};
            std::string tempVarAlloc = freshVar(counter, ptrType);
            
      
            // Lower the lhs expression
            LIR::Operand lhs = lowerLval(*stmt->assign_lhs, translationVector, counter);
            // Lower the size expression
            LIR::Operand size = lowerExpression(*stmt->assign_rhs->new_size,
                                                translationVector, counter);

            translationVector.emplace_back(LIR::Instruction{
                LIR::Instruction::Kind::Alloc,
                .alloc_lhs = {LIR::Operand::Kind::Var, tempVarAlloc},
                .alloc_size = size});
            translationVector.emplace_back(LIR::Instruction{
                LIR::Instruction::Kind::Store, .store_addr = lhs,
                .store_val = {LIR::Operand::Kind::Var, tempVarAlloc}});
          }
        } else {//JAssign(lhs, RhsExp(e))K

            //if lhs is Id(name)
          if (isId(*stmt->assign_lhs)) {
              LIR::Operand rhs = lowerExpression(*stmt->assign_rhs, translationVector, counter);

            translationVector.emplace_back(LIR::Instruction{
                LIR::Instruction::Kind::Copy,
                .copy_lhs = {LIR::Operand::Kind::Var, stmt->assign_lhs->name},
                .copy_rhs = rhs});
          } else {
            LIR::Operand lhs = lowerLval(*stmt->assign_lhs, translationVector, counter);
                LIR::Operand rhs = lowerExpression(*stmt->assign_rhs, translationVector, counter);

            translationVector.emplace_back(LIR::Instruction{
                LIR::Instruction::Kind::Store, .store_addr = lhs,
                .store_val = rhs});
          }
        }
        break;
      }
      case AST::Stmt::Kind::If: {
        std::string labelTrue = freshLabel(counter);
        std::string labelFalse = freshLabel(counter);
        std::string labelEnd = freshLabel(counter);

        LIR::Operand guard =
            lowerExpression(*stmt->if_guard, translationVector, counter);
        translationVector.push_back(LIR::Instruction{
            LIR::Instruction::Kind::Branch, .branch_guard = guard,
            .branch_tt = labelTrue, .branch_ff = labelFalse});

        translationVector.push_back(
            LIR::Instruction{LIR::Instruction::Kind::Label, .label = labelTrue});
        lowerStatements(stmt->if_then, translationVector, counter, loopStart,
                        loopEnd);
        translationVector.push_back(LIR::Instruction{LIR::Instruction::Kind::Jump,
                                                     .jump_target = labelEnd});

        translationVector.push_back(
            LIR::Instruction{LIR::Instruction::Kind::Label, .label = labelFalse});
        lowerStatements(stmt->if_else, translationVector, counter, loopStart,
                        loopEnd);
        translationVector.push_back(LIR::Instruction{LIR::Instruction::Kind::Jump,
                                                     .jump_target = labelEnd});

        translationVector.push_back(
            LIR::Instruction{LIR::Instruction::Kind::Label, .label = labelEnd});
        break;
      }
      case AST::Stmt::Kind::While: {
        std::string labelHeader = freshLabel(counter);
        std::string labelBody = freshLabel(counter);
        std::string labelEnd = freshLabel(counter);

        translationVector.push_back(LIR::Instruction{LIR::Instruction::Kind::Jump,
                                                     .jump_target = labelHeader});
        translationVector.push_back(LIR::Instruction{
            LIR::Instruction::Kind::Label, .label = labelHeader});
        LIR::Operand guard =
            lowerExpression(*stmt->while_guard, translationVector, counter);
        translationVector.push_back(LIR::Instruction{
            LIR::Instruction::Kind::Branch, .branch_guard = guard,
            .branch_tt = labelBody, .branch_ff = labelEnd});

        translationVector.push_back(
            LIR::Instruction{LIR::Instruction::Kind::Label, .label = labelBody});
        lowerStatements(stmt->while_body, translationVector, counter, labelHeader,
                        labelEnd);
        translationVector.push_back(LIR::Instruction{LIR::Instruction::Kind::Jump,
                                                     .jump_target = labelHeader});

        translationVector.push_back(
            LIR::Instruction{LIR::Instruction::Kind::Label, .label = labelEnd});
        break;
      }
      case AST::Stmt::Kind::Continue: {
        if (!loopStart.empty()) {
          translationVector.push_back(LIR::Instruction{
              LIR::Instruction::Kind::Jump, .jump_target = loopStart});
        }
        break;
      }
      case AST::Stmt::Kind::Break: {
        if (!loopEnd.empty()) {
          translationVector.push_back(LIR::Instruction{
              LIR::Instruction::Kind::Jump, .jump_target = loopEnd});
        }
        break;
      }
      case AST::Stmt::Kind::Return: {
        if (stmt->return_expr) {
          LIR::Operand retValue =
              lowerExpression(*stmt->return_expr, translationVector, counter);
          translationVector.push_back(
              LIR::Instruction{LIR::Instruction::Kind::Ret, .ret_val = retValue});
        } else {
          translationVector.push_back(
              LIR::Instruction{LIR::Instruction::Kind::Ret});
        }
        break;
      }
      default:
        break;
    }
  }
}
AST::Type getFieldTypeFromStruct(const std::string &variable, const std::string &field) {
    // Step 1: Get the type of the variable
    AST::Type varType = getVarType(variable);
    
    // Step 2: Ensure the type is a pointer to a struct
    if (varType.name != "Ptr" || varType.params.empty()) {
        throw std::runtime_error("Variable is not a pointer.");
    }
    AST::Type pointedType = varType.params[0];
    if (pointedType.name != "Struct") {
        throw std::runtime_error("Pointer does not point to a struct.");
    }

    // Step 3: Retrieve the struct name from the pointed type parameters
    if (pointedType.params.empty()) {
        throw std::runtime_error("Struct type does not have a name.");
    }
    std::string structName = pointedType.params[0].name;

    // Step 4: Retrieve the struct fields from the global LIR program's structs map
    auto structEntry = lir.structs.find(structName);
    if (structEntry == lir.structs.end()) {
        throw std::runtime_error("Struct definition not found: " + structName);
    }
    const auto &structFields = structEntry->second;

    // Step 5: Find the type of the field being accessed
    for (const auto &fieldEntry : structFields) {
        if (fieldEntry.first == field) {
            return fieldEntry.second;
        }
    }

    throw std::runtime_error("Field not found in struct: " + field);
}


LIR::Operand lowerExpression(const AST::Expr &expr,
                             std::vector<LIR::Instruction> &translationVector,
                             int &counter) {
  switch (expr.kind) {
  case AST::Expr::Kind::Nil: {
    return LIR::Operand{LIR::Operand::Kind::Const, .constant = 0};
  }
  case AST::Expr::Kind::Num: {
    return LIR::Operand{LIR::Operand::Kind::Const, .constant = expr.num};
  }
  case AST::Expr::Kind::Id:
    return LIR::Operand{LIR::Operand::Kind::Var, .var = expr.id};

  case AST::Expr::Kind::UnOp: {
    // cout<<"UnOp"<<endl;
    if (expr.unop == "Deref") {
      LIR::Operand operand =
          lowerExpression(*expr.left, translationVector, counter);
      AST::Type operandType = getVarType(operand.var);
      if (operandType.name != "Ptr" || operandType.params.empty()) {
        throw std::runtime_error("Invalid operand type for dereference.");
      }
      AST::Type derefType = operandType.params[0];
      //<<"UNOP CREATE VAR in IF"<<endl;
      std::string tempVar = freshVar(counter, derefType);
      translationVector.push_back(
          LIR::Instruction{LIR::Instruction::Kind::Load,
                           .load_lhs = {LIR::Operand::Kind::Var, tempVar},
                           .load_addr = operand});
      return LIR::Operand{LIR::Operand::Kind::Var, tempVar};
    } else {
    //  cout<<"UNOP CREATE VAR";
      std::string tempVar = freshVar(counter);
      LIR::Operand operand =
          lowerExpression(*expr.left, translationVector, counter);
      translationVector.push_back(LIR::Instruction{
          LIR::Instruction::Kind::Arith,
          .arith_lhs = {LIR::Operand::Kind::Var, tempVar},
          .arith_op = expr.unop,
          .arith_op1 = {LIR::Operand::Kind::Const, .constant = 0},
          .arith_op2 = operand});
      return {LIR::Operand::Kind::Var, tempVar};
    }
  }
  case AST::Expr::Kind::BinOp: {
    LIR::Operand lhs = lowerExpression(*expr.left, translationVector, counter);
    LIR::Operand rhs = lowerExpression(*expr.right, translationVector, counter);
    std::string tempVar = freshVar(counter);
    if (expr.binop == "Add" || expr.binop == "Sub" || expr.binop == "Mul" ||
        expr.binop == "Div") {
      translationVector.push_back(LIR::Instruction{
          LIR::Instruction::Kind::Arith,
          .arith_lhs = {LIR::Operand::Kind::Var, tempVar},
          .arith_op = expr.binop, .arith_op1 = lhs, .arith_op2 = rhs});
    } else if (expr.binop == "Equal" || expr.binop == "NotEq" ||
               expr.binop == "Lt" || expr.binop == "Lte" ||
               expr.binop == "Gt" || expr.binop == "Gte") {
      translationVector.push_back(LIR::Instruction{
          LIR::Instruction::Kind::Cmp,
          .cmp_lhs = {LIR::Operand::Kind::Var, tempVar}, .cmp_op = expr.binop,
          .cmp_op1 = lhs, .cmp_op2 = rhs});
    }
    return {LIR::Operand::Kind::Var, tempVar};
  }
  case AST::Expr::Kind::Call: {
    std::string tempVar = freshVar(counter);
    std::vector<LIR::Operand> args;
    for (const auto &arg : expr.args) {
      args.push_back(lowerExpression(*arg, translationVector, counter));
    }
    translationVector.push_back(
        LIR::Instruction{LIR::Instruction::Kind::CallDir,
                         .calldir_lhs = {LIR::Operand::Kind::Var, tempVar},
                         .calldir_name = expr.callee, .calldir_args = args});
    return {LIR::Operand::Kind::Var, tempVar};
  }
  case AST::Expr::Kind::ArrayAccess: {
    // cout<<"ArrayAccess"<<endl;
    LIR::Operand arrayPtr =
        lowerExpression(*expr.array_ptr, translationVector, counter);
    AST::Type arrayPtrType = getVarType(arrayPtr.var);
    if (arrayPtrType.name != "Ptr" || arrayPtrType.params.empty()) {
      throw std::runtime_error("Invalid array pointer type.");
    }
    AST::Type elementType = arrayPtrType.params[0];
    LIR::Operand idx =
        lowerExpression(*expr.array_index, translationVector, counter);

    std::string ptrVar = freshVar(counter, arrayPtrType);
    std::string elemVar = freshVar(counter, elementType);
    translationVector.push_back(
        LIR::Instruction{LIR::Instruction::Kind::Gep,
                         .gep_lhs = {LIR::Operand::Kind::Var, ptrVar},
                         .gep_ptr = arrayPtr, .gep_idx = idx});
    translationVector.push_back(
        LIR::Instruction{LIR::Instruction::Kind::Load,
                         .load_lhs = {LIR::Operand::Kind::Var, elemVar},
                         .load_addr = {LIR::Operand::Kind::Var, ptrVar}});
    return {LIR::Operand::Kind::Var, elemVar};
  }
      case AST::Expr::Kind::FieldAccess: {
      // Extract the struct pointer operand
      LIR::Operand ptr = lowerExpression(*expr.field_ptr, translationVector, counter);

      // Get the type of the field being accessed
      AST::Type fieldType = getFieldTypeFromStruct(ptr.var, expr.field_name);

      // Create fresh variables for the field pointer and the field value
      std::string fieldPtrVar = freshVar(counter, {"Ptr", {fieldType}});
      std::string fieldValueVar = freshVar(counter, fieldType);

      // Emit Gfp to get the field pointer
      translationVector.push_back(
          LIR::Instruction{LIR::Instruction::Kind::Gfp,
                           .gfp_lhs = {LIR::Operand::Kind::Var, fieldPtrVar},
                           .gfp_ptr = ptr, .gfp_field = expr.field_name});

      // Emit Load to get the value at the field pointer
      translationVector.push_back(
          LIR::Instruction{LIR::Instruction::Kind::Load,
                           .load_lhs = {LIR::Operand::Kind::Var, fieldValueVar},
                           .load_addr = {LIR::Operand::Kind::Var, fieldPtrVar}});
      
      // Return the fresh variable holding the field value
      return {LIR::Operand::Kind::Var, fieldValueVar};
    }case AST::Expr::Kind::New: {
    // Handle New expression
    std::string tempVar = freshVar(counter);
    LIR::Operand size =
        lowerExpression(*expr.new_size, translationVector, counter);
    translationVector.push_back(LIR::Instruction{
        LIR::Instruction::Kind::Alloc,
        .alloc_lhs = {LIR::Operand::Kind::Var, tempVar}, .alloc_size = size});
    return {LIR::Operand::Kind::Var, tempVar};
  }
  default:
    throw std::runtime_error(
        "Unrecognized expression kind in lowerExpression.");
  }
}

LIR::Operand lowerLval(const AST::Lval &lval,
                       std::vector<LIR::Instruction> &translationVector,
                       int &counter) {
  if (!lval.array_index && lval.field_name.empty() && lval.name != "Deref") {
    return LIR::Operand{LIR::Operand::Kind::Var, lval.name};
  } else if (lval.array_index) {
    LIR::Operand arrayPtr =
        lowerExpression(*lval.array_ptr, translationVector, counter);
    LIR::Operand index =
        lowerExpression(*lval.array_index, translationVector, counter);

    AST::Type arrayPtrType = getVarType(arrayPtr.var);
    if (arrayPtrType.name != "Ptr" || arrayPtrType.params.empty()) {
      throw std::runtime_error("Invalid array pointer type.");
    }

    std::string tempVar = freshVar(counter, arrayPtrType);
    translationVector.push_back(
        LIR::Instruction{LIR::Instruction::Kind::Gep,
                         .gep_lhs = {LIR::Operand::Kind::Var, tempVar},
                         .gep_ptr = arrayPtr, .gep_idx = index});
    return {LIR::Operand::Kind::Var, tempVar};
  } else if (lval.name == "Deref") {
    LIR::Operand ptr = lowerExpression(*lval.array_ptr, translationVector, counter);
    AST::Type ptrType = getVarType(ptr.var);
    if (ptrType.name != "Ptr" || ptrType.params.empty()) {
      throw std::runtime_error("Invalid pointer type for dereference.");
    }
    return ptr;
  } else if (!lval.field_name.empty()) {
    std::string structVar = freshVar(counter);
    std::string fieldPtrVar = freshVar(counter);
    translationVector.push_back(
        LIR::Instruction{LIR::Instruction::Kind::Gfp,
                         .gfp_lhs = {LIR::Operand::Kind::Var, fieldPtrVar},
                         .gfp_ptr = {LIR::Operand::Kind::Var, lval.name},
                         .gfp_field = lval.field_name});
    return {LIR::Operand::Kind::Var, fieldPtrVar};
  }
  return LIR::Operand();
}

LIR::Operand lowerLvalAsExpr(const AST::Lval &lval,
                             std::vector<LIR::Instruction> &translationVector,
                             int &counter) {
  if (!lval.array_index && lval.field_name.empty()) {
    // Simple identifier, directly get the value
    return LIR::Operand{LIR::Operand::Kind::Var, lval.name};
  } else if (lval.array_index) {
    // Array access
    std::string arrayBaseVar = freshVar(counter);
    std::string indexVar = freshVar(counter);
    std::string elemVar = freshVar(counter);

    // Lower the base pointer and index expressions
    LIR::Operand base =
        lowerExpression(*lval.array_index, translationVector, counter);
    LIR::Operand index =
        lowerExpression(*lval.array_index, translationVector, counter);

    // Compute the address using Gep and then load the value
    translationVector.push_back(
        LIR::Instruction{LIR::Instruction::Kind::Gep,
                         .gep_lhs = {LIR::Operand::Kind::Var, arrayBaseVar},
                         .gep_ptr = base, .gep_idx = index});
    translationVector.push_back(
        LIR::Instruction{LIR::Instruction::Kind::Load,
                         .load_lhs = {LIR::Operand::Kind::Var, elemVar},
                         .load_addr = {LIR::Operand::Kind::Var, arrayBaseVar}});

    return {LIR::Operand::Kind::Var, elemVar};
  } else if (!lval.field_name.empty()) {
    // Field access within a struct
    std::string structVar = freshVar(counter);
    std::string fieldPtrVar = freshVar(counter);
    std::string fieldValueVar = freshVar(counter);

    // Compute the field pointer and then load the field value
    translationVector.push_back(
        LIR::Instruction{LIR::Instruction::Kind::Gfp,
                         .gfp_lhs = {LIR::Operand::Kind::Var, fieldPtrVar},
                         .gfp_ptr = {LIR::Operand::Kind::Var, lval.name},
                         .gfp_field = lval.field_name});
    translationVector.push_back(
        LIR::Instruction{LIR::Instruction::Kind::Load,
                         .load_lhs = {LIR::Operand::Kind::Var, fieldValueVar},
                         .load_addr = {LIR::Operand::Kind::Var, fieldPtrVar}});

    return {LIR::Operand::Kind::Var, fieldValueVar};
  }

  // Default return if none of the above cases are met
  return LIR::Operand();
}

// Function to construct the control-flow graph (CFG)
void constructCFG(const std::vector<LIR::Instruction> &translationVector,
                  LIR::FunctionBody &functionBody) {
  // Identify the leader instructions
  std::unordered_set<std::string> leaderLabels;
  for (size_t i = 0; i < translationVector.size(); ++i) {
    if (translationVector[i].kind == LIR::Instruction::Kind::Label) {
      leaderLabels.insert(translationVector[i].label);
    }
  }

  // Create basic blocks
  std::string currentLabel;
  LIR::BasicBlock currentBlock;
  for (size_t i = 0; i < translationVector.size(); ++i) {
    const auto &instr = translationVector[i];
    if (leaderLabels.count(instr.label) > 0) {
      if (!currentLabel.empty()) {
        functionBody.basic_blocks[currentLabel] = std::move(currentBlock);
      }
      currentLabel = instr.label;
      currentBlock = LIR::BasicBlock{currentLabel};
      continue;
    }
    currentBlock.instructions.push_back(instr);
    if (instr.kind == LIR::Instruction::Kind::Jump ||
        instr.kind == LIR::Instruction::Kind::Branch ||
        instr.kind == LIR::Instruction::Kind::Ret) {
      functionBody.basic_blocks[currentLabel] = std::move(currentBlock);
      currentLabel.clear();
    }
  }
  if (!currentLabel.empty()) {
    functionBody.basic_blocks[currentLabel] = std::move(currentBlock);
  }

  // Remove unreachable basic blocks
  std::unordered_set<std::string> reachableLabels;
  std::vector<std::string> worklist;
  worklist.push_back("entry");
  while (!worklist.empty()) {
    std::string label = worklist.back();
    worklist.pop_back();
    if (reachableLabels.count(label) > 0) {
      continue;
    }
    reachableLabels.insert(label);
    const auto &block = functionBody.basic_blocks[label];
    const auto &lastInstr = block.instructions.back();
    if (lastInstr.kind == LIR::Instruction::Kind::Jump) {
      worklist.push_back(lastInstr.jump_target);
    } else if (lastInstr.kind == LIR::Instruction::Kind::Branch) {
      worklist.push_back(lastInstr.branch_tt);
      worklist.push_back(lastInstr.branch_ff);
    }
  }
  for (auto it = functionBody.basic_blocks.begin();
       it != functionBody.basic_blocks.end();) {
    if (reachableLabels.count(it->first) == 0) {
      it = functionBody.basic_blocks.erase(it);
    } else {
      ++it;
    }
  }
}

void parseExpression(AST::Expr &expr, const json &expr_json);
void parseStatement(std::unique_ptr<AST::Stmt> &statement,
                    const json &stmt_json);
// Function to parse the AST JSON and construct the AST data structure
// Helper function to parse types, including nested pointers
AST::Type parseType(const json &type_json) {
  AST::Type type;

  if (type_json.is_string()) {
    type.name = type_json.get<std::string>();
  } else if (type_json.is_object()) {
    if (type_json.contains("Ptr")) {
      type.name = "Ptr";
      type.params.push_back(parseType(type_json["Ptr"]));
    } else if (type_json.contains("Struct")) {
      type.name = "Struct";
      type.params.push_back(parseType(type_json["Struct"]));
    } else {
      throw std::runtime_error("Unknown type structure in JSON.");
    }
  }

  return type;
}


AST::Program parseAST(const json &ast_json) {
  AST::Program program;

  // Parse globals
  if (ast_json.contains("globals") && ast_json["globals"].is_array()) {
    for (const auto &global : ast_json["globals"]) {
      if (global.contains("name") && global.contains("typ")) {
        AST::Type type = parseType(global["typ"]);
        program.globals[global["name"]] = type;
      }
    }
  }

  // Parse externs
  if (ast_json.contains("externs") && ast_json["externs"].is_object()) {
    for (const auto &extern_ : ast_json["externs"].items()) {
      std::vector<AST::Type> param_types;
      if (extern_.value().contains("params") &&
          extern_.value()["params"].is_array()) {
        for (const auto &param : extern_.value()["params"]) {
          param_types.push_back(parseType(param));
        }
      }
      AST::Type ret_type;
      if (extern_.value().contains("ret") &&
          extern_.value()["ret"].contains("name")) {
        ret_type = parseType(extern_.value()["ret"]);
      }
      program.externs[extern_.key()] = std::make_pair(param_types, ret_type);
    }
  }

  // Parse structs
  if (ast_json.contains("structs") && ast_json["structs"].is_array()) {
    for (const auto &struct_ : ast_json["structs"]) {
      std::vector<std::pair<std::string, AST::Type>> fields;
      if (struct_.contains("fields") && struct_["fields"].is_array()) {
        for (const auto &field : struct_["fields"]) {
          if (field.contains("name") && field.contains("typ")) {
            AST::Type field_type = parseType(field["typ"]);
            fields.push_back(std::make_pair(field["name"], field_type));
          }
        }
      }
      program.structs[struct_["name"]] = fields;
    }
  }

  // Parse functions
  if (ast_json.contains("functions") && ast_json["functions"].is_array()) {
    for (const auto &func : ast_json["functions"]) {
      AST::Function function;
      if (func.contains("name")) {
        function.name = func["name"];
      }

      // Parse parameters
      if (func.contains("params") && func["params"].is_array()) {
        for (const auto &param : func["params"]) {
          AST::Type param_type = parseType(param);
          function.params.push_back(std::make_pair("", param_type));
        }
      }

      // Parse return type
      if (func.contains("rettyp")) {
        function.ret_type = parseType(func["rettyp"]);
      } else {
        function.ret_type.name = "void";
      }

      // Parse locals
      if (func.contains("locals") && func["locals"].is_array()) {
        for (const auto &local : func["locals"]) {
          if (local.is_array() && local.size() >= 1 && local[0].is_object()) {
            AST::Type local_type = parseType(local[0]["typ"]);
            if (local[0].contains("name")) {
              function.locals.push_back(
                  std::make_pair(local[0]["name"], local_type));

              // Handle variable initialization
              if (local.size() >= 2 && !local[1].is_null()) {
                // Create an assignment statement for the initialization
                std::unique_ptr<AST::Stmt> assignStmt =
                    std::make_unique<AST::Stmt>();
                assignStmt->kind = AST::Stmt::Kind::Assign;

                // Set the lhs to the local variable
                assignStmt->assign_lhs = std::make_unique<AST::Lval>();
                assignStmt->assign_lhs->name = local[0]["name"];

                // Create an expression for the initialization
                assignStmt->assign_rhs = std::make_unique<AST::Expr>();
                parseExpression(*assignStmt->assign_rhs, local[1]);
                // Add the assignment statement to the end of func.stmts
                function.stmts.push_back(std::move(assignStmt));
              }
            }
          }
        }
      }

      // Parse statements
      if (func.contains("stmts") && func["stmts"].is_array()) {
        for (const auto &stmt : func["stmts"]) {
          std::unique_ptr<AST::Stmt> statement;
          parseStatement(statement, stmt);
          if (statement) {
            function.stmts.push_back(std::move(statement));
          }
        }
      }

      program.functions.emplace_back(std::move(function));
    }
  }

  return program;
}

void parseExpression(AST::Expr &expr, const json &expr_json) {
    if (expr_json.is_object() && expr_json.contains("Num")) {
        expr.kind = AST::Expr::Kind::Num;
        expr.num = expr_json["Num"].get<int>();
    } else if (expr_json.is_string()) {
        if (expr_json == "Nil") {
            expr.kind = AST::Expr::Kind::Nil;
        } else {
            expr.kind = AST::Expr::Kind::Id;
            expr.id = expr_json.get<std::string>();
        }
    } else if (expr_json.is_object()) {
        if (expr_json.contains("Id")) {
            expr.kind = AST::Expr::Kind::Id;
            expr.id = expr_json["Id"];
        } else if (expr_json.contains("UnOp")) {
            expr.kind = AST::Expr::Kind::UnOp;
            if (expr_json["UnOp"].contains("op")) {
                expr.unop = expr_json["UnOp"]["op"];
            }
            if (expr_json["UnOp"].contains("operand")) {
                expr.left = std::make_unique<AST::Expr>();
                parseExpression(*expr.left, expr_json["UnOp"]["operand"]);
            }
        } else if (expr_json.contains("BinOp")) {
            expr.kind = AST::Expr::Kind::BinOp;
            if (expr_json["BinOp"].contains("op")) {
                expr.binop = expr_json["BinOp"]["op"];
            }
            if (expr_json["BinOp"].contains("left")) {
                expr.left = std::make_unique<AST::Expr>();
                parseExpression(*expr.left, expr_json["BinOp"]["left"]);
            }
            if (expr_json["BinOp"].contains("right")) {
                expr.right = std::make_unique<AST::Expr>();
                parseExpression(*expr.right, expr_json["BinOp"]["right"]);
            }
        } else if (expr_json.contains("ArrayAccess")) {
            expr.kind = AST::Expr::Kind::ArrayAccess;
            if (expr_json["ArrayAccess"].contains("ptr")) {
                expr.array_ptr = std::make_unique<AST::Expr>();
                parseExpression(*expr.array_ptr, expr_json["ArrayAccess"]["ptr"]);
            }
            if (expr_json["ArrayAccess"].contains("index")) {
                expr.array_index = std::make_unique<AST::Expr>();
                parseExpression(*expr.array_index, expr_json["ArrayAccess"]["index"]);
            }
        } else if (expr_json.contains("New")) {
            expr.kind = AST::Expr::Kind::New;
            if (expr_json["New"].contains("typ")) {
                expr.new_type = parseType(expr_json["New"]["typ"]);
            }
            if (expr_json["New"].contains("amount")) {
                expr.new_size = std::make_unique<AST::Expr>();
                parseExpression(*expr.new_size, expr_json["New"]["amount"]);
            }
        } else if (expr_json.contains("Deref")) {
            expr.kind = AST::Expr::Kind::UnOp;
            expr.unop = "Deref";
            if (expr_json["Deref"].contains("ptr")) {
                expr.left = std::make_unique<AST::Expr>();
                parseExpression(*expr.left, expr_json["Deref"]["ptr"]);
            }
        } else if (expr_json.contains("FieldAccess")) {
            expr.kind = AST::Expr::Kind::FieldAccess;
            if (expr_json["FieldAccess"].contains("ptr")) {
                expr.field_ptr = std::make_unique<AST::Expr>();
                parseExpression(*expr.field_ptr, expr_json["FieldAccess"]["ptr"]);
            }
            if (expr_json["FieldAccess"].contains("field")) {
                expr.field_name = expr_json["FieldAccess"]["field"];
            }
        }
    }
}

void parseStatement(std::unique_ptr<AST::Stmt> &statement, const json &stmt_json) {
    if (stmt_json.is_object()) {
        if (stmt_json.contains("Assign")) {
            statement = std::make_unique<AST::Stmt>();
            statement->kind = AST::Stmt::Kind::Assign;

            // Parse the LHS
            if (stmt_json["Assign"].contains("lhs")) {
                statement->assign_lhs = std::make_unique<AST::Lval>();

                const auto &lhs_json = stmt_json["Assign"]["lhs"];
                if (lhs_json.contains("Id")) {
                    statement->assign_lhs->name = lhs_json["Id"];
                } else if (lhs_json.contains("ArrayAccess")) {
                    statement->assign_lhs->array_index = std::make_unique<AST::Expr>();
                    statement->assign_lhs->array_ptr = std::make_unique<AST::Expr>();
                    parseExpression(*statement->assign_lhs->array_ptr, lhs_json["ArrayAccess"]["ptr"]);
                    parseExpression(*statement->assign_lhs->array_index, lhs_json["ArrayAccess"]["index"]);
                } else if (lhs_json.contains("Deref")) {
                    statement->assign_lhs->array_ptr = std::make_unique<AST::Expr>();
                    parseExpression(*statement->assign_lhs->array_ptr, lhs_json["Deref"]);
                    statement->assign_lhs->name = "Deref"; // Marking as dereference for clarity
                } else {
                    throw std::runtime_error("Unrecognized LHS in Assign statement.");
                }
            }

            // Parse the RHS
            if (stmt_json["Assign"].contains("rhs")) {
                statement->assign_rhs = std::make_unique<AST::Expr>();
                if (stmt_json["Assign"]["rhs"].contains("RhsExp")) {
                    parseExpression(*statement->assign_rhs, stmt_json["Assign"]["rhs"]["RhsExp"]);
                } else {
                    parseExpression(*statement->assign_rhs, stmt_json["Assign"]["rhs"]);
                }
            }
        } else if (stmt_json.contains("If")) {
            statement = std::make_unique<AST::Stmt>();
            statement->kind = AST::Stmt::Kind::If;
            if (stmt_json["If"].contains("guard")) {
                statement->if_guard = std::make_unique<AST::Expr>();
                parseExpression(*statement->if_guard, stmt_json["If"]["guard"]);
            }
            if (stmt_json["If"].contains("tt") && stmt_json["If"]["tt"].is_array()) {
                for (const auto &then_stmt : stmt_json["If"]["tt"]) {
                    std::unique_ptr<AST::Stmt> then_statement;
                    parseStatement(then_statement, then_stmt);
                    if (then_statement) {
                        statement->if_then.push_back(std::move(then_statement));
                    }
                }
            }
            if (stmt_json["If"].contains("ff") && stmt_json["If"]["ff"].is_array()) {
                for (const auto &else_stmt : stmt_json["If"]["ff"]) {
                    std::unique_ptr<AST::Stmt> else_statement;
                    parseStatement(else_statement, else_stmt);
                    if (else_statement) {
                        statement->if_else.push_back(std::move(else_statement));
                    }
                }
            }
        } else if (stmt_json.contains("While")) {
            statement = std::make_unique<AST::Stmt>();
            statement->kind = AST::Stmt::Kind::While;
            if (stmt_json["While"].contains("guard")) {
                statement->while_guard = std::make_unique<AST::Expr>();
                parseExpression(*statement->while_guard, stmt_json["While"]["guard"]);
            }
            if (stmt_json["While"].contains("body") && stmt_json["While"]["body"].is_array()) {
                for (const auto &body_stmt : stmt_json["While"]["body"]) {
                    std::unique_ptr<AST::Stmt> body_statement;
                    parseStatement(body_statement, body_stmt);
                    if (body_statement) {
                        statement->while_body.push_back(std::move(body_statement));
                    }
                }
            }
        } else if (stmt_json.contains("Return")) {
            statement = std::make_unique<AST::Stmt>();
            statement->kind = AST::Stmt::Kind::Return;
            if (!stmt_json["Return"].is_null()) {
                statement->return_expr = std::make_unique<AST::Expr>();
                parseExpression(*statement->return_expr, stmt_json["Return"]);
            }
        } else {
            throw std::runtime_error("Unrecognized statement type in JSON.");
        }
    } else {
        throw std::runtime_error("Invalid statement format in JSON.");
    }
}

// Helper function to format types, handling nested types like Ptr(Int)
std::string formatType(const AST::Type &type) {
  if (type.name == "Ptr" && !type.params.empty()) {
    return "Ptr(" + formatType(type.params[0]) + ")";
  } else if (type.name == "Struct" && !type.params.empty()) {
    return "Struct(" + type.params[0].name + ")";
  }
  std::string capitalizedType = type.name;
  capitalizedType[0] = std::toupper(capitalizedType[0]);
  return capitalizedType;
}



// Function to output the LIR data structure
void outputLIR(const LIR::Program &lir) {
  // Output structs
  // Collect and sort struct names
  std::vector<std::string> structNames;
  for (const auto &struct_ : lir.structs) {
    structNames.push_back(struct_.first);
  }
  std::sort(structNames.begin(), structNames.end());

  // Print structs in sorted order
  for (const auto &structName : structNames) {
    const auto &fields = lir.structs.at(structName);
    std::cout << "Struct " << structName << "\n";
    for (const auto &field : fields) {
      std::cout << "  " << field.first << " : " << formatType(field.second) << "\n";
    }
    std::cout << "\n";
  }

  // Output externs
  std::cout << "Externs\n";
  for (const auto &extern_ : lir.externs) {
    std::cout << "  " << extern_.first << " : ";
    for (const auto &param : extern_.second.first) {
      std::cout << param << " ";
    }
    std::cout << "-> " << extern_.second.second << "\n";
  }
  std::cout << "\n";

  // Output globals
  std::cout << "Globals\n";
  for (const auto &global : lir.globals) {
    std::cout << "  " << global.first << " : " << global.second << "\n";
  }
  std::cout << "\n";

  // Output functions
  for (const auto &funcEntry : lir.functions) {
    const auto &funcName = funcEntry.first;
    const auto &func = funcEntry.second;

    std::cout << "Function " << funcName << "() -> " << func.ret_type << " {\n";

    // Output locals
    std::cout << "  Locals\n";
    std::vector<std::pair<std::string, AST::Type>> allLocals;

    // Add function locals to the vector
    for (const auto &local : func.locals) {
      allLocals.emplace_back(local);
    }

    // Add global temporary variables to the vector
    for (const auto &tempVar : tempVars) {
      AST::Type tempVarType;
      tempVarType.name = tempVar.second;
      allLocals.emplace_back(tempVar.first, tempVarType);
    }
    std::sort(allLocals.begin(), allLocals.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });

    for (const auto &[name, type] : allLocals) {
      std::cout << "    " << name << " : " << formatType(type) << "\n";
    }

    // Output basic blocks
    std::vector<std::string> sortedLabels;
    for (const auto &block : func.body.basic_blocks) {
      sortedLabels.push_back(block.first);
    }
    std::sort(sortedLabels.begin(), sortedLabels.end());

    for (const auto &label : sortedLabels) {
      const auto &block = func.body.basic_blocks.at(label);
      std::cout << "  " << block.label << ":\n";
      for (const auto &instr : block.instructions) {
        std::cout << "    ";
        switch (instr.kind) {
        case LIR::Instruction::Kind::Copy:
          std::cout << "Copy(" << instr.copy_lhs.var << ", ";
          if (instr.copy_rhs.kind == LIR::Operand::Kind::Var) {
            std::cout << instr.copy_rhs.var;
          } else {
            std::cout << instr.copy_rhs.constant;
          }
          std::cout << ")\n";
          break;

        case LIR::Instruction::Kind::Cmp: {
          std::string cmpOp = instr.cmp_op;
          if (cmpOp == "Equal")
            cmpOp = "eq";
          else if (cmpOp == "NotEq")
            cmpOp = "neq";
          else {
            std::transform(cmpOp.begin(), cmpOp.end(), cmpOp.begin(),
                           ::tolower);
          }

          std::cout << "Cmp(";
          if (instr.cmp_lhs.kind == LIR::Operand::Kind::Var) {
            std::cout << instr.cmp_lhs.var;
          } else {
            std::cout << instr.cmp_lhs.constant;
          }
          std::cout << ", " << cmpOp << ", ";
          if (instr.cmp_op1.kind == LIR::Operand::Kind::Var) {
            std::cout << instr.cmp_op1.var;
          } else {
            std::cout << instr.cmp_op1.constant;
          }
          std::cout << ", ";
          if (instr.cmp_op2.kind == LIR::Operand::Kind::Var) {
            std::cout << instr.cmp_op2.var;
          } else {
            std::cout << instr.cmp_op2.constant;
          }
          std::cout << ")\n";
          break;
        }

        case LIR::Instruction::Kind::Arith: {
          std::string arithOp = instr.arith_op;
          if (arithOp == "Neg")
            arithOp = "sub";
          else {
            std::transform(arithOp.begin(), arithOp.end(), arithOp.begin(),
                           ::tolower);
          }

          std::cout << "Arith(" << instr.arith_lhs.var << ", " << arithOp
                    << ", ";
          if (instr.arith_op1.kind == LIR::Operand::Kind::Var) {
            std::cout << instr.arith_op1.var;
          } else {
            std::cout << instr.arith_op1.constant;
          }
          std::cout << ", ";
          if (instr.arith_op2.kind == LIR::Operand::Kind::Var) {
            std::cout << instr.arith_op2.var;
          } else {
            std::cout << instr.arith_op2.constant;
          }
          std::cout << ")\n";
          break;
        }

        case LIR::Instruction::Kind::Branch:
          std::cout << "Branch(";
          if (instr.branch_guard.kind == LIR::Operand::Kind::Var) {
            std::cout << instr.branch_guard.var;
          } else {
            std::cout << instr.branch_guard.constant;
          }
          std::cout << ", " << instr.branch_tt << ", " << instr.branch_ff
                    << ")\n";
          break;

        case LIR::Instruction::Kind::Jump:
          std::cout << "Jump(" << instr.jump_target << ")\n";
          break;

        case LIR::Instruction::Kind::Ret:
          std::cout << "Ret(";
          if (instr.ret_val.kind == LIR::Operand::Kind::Var) {
            std::cout << instr.ret_val.var;
          } else {
            std::cout << instr.ret_val.constant;
          }
          std::cout << ")\n";
          break;

        case LIR::Instruction::Kind::Alloc:
          std::cout << "Alloc(" << instr.alloc_lhs.var << ", ";
          if (instr.alloc_size.kind == LIR::Operand::Kind::Var) {
            std::cout << instr.alloc_size.var;
          } else {
            std::cout << instr.alloc_size.constant;
          }
          std::cout << ")\n";
          break;

        case LIR::Instruction::Kind::Gep:
          std::cout << "Gep(" << instr.gep_lhs.var << ", ";
          if (instr.gep_ptr.kind == LIR::Operand::Kind::Var) {
            std::cout << instr.gep_ptr.var;
          } else {
            std::cout << instr.gep_ptr.constant;
          }
          std::cout << ", ";
          if (instr.gep_idx.kind == LIR::Operand::Kind::Var) {
            std::cout << instr.gep_idx.var;
          } else {
            std::cout << instr.gep_idx.constant;
          }
          std::cout << ")\n";
          break;

        case LIR::Instruction::Kind::Load:
          std::cout << "Load(" << instr.load_lhs.var << ", ";
          if (instr.load_addr.kind == LIR::Operand::Kind::Var) {
            std::cout << instr.load_addr.var;
          } else {
            std::cout << instr.load_addr.constant;
          }
          std::cout << ")\n";
          break;
          
        case LIR::Instruction::Kind::Store:
          std::cout << "Store(";
          if (instr.store_addr.kind == LIR::Operand::Kind::Var) {
            std::cout << instr.store_addr.var;
          } else {
            std::cout << instr.store_addr.constant;
          }
          std::cout << ", ";
          if (instr.store_val.kind == LIR::Operand::Kind::Var) {
            std::cout << instr.store_val.var;
          } else {
            std::cout << instr.store_val.constant;
          }
          std::cout << ")\n";
          break;

        case LIR::Instruction::Kind::Gfp:
          std::cout << "Gfp(" << instr.gfp_lhs.var << ", ";
          if (instr.gfp_ptr.kind == LIR::Operand::Kind::Var) {
            std::cout << instr.gfp_ptr.var;
          } else {
            std::cout << instr.gfp_ptr.constant;
          }
          std::cout << ", " << instr.gfp_field << ")\n";
          break;

        default:
          std::cout << "Unknown instruction kind\n";
          break;
        }
      }
      std::cout << "\n";
    }

    std::cout << "}\n";
  }
}

int main(int argc, char *argv[]) {
  if (argc != 3 && argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <ast_file>" << std::endl;
    return 1;
  }

  std::ifstream ast_file(argv[1]);
  json ast_json;
  ast_file >> ast_json;

  // Parse the AST JSON and construct the AST data structure
  AST::Program ast = parseAST(ast_json);

  // Create an empty LIR::Program(changed it to global its way above)
   lir;

  // Lower the AST program to LIR
  lowerProgram(ast, lir);

  // Output the LIR data structure
  outputLIR(lir);

  return 0;
}