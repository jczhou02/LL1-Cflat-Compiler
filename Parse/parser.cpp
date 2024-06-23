#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <optional>
#include <variant>
#include <unordered_map>
#include <algorithm>
#include <stdexcept>
#include <exception>


using namespace std;
// Type definitions for clarity
using StructId = std::string;
using FuncId = std::string;


struct TypeError {
    std::string message;
    // Additional fields can be included, such as node position, etc.

    // Constructor for easy creation
    TypeError(std::string msg) : message(msg) {}
};
std::vector<TypeError> typeErrors;     // Global variable to store type errors

void addTypeError(const std::string& error) {   
    typeErrors.push_back(TypeError(error));
}

void reportTypeErrors() {
    std::sort(typeErrors.begin(), typeErrors.end(), [](const TypeError& a, const TypeError& b) {
        return a.message < b.message;  // Sort based on the error message
    });

    for (const auto& error : typeErrors) {
        std::cout << error.message << std::endl;
    }
}


// Base type class
struct Type {
    virtual std::string toString() = 0;
    virtual ~Type() = default;
    virtual bool operator==(const Type& other) const = 0;
    virtual std::unique_ptr<Type> clone() const = 0;
};

struct Any : Type{
    bool operator==(const Type& other) const override {
        return true;
    }
    std::string toString() override { return "_"; }

    std::unique_ptr<Type> clone() const override {
        return std::make_unique<Any>();
    }
};

// Integer type
struct Int : Type {
    std::string toString() override { return "int"; }
    bool operator==(const Type& other) const override {
        if (dynamic_cast<const Int*>(&other)) {
            return true;
        }
        if (const Any* a = dynamic_cast<const Any*>(&other)) {
            return true;
        }
        return false;
    }
    std::unique_ptr<Type> clone() const override {
        return std::make_unique<Int>();
    }
};

// Struct type
struct Struct : Type {
    StructId name;
    std::vector<std::pair<std::string, std::unique_ptr<Type>>> fields;


        Struct(StructId name, std::vector<std::pair<std::string, std::unique_ptr<Type>>> fields)
        : name(std::move(name)), fields(std::move(fields)) {}

    std::string toString() override {return name;}
    bool operator==(const Type& other) const override {
        if (const Struct* s = dynamic_cast<const Struct*>(&other)) {
            return name == s->name;
        }
        if (const Any* a = dynamic_cast<const Any*>(&other)) {
            return true;
        }
        return false;
    }
    std::unique_ptr<Type> clone() const override {
        std::vector<std::pair<std::string, std::unique_ptr<Type>>> clonedFields;
        for (const auto& field : fields) {
            clonedFields.push_back({field.first, field.second->clone()});
        }
        return std::make_unique<Struct>(name, std::move(clonedFields));
    }
};
struct Array : Type {
    std::unique_ptr<Type> elemType;
    Array(std::unique_ptr<Type> elemType) : elemType(std::move(elemType)) {}
};
// Function type
struct Fn : Type {
    std::vector<std::unique_ptr<Type>> params;
    std::unique_ptr<Type> ret;

        Fn(std::vector<std::unique_ptr<Type>> params, std::unique_ptr<Type> ret)
        : params(std::move(params)), ret(std::move(ret)) {}
    std::string toString() override {
        std::string prms = "";
        if (!ret){}
        for (size_t i = 0; i < params.size(); ++i) {
            prms += (i == 0 ? "" : ", ") + params[i]->toString();
        }
        return "(" + prms + ")" + " -> " +(ret ? ret->toString() : "_");
        }

    bool operator==(const Type& other) const override {
        if (const Fn* f = dynamic_cast<const Fn*>(&other)) {
            if (params.size() != f->params.size()) {
                return false;
            }
            for (size_t i = 0; i < params.size(); ++i) {
                if (!params[i]->operator==(*f->params[i])) {
                    return false;
                }
            }
            if (ret && f->ret) {
                return ret->operator==(*f->ret);
            }
            return !ret && !f->ret;
        }
        if (const Any* a = dynamic_cast<const Any*>(&other)) {
            return true;
        }
        return false;
    }
    std::unique_ptr<Type> clone() const override {
        std::vector<std::unique_ptr<Type>> clonedParams;
        for (const auto& param : params) {
            if (param) {
            clonedParams.push_back(param->clone());
        } else {
            // Handle or log the null case
            clonedParams.push_back(nullptr);
        }
        }
        return std::make_unique<Fn>(std::move(clonedParams), ret ? ret->clone() : nullptr);
    }
};

// Pointer type
struct Ptr : Type {
    std::unique_ptr<Type> ref;
        Ptr(std::unique_ptr<Type> ref) : ref(std::move(ref)) {}
    std::string toString() override { return "&" + ref->toString(); }

    bool operator==(const Type& other) const override {
        if (const Ptr* p = dynamic_cast<const Ptr*>(&other)) {
            return ref->operator==(*p->ref);
        }
        if (const Any* a = dynamic_cast<const Any*>(&other)) {
            return true;
        }
        return false;
    }
    std::unique_ptr<Type> clone() const override {
        return std::make_unique<Ptr>(ref->clone());
    }
};


// Declarations for variables, parameters, etc.
struct Decl {
    std::string name;
    std::unique_ptr<Type> type;
        Decl() = default;
    Decl(std::string name, std::unique_ptr<Type> type)
        : name(std::move(name)), type(std::move(type)) {}
};

// Expression base class
struct Exp {
    virtual ~Exp() = default;
    virtual Type* judgement(const std::string funcName, std::unordered_map<std::string, unique_ptr<Type>>& gamma, 
    const std::unordered_map<std::string, std::unordered_map<std::string, unique_ptr<Type>>>& delta) = 0;
};

struct NumExp : Exp {
    int32_t n;

    NumExp(int32_t n) : n(n) {}
    Type* judgement(const std::string funcName, std::unordered_map<std::string, unique_ptr<Type>>& gamma,
    const std::unordered_map<std::string, std::unordered_map<std::string, unique_ptr<Type>>>& delta) override {
        return new Int();
    }
};

struct IdExp : Exp {
    std::string name;

    IdExp(std::string name) : name(std::move(name)) {}

    Type* judgement(const std::string funcName, std::unordered_map<std::string, unique_ptr<Type>>& gamma, 
    const std::unordered_map<std::string, std::unordered_map<std::string, unique_ptr<Type>>>& delta) override {
        if (gamma.find(name) != gamma.end()) {
            return gamma[name].get();
        }
        addTypeError("[ID] in function " + funcName + ": variable " + name + " undefined");
        return new Any();
    }
};

// Nil expression
struct NilExp : Exp {
    Type* judgement(const std::string funcName, std::unordered_map<std::string, unique_ptr<Type>>& gamma, 
    const std::unordered_map<std::string, std::unordered_map<std::string, unique_ptr<Type>>>& delta) override {
        return new Ptr(make_unique<Any>());
    }
};

// Unary operation expression
enum class UnaryOp { Neg, Deref ,Addr};
struct UnOpExp : Exp {
    UnaryOp op;
    std::unique_ptr<Exp> operand;

        UnOpExp(UnaryOp op, std::unique_ptr<Exp> operand)
        : op(op), operand(std::move(operand)) {}

    Type* judgement(const std::string funcName, std::unordered_map<std::string, unique_ptr<Type>>& gamma,
    const std::unordered_map<std::string, std::unordered_map<std::string, unique_ptr<Type>>>& delta) override {
        Type* operandType = operand->judgement(funcName, gamma, delta);
        if (op == UnaryOp::Neg) {
            if (auto intType = dynamic_cast<Int*>(operandType)) {
                return intType;
            }
            else if (auto anyType = dynamic_cast<Any*>(operandType)) {
                return new Int();
            }
            addTypeError("[NEG] in function " + funcName + ": negating type " + operandType->toString() + " instead of int");
            return new Int();
        } else if (op == UnaryOp::Deref) {
            if (auto ptrType = dynamic_cast<Ptr*>(operandType)) {
                return ptrType->ref.get();
            }
            else if (auto anyType = dynamic_cast<Any*>(operandType)) {
                return anyType;
            }
            addTypeError("[DEREF] in function " + funcName + ": dereferencing type " + operandType->toString() + " instead of pointer");
        } 
        return new Any();
    }
};

// Binary operation expression
enum class BinaryOp { Add, Sub, Mul, Div, Equal, NotEq, Lt, Lte, Gt, Gte };
struct BinOpExp : Exp {
    BinaryOp op;
    std::unique_ptr<Exp> left;
    std::unique_ptr<Exp> right;


        BinOpExp(BinaryOp op, std::unique_ptr<Exp> left, std::unique_ptr<Exp> right)
        : op(op), left(std::move(left)), right(std::move(right)) {}

    Type* judgement(const std::string funcName, std::unordered_map<std::string, unique_ptr<Type>>& gamma,
    const std::unordered_map<std::string, std::unordered_map<std::string, unique_ptr<Type>>>& delta) override {
        Type* leftType = left->judgement(funcName, gamma, delta);
        Type* rightType = right->judgement(funcName, gamma, delta);
        // split into BINOP-EQ and BINOP-REST
        if (op == BinaryOp::Equal || op == BinaryOp::NotEq) {   //  BINOP-EQ: check left and right are same type, and that both are either Int or Ptr
            if (!(leftType->operator==(*rightType))){
                addTypeError("[BINOP-EQ] in function " + funcName + ": operands with different types: " + leftType->toString() + " vs " + rightType->toString());
            }
            if (!(dynamic_cast<Int*>(leftType) || dynamic_cast<Ptr*>(leftType) || dynamic_cast<Any*>(leftType))) {
                addTypeError("[BINOP-EQ] in function " + funcName + ": operand has non-primitive type " + leftType->toString());
            } 
            if (!(dynamic_cast<Int*>(rightType) || dynamic_cast<Ptr*>(rightType) || dynamic_cast<Any*>(rightType))) {
                addTypeError("[BINOP-EQ] in function " + funcName + ": operand has non-primitive type " + rightType->toString());
            }
            
        } else {        //  BINOP-REST: check left and right are both Int
            if (!(dynamic_cast<Int*>(leftType) || (dynamic_cast<Any*>(leftType)))) {
                addTypeError("[BINOP-REST] in function " + funcName + ": operand has type " + leftType->toString() + " instead of int");
            }
            if (!(dynamic_cast<Int*>(rightType) || (dynamic_cast<Any*>(rightType)))) {
                addTypeError("[BINOP-REST] in function " + funcName + ": operand has type " + rightType->toString() + " instead of int");
            }
        }
        return new Int();
    }
};

// Array access expression
struct ArrayAccessExp : Exp {
    std::unique_ptr<Exp> ptr;
    std::unique_ptr<Exp> index;

        ArrayAccessExp(std::unique_ptr<Exp> ptr, std::unique_ptr<Exp> index)
        : ptr(std::move(ptr)), index(std::move(index)) {}

    Type* judgement(const std::string funcName, std::unordered_map<std::string, unique_ptr<Type>>& gamma,
    const std::unordered_map<std::string, std::unordered_map<std::string, unique_ptr<Type>>>& delta) override {
        //Type* ptrType = ptr->judgement(funcName, gamma, delta);
        Type* indexType = index->judgement(funcName, gamma, delta);
        if (!(dynamic_cast<Int*>(indexType)) && !(dynamic_cast<Any*>(indexType))) {
            addTypeError("[ARRAY] in function " + funcName + ": array index is type " + indexType->toString() + " instead of int");
        }
        // check if ptr is a pointer type to some type; if so return its ref type
        auto some_type = ptr->judgement(funcName, gamma, delta);
        if (auto found_ptr = dynamic_cast<Ptr*>(some_type)){
            return found_ptr->ref.get();
        }
        else if (auto found_any = dynamic_cast<Any*>(some_type)){
            return some_type;
        }
        else{
            addTypeError("[ARRAY] in function " + funcName + ": dereferencing non-pointer type " + some_type->toString());
            return new Any();
        }
    }
};

// Field access expression
struct FieldAccessExp : Exp {
    std::unique_ptr<Exp> ptr;
    std::string field;

        FieldAccessExp(std::unique_ptr<Exp> ptr, std::string field)
        : ptr(std::move(ptr)), field(std::move(field)) {}

    Type* judgement(const std::string funcName, std::unordered_map<std::string, unique_ptr<Type>>& gamma,
    const std::unordered_map<std::string, std::unordered_map<std::string, unique_ptr<Type>>>& delta) override {
        Type* ptrType = ptr->judgement(funcName, gamma, delta);
        if (!(dynamic_cast<Ptr*>(ptrType) || dynamic_cast<Any*>(ptrType))){
            addTypeError("[FIELD] in function " + funcName + ": accessing field of incorrect type " + ptrType->toString());
        }
        else if (auto ptr = dynamic_cast<Ptr*>(ptrType)) {
            if (!dynamic_cast<Struct*>(ptr->ref.get())) {
              addTypeError("[FIELD] in function " + funcName + ": accessing field of incorrect type " + ptrType->toString());
            }
            else if (auto str = dynamic_cast<Struct*>(ptr->ref.get())) {
                if (delta.find(str->name) != delta.end()) {    //  not equal to end so we found it
                    if (delta.at(str->name).find(field) != delta.at(str->name).end()) {
                        return delta.at(str->name).at(field).get();
                    }
                    addTypeError("[FIELD] in function " + funcName + ": accessing non-existent field " + field + " of struct type " + str->name);
                }
                else{addTypeError("[FIELD] in function " + funcName + ": accessing field of non-existent struct type " + str->name);}
            }
        }
        return new Any();
    }
};

// Function call expression

struct CallExp : Exp {
    std::unique_ptr<Exp> callee;
    std::vector<std::unique_ptr<Exp>> args;
        CallExp(std::unique_ptr<Exp> callee, std::vector<std::unique_ptr<Exp>> args)
        : callee(std::move(callee)), args(std::move(args)) {}

    Type* judgement(const std::string funcName, std::unordered_map<std::string, unique_ptr<Type>>& gamma,
    const std::unordered_map<std::string, std::unordered_map<std::string, unique_ptr<Type>>>& delta) override {
        Type* calleeType = callee->judgement(funcName, gamma, delta);    
        bool caughtArgError = false;
        bool isMain = false;
        // check if callee is main ; we can't call judgement on callee again or else it will print the error twice
        if (auto id = dynamic_cast<IdExp*>(callee.get())) {          //   CHANGED THIS LINE from calleeType to callee.get()
            if (id->name == "main") {
                addTypeError("[ECALL-INTERNAL] in function " + funcName + ": calling main");
                isMain = true;
            }
        }
        if (auto ptr = dynamic_cast<Ptr*>(calleeType)) {
            if (auto fn = dynamic_cast<Fn*>(ptr->ref.get())) {
                if (!(fn->ret)) {
                    addTypeError("[ECALL-INTERNAL] in function " + funcName + ": calling a function with no return value");
                    //retAny = true;
                }
                if (fn->params.size() != args.size()) {
                    addTypeError("[ECALL-INTERNAL] in function " + funcName + ": call number of arguments (" + std::to_string(args.size()) + ") and parameters (" + std::to_string(fn->params.size()) + ") don't match");
                    //hasError = true;
                }
                for (size_t i = 0; i < args.size(); ++i) {
                    if (i < fn->params.size()){
                        Type* argType = args[i]->judgement(funcName, gamma, delta);
                        if (!fn->params[i]->operator==(*argType)) {
                        addTypeError("[ECALL-INTERNAL] in function " + funcName + ": call argument has type " + argType->toString() + " but parameter has type " + fn->params[i]->toString());
                        //hasError = true;
                    }
                    }
                    else{
                        continue;
                    }
                }
                return fn->ret.get() ? fn->ret.get() : new Any();
            }    //  not internal; check if its external
            else{
                addTypeError("[ECALL-*] in function " + funcName + ": calling non-function type " + calleeType->toString());
                return new Any();
            
            }
        }
        else if (auto fn = dynamic_cast<Fn*>(calleeType)) {
            if (!(fn->ret)) {
                addTypeError("[ECALL-EXTERN] in function " + funcName + ": calling a function with no return value");
                //retAny = true;
            }
            if (fn->params.size() != args.size()) {
                addTypeError("[ECALL-EXTERN] in function " + funcName + ": call number of arguments (" + std::to_string(args.size()) + ") and parameters (" + std::to_string(fn->params.size()) + ") don't match");
                //hasError = true;
            }
            for (size_t i = 0; i < args.size(); ++i) {
                if (i < fn->params.size()){
                    Type* argType = args[i]->judgement(funcName, gamma, delta);
                    if (!(fn->params[i]->operator==(*argType))) {
                        addTypeError("[ECALL-EXTERN] in function " + funcName + ": call argument has type " + argType->toString() + " but parameter has type " + fn->params[i]->toString());
                        caughtArgError = true;
                    }
                }
                else{
                    continue;
                }
            }
            return fn->ret ? fn->ret.get() : new Any();
        } else {
            if (!dynamic_cast<Any*>(calleeType)){
            //addTypeError("[ECALL-*] in function " + funcName + ": calling non-function type " + calleeType->toString()); 
                if (!isMain){
                    if (calleeType) {  // Add a null check
                        addTypeError("[ECALL-*] in function " + funcName + ": calling non-function type " + calleeType->toString());
                    } else {
                        addTypeError("[ECALL-*] in function " + funcName + ": calling non-function type on null object");
                    }    
                }    
            }
            return new Any();
        }
    }
};

// L-values
struct Lval {
    virtual ~Lval() = default;
    virtual Type* judgement(const std::string funcName, std::unordered_map<std::string, unique_ptr<Type>>& gamma,
    const std::unordered_map<std::string, std::unordered_map<std::string, unique_ptr<Type>>>& delta) = 0;
};

struct IdLval : Lval {
    std::string name;

    IdLval(std::string name) : name(std::move(name)) {}

    Type* judgement(const std::string funcName, std::unordered_map<std::string, unique_ptr<Type>>& gamma,
    const std::unordered_map<std::string, std::unordered_map<std::string, unique_ptr<Type>>>& delta) override {
        if (gamma.find(name) != gamma.end()) {
            return gamma[name].get();
        }
        addTypeError("[ID] in function " + funcName + ": variable " + name + " undefined");
        return new Any();
    }
};

struct DerefLval : Lval {
    std::unique_ptr<Lval> lval;

    DerefLval(std::unique_ptr<Lval> lval) : lval(std::move(lval)) {}

    Type* judgement(const std::string funcName, std::unordered_map<std::string, unique_ptr<Type>>& gamma,
    const std::unordered_map<std::string, std::unordered_map<std::string, unique_ptr<Type>>>& delta) override {
        Type* lvalType = lval->judgement(funcName, gamma, delta);
        if (auto ptr = dynamic_cast<Ptr*>(lvalType)) {
            return ptr->ref.get();
        }
        else if (auto anyType = dynamic_cast<Any*>(lvalType)) {
            return anyType;
        }
        addTypeError("[DEREF] in function " + funcName + ": dereferencing type " + lvalType->toString() + " instead of pointer");
        return new Any();
    }
};

struct ArrayAccessLval : Lval {
    std::unique_ptr<Lval> ptr;
    std::unique_ptr<Exp> index;

    ArrayAccessLval(std::unique_ptr<Lval> ptr, std::unique_ptr<Exp> index)
        : ptr(std::move(ptr)), index(std::move(index)) {}

    Type* judgement(const std::string funcName, std::unordered_map<std::string, unique_ptr<Type>>& gamma,
    const std::unordered_map<std::string, std::unordered_map<std::string, unique_ptr<Type>>>& delta) override {
        Type* ptrType = ptr->judgement(funcName, gamma, delta);
        Type* indexType = index->judgement(funcName, gamma, delta);
        if (!(dynamic_cast<Int*>(indexType)) && !(dynamic_cast<Any*>(indexType))) {
            addTypeError("[ARRAY] in function " + funcName + ": array index is type " + indexType->toString() + " instead of int");
        }
        if (auto ptr = dynamic_cast<Ptr*>(ptrType)) {
            return ptr->ref.get();
        }
        else if (auto found_any = dynamic_cast<Any*>(ptrType)){
            return found_any;
        }
        addTypeError("[ARRAY] in function " + funcName + ": dereferencing non-pointer type " + ptrType->toString());
        return new Any();
    }
};

struct FieldAccessLval : Lval {
    std::unique_ptr<Lval> ptr;
    std::string field;

    FieldAccessLval(std::unique_ptr<Lval> ptr, std::string field)
        : ptr(std::move(ptr)), field(std::move(field)) {}

    Type* judgement(const std::string funcName, std::unordered_map<std::string, unique_ptr<Type>>& gamma,
    const std::unordered_map<std::string, std::unordered_map<std::string, unique_ptr<Type>>>& delta) override {
        Type* ptrType = ptr->judgement(funcName, gamma, delta);
        if (!(dynamic_cast<Ptr*>(ptrType) || dynamic_cast<Any*>(ptrType))){
            addTypeError("[FIELD] in function " + funcName + ": accessing field of incorrect type " + ptrType->toString());
        }
        else if (auto ptr = dynamic_cast<Ptr*>(ptrType)) {
            if (!dynamic_cast<Struct*>(ptr->ref.get())) {
              addTypeError("[FIELD] in function " + funcName + ": accessing field of incorrect type " + ptrType->toString());
            }
            else if (auto str = dynamic_cast<Struct*>(ptr->ref.get())) {
                if (delta.find(str->name) != delta.end()) {    //  not equal to end so we found it
                    if (delta.at(str->name).find(field) != delta.at(str->name).end()) {
                        return delta.at(str->name).at(field).get();
                    }
                    addTypeError("[FIELD] in function " + funcName + ": accessing non-existent field " + field + " of struct type " + str->name);
                }
                else{addTypeError("[FIELD] in function " + funcName + ": accessing field of non-existent struct type " + str->name);}
            }
        }
        return new Any();
    }
};

// Right-hand side of assignments
struct Rhs {
    virtual ~Rhs() = default;
    virtual Type* judgement(const std::string funcName, std::unordered_map<std::string, unique_ptr<Type>>& gamma,
    const std::unordered_map<std::string, std::unordered_map<std::string, unique_ptr<Type>>>& delta) = 0;
};

struct RhsExp : Rhs {
    std::unique_ptr<Exp> exp;

    RhsExp(std::unique_ptr<Exp> exp) : exp(std::move(exp)) {}

    Type* judgement(const std::string funcName, std::unordered_map<std::string, unique_ptr<Type>>& gamma,
    const std::unordered_map<std::string, std::unordered_map<std::string, unique_ptr<Type>>>& delta) override {
        return exp->judgement(funcName, gamma, delta);
    }
};

struct NewRhs : Rhs {
    std::unique_ptr<Type> type;
    std::unique_ptr<Exp> amount;

    NewRhs(std::unique_ptr<Type> type, std::unique_ptr<Exp> amount)
        : type(std::move(type)), amount(std::move(amount)) {}

    Type* judgement(const std::string funcName, std::unordered_map<std::string, unique_ptr<Type>>& gamma,
    const std::unordered_map<std::string, std::unordered_map<std::string, unique_ptr<Type>>>& delta) override {
        return amount->judgement(funcName, gamma, delta);
    }
};

// Statements
struct Stmt {
    virtual ~Stmt() = default;
    // judgement for stmts need to also take in boolean for loop and optional return type
    virtual void judgement(const std::string funcName, bool loop, const unique_ptr<Type>& retType, std::unordered_map<std::string, unique_ptr<Type>>& gamma,
    const std::unordered_map<std::string, std::unordered_map<std::string, unique_ptr<Type>>>& delta) = 0;
};

struct BreakStmt : Stmt {
    void judgement(const std::string funcName, bool loop, const unique_ptr<Type>& retType, std::unordered_map<std::string, unique_ptr<Type>>& gamma,
    const std::unordered_map<std::string, std::unordered_map<std::string, unique_ptr<Type>>>& delta) override {
        if (!loop) {
            addTypeError("[BREAK] in function " + funcName + ": break outside of loop");
        }
    }
};
struct ContinueStmt : Stmt {
    void judgement(const std::string funcName, bool loop, const unique_ptr<Type>& retType, std::unordered_map<std::string, unique_ptr<Type>>& gamma,
    const std::unordered_map<std::string, std::unordered_map<std::string, unique_ptr<Type>>>& delta) override {
        if (!loop) {
            addTypeError("[CONTINUE] in function " + funcName + ": continue outside of loop");
        }
    }
};

struct IfStmt : Stmt {
    std::unique_ptr<Exp> guard;
    std::vector<std::unique_ptr<Stmt>> tt;
    std::vector<std::unique_ptr<Stmt>> ff;

    IfStmt(std::unique_ptr<Exp> guard, std::vector<std::unique_ptr<Stmt>> tt, std::vector<std::unique_ptr<Stmt>> ff)
        : guard(std::move(guard)), tt(std::move(tt)), ff(std::move(ff)) {}

    void judgement(const std::string funcName, bool loop, const unique_ptr<Type>& retType, std::unordered_map<std::string, unique_ptr<Type>>& gamma,
    const std::unordered_map<std::string, std::unordered_map<std::string, unique_ptr<Type>>>& delta) override {
        Type* guardType = guard->judgement(funcName, gamma, delta);
        if (!(dynamic_cast<Int*>(guardType) || dynamic_cast<Any*>(guardType))) {
            addTypeError("[IF] in function " + funcName + ": if guard has type " + guardType->toString() + " instead of int");
        }
        for (const auto& stmt : tt) {
            stmt->judgement(funcName, loop, retType, gamma, delta);
        }
        for (const auto& stmt : ff) {
            stmt->judgement(funcName, loop, retType, gamma, delta);
        }
    }
};

struct WhileStmt : Stmt {
    std::unique_ptr<Exp> guard;
    std::vector<std::unique_ptr<Stmt>> body;

    WhileStmt(std::unique_ptr<Exp> guard, std::vector<std::unique_ptr<Stmt>> body)
        : guard(std::move(guard)), body(std::move(body)) {}

    void judgement(const std::string funcName, bool loop, const unique_ptr<Type>& retType, std::unordered_map<std::string, unique_ptr<Type>>& gamma,
    const std::unordered_map<std::string, std::unordered_map<std::string, unique_ptr<Type>>>& delta) override {
        Type* guardType = guard->judgement(funcName, gamma, delta);
        if (!(dynamic_cast<Int*>(guardType) || dynamic_cast<Any*>(guardType))){
            addTypeError("[WHILE] in function " + funcName + ": while guard has type " + guardType->toString() + " instead of int");
        }
        for (const auto& stmt : body) {
            stmt->judgement(funcName, true, retType, gamma, delta);
        }        
    }
};

struct ReturnStmt : Stmt {
    std::unique_ptr<Exp> exp;

    ReturnStmt(std::unique_ptr<Exp> exp) : exp(std::move(exp)) {}

    void judgement(const std::string funcName, bool loop, const unique_ptr<Type>& retType, std::unordered_map<std::string, unique_ptr<Type>>& gamma,
    const std::unordered_map<std::string, std::unordered_map<std::string, unique_ptr<Type>>>& delta) override {
        if (retType) {
            if (exp) {
                Type* retExpType = exp->judgement(funcName, gamma, delta);
                if (!(retType.get()->operator==(*retExpType))) {
                    addTypeError("[RETURN-2] in function " + funcName + ": should return " + retType.get()->toString() + " but returning " + retExpType->toString());
                }
            } else {
                addTypeError("[RETURN-2] in function " + funcName + ": should return " + retType.get()->toString() + " but returning nothing");
            }
        } else {
            if (exp) {
                Type* retExpType = exp->judgement(funcName, gamma, delta);
                addTypeError("[RETURN-1] in function " + funcName + ": should return nothing but returning " + retExpType->toString());
            }
        }
    }
};

struct AssignStmt : Stmt {
    std::unique_ptr<Lval> lhs;
    std::unique_ptr<Rhs> rhs;

    AssignStmt(std::unique_ptr<Lval> lhs, std::unique_ptr<Rhs> rhs)
        : lhs(std::move(lhs)), rhs(std::move(rhs)) {}

// call judgment recursively on lhs and rhs
// figure out if member rhs is of type RhsExp or NewExp
// EXP case: lhs and rhs must have same type, cannot be struct or function type
// NEW case: lhs must be pointer type to the same type in Rhs::New, rhs must be int type, and the lhs ptr type can't be a function type
    void judgement(const std::string funcName, bool loop, const unique_ptr<Type>& retType, std::unordered_map<std::string, unique_ptr<Type>>& gamma,
    const std::unordered_map<std::string, std::unordered_map<std::string, unique_ptr<Type>>>& delta) override {
        Type* lhsType = lhs->judgement(funcName, gamma, delta);    
        Type* rhsType = rhs->judgement(funcName, gamma, delta);    //  if NEW case : returns amount->judgement()
        if (dynamic_cast<RhsExp*>(rhs.get())){     // ASSIGN-EXP
            if (!(lhsType->operator==(*rhsType))) {
                if (auto nilp = dynamic_cast<Ptr*>(lhsType)){     //  dismiss error if lhs has type &_
                    if (!dynamic_cast<Any*>(nilp->ref.get())){
                        addTypeError("[ASSIGN-EXP] in function " + funcName + ": assignment lhs has type " + lhsType->toString() +  " but rhs has type " + rhsType->toString());
                    }
                }
                else{addTypeError("[ASSIGN-EXP] in function " + funcName + ": assignment lhs has type " + lhsType->toString() +  " but rhs has type " + rhsType->toString());}
            }
            // check if if lhsType is a struct or function type or if rhsType is a struct or function type
            if (dynamic_cast<Struct*>(lhsType) || dynamic_cast<Fn*>(lhsType)) {
                addTypeError("[ASSIGN-EXP] in function " + funcName + ": assignment to struct or function");
            }
            // else if (dynamic_cast<Struct*>(rhsType) || dynamic_cast<Fn*>(rhsType)) {
            //     addTypeError("[ASSIGN-EXP] in function " + funcName + ": assignment to a struct or function");
            // }
        }
        else if (auto newRhs = dynamic_cast<NewRhs*>(rhs.get())){   // ASSIGN-NEW ;  lhsType should be a pointer type to the same type in Rhs::New, rhsType should be int type, and the lhs ptr type or NewRhs.exp can't be a function type
            if (auto ptr = dynamic_cast<Ptr*>(lhsType)) {
                if (!(ptr->ref->operator==(*newRhs->type.get()))) {
                    addTypeError("[ASSIGN-NEW] in function " + funcName + ": assignment lhs has type " + lhsType->toString() +  " but we're allocating type " + newRhs->type.get()->toString());
                }
                if (dynamic_cast<Fn*>(newRhs->type.get()) || dynamic_cast<Fn*>(ptr->ref.get())) {
                    addTypeError("[ASSIGN-NEW] in function " + funcName + ": allocating function type " + newRhs->type->toString());
                }
                if (!(dynamic_cast<Int*>(rhsType) || dynamic_cast<Any*>(rhsType))) {
                    addTypeError("[ASSIGN-NEW] in function " + funcName + ": allocation amount is type " + rhsType->toString() + " instead of int");
                }
            }
            else{      //   lhstype is not a pointer type to something
                if (!(dynamic_cast<Any*>(lhsType))){
                    addTypeError("[ASSIGN-NEW] in function " + funcName + ": assignment lhs has type " + lhsType->toString() +  " but we're allocating type " + newRhs->type.get()->toString());
                }
                if (dynamic_cast<Fn*>(newRhs->type.get())) {    //  || dynamic_cast<Fn*>(lhsType)
                    addTypeError("[ASSIGN-NEW] in function " + funcName + ": allocating function type " + newRhs->type->toString());
                }
                if (!(dynamic_cast<Int*>(rhsType) || dynamic_cast<Any*>(rhsType))) {
                    addTypeError("[ASSIGN-NEW] in function " + funcName + ": allocation amount is type " + rhsType->toString() + " instead of int");
                }
            }
        }
    }
};

struct CallStmt : Stmt {
    std::unique_ptr<Lval> callee;
    std::vector<std::unique_ptr<Exp>> args;

    CallStmt(std::unique_ptr<Lval> callee, std::vector<std::unique_ptr<Exp>> args)
        : callee(std::move(callee)), args(std::move(args)) {}
        
    void judgement(const std::string funcName, bool loop, const unique_ptr<Type>& retType, std::unordered_map<std::string, unique_ptr<Type>>& gamma,
    const std::unordered_map<std::string, std::unordered_map<std::string, unique_ptr<Type>>>& delta) override {
        Type* calleeType = callee->judgement(funcName, gamma, delta);
        // check if callee is main
        bool isMain = false;
        if (auto id = dynamic_cast<IdLval*>(callee.get())) {
            if (id->name == "main") {
                addTypeError("[SCALL-INTERNAL] in function " + funcName + ": calling main");
                isMain = true;
                //cout << "found main\n";
                // print out mapping in gamma for main
                // for (auto const& x : gamma){
                //     std::cout << x.first << " " << x.second->toString() << "\n";
                // } 
                if (args.size() != 0){
                    addTypeError("[SCALL-INTERNAL] in function " + funcName + ": call number of arguments (" + std::to_string(args.size()) + ") and parameters (0) don't match");
                }       
            }
        } 

        if (auto ptr = dynamic_cast<Ptr*>(calleeType)) {
            if (auto fn = dynamic_cast<Fn*>(ptr->ref.get())) {
                if (fn->params.size() != args.size()) {
                    addTypeError("[SCALL-INTERNAL] in function " + funcName + ": call number of arguments (" + std::to_string(args.size()) + ") and parameters (" + std::to_string(fn->params.size()) + ") don't match");
                }
                for (size_t i = 0; i < args.size(); ++i) {
                    Type* argType = args[i]->judgement(funcName, gamma, delta);
                    if (!fn->params[i]->operator==(*argType)) {
                        addTypeError("[SCALL-INTERNAL] in function " + funcName + ": call argument has type " + argType->toString() + " but parameter has type " + fn->params[i]->toString());
                    }
                }
                return;
            }    //  not internal; check if its external
            else{
                addTypeError("[SCALL-*] in function " + funcName + ": calling non-function type " + calleeType->toString()); 
                return;           
            }
        }   
        
        else if (auto fn = dynamic_cast<Fn*>(calleeType)) {
            if (fn->params.size() != args.size()) {
                addTypeError("[SCALL-EXTERN] in function " + funcName + ": call number of arguments (" + std::to_string(args.size()) + ") and parameters (" + std::to_string(fn->params.size()) + ") don't match");
            }
            for (size_t i = 0; i < args.size(); ++i) {
                Type* argType = args[i]->judgement(funcName, gamma, delta);
                if (!fn->params[i]->operator==(*argType)) {
                    addTypeError("[SCALL-EXTERN] in function " + funcName + ": call argument has type " + argType->toString() + " but parameter has type " + fn->params[i]->toString());
                }
            }
        } else {
            if (!dynamic_cast<Any*>(calleeType)){
                if (!isMain){     
                    addTypeError("[SCALL-*] in function " + funcName + ": calling non-function type " + calleeType->toString());         
                }
            }
        }
    }
};

// Function definition
struct Function {
    FuncId name;
    std::vector<Decl> params;
    std::unique_ptr<Type> rettyp;
    std::vector<std::pair<Decl, std::unique_ptr<Exp>>> locals;
    std::vector<std::unique_ptr<Stmt>> stmts;
};

// Program structure
struct Program {
    std::vector<Decl> globals;
    std::vector<Struct> structs;
    std::vector<Decl> externs;
    std::vector<Function> functions;
};
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <optional>
#include <sstream>
#include <map>
#include <stdexcept>
#include <fstream>

enum class TokenType {
    Fn, Id, OpenParen, CloseParen, Arrow, Int, OpenBrace, CloseBrace, Semicolon,
    Let, Struct, Colon, Comma, Return, Num, Nil, If, Else, While, Break, Continue,
    Assign, Star, Amp, Plus, Minus, Equal, NotEq, Lt, Lte, Gt, Gte, Dot, New, Extern,Underscore, OpenBracket, CloseBracket,Slash
};

struct Token {
    TokenType type;
    std::string value;
    Token(TokenType type, std::string value = "") : type(type), value(std::move(value)) {}
};

std::map<std::string, TokenType> tokenMap = {
        {"Slash", TokenType::Slash},
       {"Underscore", TokenType::Underscore},
    {"OpenBracket", TokenType::OpenBracket},
    {"CloseBracket", TokenType::CloseBracket},
    {"Fn", TokenType::Fn},
    {"OpenParen", TokenType::OpenParen},
    {"CloseParen", TokenType::CloseParen},
    {"Arrow", TokenType::Arrow},
    {"Int", TokenType::Int},
    {"OpenBrace", TokenType::OpenBrace},
    {"CloseBrace", TokenType::CloseBrace},
    {"Semicolon", TokenType::Semicolon},
    {"Let", TokenType::Let},
    {"Struct", TokenType::Struct},
    {"Colon", TokenType::Colon},
    {"Comma", TokenType::Comma},
    {"Return", TokenType::Return},
    {"Nil", TokenType::Nil},
    {"If", TokenType::If},
    {"Else", TokenType::Else},
    {"While", TokenType::While},
    {"Break", TokenType::Break},
    {"Continue", TokenType::Continue},
    {"Gets", TokenType::Assign},
    {"Star", TokenType::Star},
    {"Address", TokenType::Amp},
    {"Plus", TokenType::Plus},
    {"Dash", TokenType::Minus},
    {"Equal", TokenType::Equal},
    {"NotEq", TokenType::NotEq},
    {"Lt", TokenType::Lt},
    {"Lte", TokenType::Lte},
    {"Gt", TokenType::Gt},
    {"Gte", TokenType::Gte},
    {"Dot", TokenType::Dot},
    {"New", TokenType::New},
    {"Extern", TokenType::Extern}
};

std::vector<Token> tokenize(const std::string& input) {
    std::vector<Token> tokens;
    std::istringstream iss(input);
    std::string word;
    while (iss >> word) {
        if (word.front() == 'N' && word.find('(') != std::string::npos) {
            auto start = word.find('(') + 1;
            auto end = word.find(')');
            std::string numStr = word.substr(start, end - start);
            tokens.emplace_back(TokenType::Num, numStr);
        } else if (word.front() == 'I' && word.find('(') != std::string::npos) {
            auto start = word.find('(') + 1;
            auto end = word.find(')');
            std::string idStr = word.substr(start, end - start);
            tokens.emplace_back(TokenType::Id, idStr);
        } else {
            auto found = tokenMap.find(word);
            if (found != tokenMap.end()) {
                tokens.emplace_back(found->second);
            }
        }
    }
    return tokens;
}

class Parser {
    std::vector<Token> tokens;
    size_t current = 0;

public:
    explicit Parser(std::vector<Token> tokens) : tokens(std::move(tokens)) {}

    std::unique_ptr<Program> parseProgram() {
        auto program = std::make_unique<Program>();
        //std::cout<<"current: "<<current<<" tokens.size(): "<<tokens.size()<<std::endl;
        while (current < tokens.size()) {
           // std::cout<<"currenttokentype: "<<(int)tokens[current].type<<std::endl;
            switch (tokens[current].type) {
                case TokenType::Fn:
                 //   std::cout<<"fn"<<std::endl;
                    program->functions.push_back(parseFunction());
                    break;
                case TokenType::Extern:
                    program->externs.push_back(parseExtern());
                    break;
                case TokenType::Struct:
                    program->structs.push_back(parseStruct());
                    break;
                case TokenType::Let:
               // std::cout<<"let"<<std::endl;
                expect(TokenType::Let);
                program->globals.push_back(parseDecl());
                //  can see a colon (more globals to come) or a semicolon (end of globals)
                while (tokens[current].type == TokenType::Comma) {
                    expect(TokenType::Comma);
                    program->globals.push_back(parseDecl());
                }
                expect(TokenType::Semicolon);
                break;
                default:
                //std::cout<<current<<std::endl;
            throw std::runtime_error("parse error at token " + std::to_string(current));
            }
        }
        return program;
    }

Function parseFunction() {
    expect(TokenType::Fn);
    auto funcName = expect(TokenType::Id).value;
    expect(TokenType::OpenParen);
    std::vector<Decl> params;
    if (tokens[current].type != TokenType::CloseParen) {
        params.push_back(parseDecl());
        while (tokens[current].type == TokenType::Comma) {
            expect(TokenType::Comma);
            params.push_back(parseDecl());
        }
    }
    expect(TokenType::CloseParen);
    expect(TokenType::Arrow);
    std::unique_ptr<Type> returnType = nullptr;
    if(tokens[current].type== TokenType::Underscore)
    {
        expect(TokenType::Underscore);
    }
    else if (tokens[current].type != TokenType::OpenBrace) {
        returnType = parseType();
    }
    expect(TokenType::OpenBrace);
    std::vector<std::unique_ptr<Stmt>> stmts;
    std::vector<std::pair<Decl, std::unique_ptr<Exp>>> locals;
    while (tokens[current].type != TokenType::CloseBrace) {
        if (tokens[current].type == TokenType::Let) {
            expect(TokenType::Let);
            auto decl = parseDecl();
            std::unique_ptr<Exp> init = nullptr;
            if (tokens[current].type == TokenType::Assign) {
                expect(TokenType::Assign);
                init = parseExp();
            }
            locals.push_back(std::make_pair(std::move(decl), std::move(init)));
            while (tokens[current].type == TokenType::Comma) {
                expect(TokenType::Comma);
                auto nextDecl = parseDecl();
                std::unique_ptr<Exp> nextInit = nullptr;
                if (tokens[current].type == TokenType::Assign) {
                    expect(TokenType::Assign);
                    nextInit = parseExp();
                }
                locals.push_back(std::make_pair(std::move(nextDecl), std::move(nextInit)));
            }
            expect(TokenType::Semicolon);
        } else {
            stmts.push_back(parseStmt());
        }
    }
    expect(TokenType::CloseBrace);
    return Function{funcName, std::move(params), std::move(returnType), std::move(locals), std::move(stmts)};
}


Decl parseExtern() {
    expect(TokenType::Extern);
    auto name = expect(TokenType::Id).value;
    expect(TokenType::Colon);
    if(tokens[current].type == TokenType::Underscore)
        throw std::runtime_error("parse error at token " + std::to_string(current));
    
    auto type = parseType();
    expect(TokenType::Semicolon);
    return Decl{name, std::move(type)};
}


Struct parseStruct() {
    expect(TokenType::Struct);
    auto name = expect(TokenType::Id).value;
    expect(TokenType::OpenBrace);
    std::vector<std::pair<std::string, std::unique_ptr<Type>>> fields;
    while (tokens[current].type != TokenType::CloseBrace) {
        auto decl = parseDecl();
        fields.emplace_back(decl.name, std::move(decl.type));
        if (tokens[current].type == TokenType::Comma) {
            expect(TokenType::Comma);
        }
    }
    expect(TokenType::CloseBrace);
    return Struct{name, std::move(fields)};
}

Decl parseDecl() {
    auto name = expect(TokenType::Id).value;
    expect(TokenType::Colon);
    auto type = parseType();
    return Decl{name, std::move(type)};
}
    std::unique_ptr<Stmt> parseStmt() {
        switch (tokens[current].type) {
            case TokenType::If:
                return parseIf();
            case TokenType::While:
                return parseWhile();
            case TokenType::Break:
                return parseBreak();
            case TokenType::Continue:
                return parseContinue();
            case TokenType::Return:
                return parseReturn();
            default:
                return parseAssignOrCall();
        }
    }

    std::unique_ptr<IfStmt> parseIf() {
        expect(TokenType::If);
        auto guard = parseExp();
        auto tt = parseBlock();
        std::vector<std::unique_ptr<Stmt>> ff;
        if (tokens[current].type == TokenType::Else) {
            expect(TokenType::Else);
            ff = parseBlock();
        }
        return std::make_unique<IfStmt>(IfStmt{std::move(guard), std::move(tt), std::move(ff)});
    }

    std::unique_ptr<WhileStmt> parseWhile() {
        expect(TokenType::While);
        auto guard = parseExp();
        auto body = parseBlock();
        return std::make_unique<WhileStmt>(WhileStmt{std::move(guard), std::move(body)});
    }

    std::unique_ptr<BreakStmt> parseBreak() {
        expect(TokenType::Break);
        expect(TokenType::Semicolon);
        return std::make_unique<BreakStmt>();
    }

    std::unique_ptr<ContinueStmt> parseContinue() {
        expect(TokenType::Continue);
        expect(TokenType::Semicolon);
        return std::make_unique<ContinueStmt>();
    }

    std::unique_ptr<ReturnStmt> parseReturn() {
        expect(TokenType::Return);
        std::unique_ptr<Exp> exp = nullptr;
        if (tokens[current].type != TokenType::Semicolon) {
            exp = parseExp();
        }
        expect(TokenType::Semicolon);
        return std::make_unique<ReturnStmt>(ReturnStmt{std::move(exp)});
    }

    std::unique_ptr<Stmt> parseAssignOrCall() {
        auto lval = parseLval();
        if (tokens[current].type == TokenType::Assign) {
            expect(TokenType::Assign);
            auto rhs = parseRhs();
            expect(TokenType::Semicolon);
            return std::make_unique<AssignStmt>(AssignStmt{std::move(lval), std::move(rhs)});
        } else {
            expect(TokenType::OpenParen);
            std::vector<std::unique_ptr<Exp>> args;
            if (tokens[current].type != TokenType::CloseParen) {
                args.push_back(parseExp());
                while (tokens[current].type == TokenType::Comma) {
                    expect(TokenType::Comma);
                    args.push_back(parseExp());
                }
            }
            expect(TokenType::CloseParen);
            expect(TokenType::Semicolon);
            return std::make_unique<CallStmt>(CallStmt{std::move(lval), std::move(args)});
        }
    }

std::unique_ptr<Lval> parseLval() {
    std::unique_ptr<Lval> lval;
    if (tokens[current].type == TokenType::Star) {
        expect(TokenType::Star);
        lval = std::make_unique<DerefLval>(DerefLval{parseLval()});
    } else {
        lval = std::make_unique<IdLval>(IdLval{expect(TokenType::Id).value});
    }
    while (true) {
        if (tokens[current].type == TokenType::OpenBracket) {
            expect(TokenType::OpenBracket);
            auto index = parseExp();
            expect(TokenType::CloseBracket);
            lval = std::make_unique<ArrayAccessLval>(ArrayAccessLval{std::move(lval), std::move(index)});
        } else if (tokens[current].type == TokenType::Dot) {
            expect(TokenType::Dot);
            auto field = expect(TokenType::Id).value;
            lval = std::make_unique<FieldAccessLval>(FieldAccessLval{std::move(lval), field});
        } else {
            break;
        }
    }
    return lval;
}

    std::unique_ptr<Rhs> parseRhs() {
        if (tokens[current].type == TokenType::New) {
            expect(TokenType::New);
            auto type = parseType();
            std::unique_ptr<Exp> amount = nullptr;
            if (tokens[current].type != TokenType::Semicolon) {
                amount = parseExp();
            }
        else {
            // Handle the default case: amount = Num(1)
            amount = std::make_unique<NumExp>(NumExp{1});
        }
            return std::make_unique<NewRhs>(NewRhs{std::move(type), std::move(amount)});
        } else {
            auto exp = parseExp();
            return std::make_unique<RhsExp>(RhsExp{std::move(exp)});
        }
    }

    std::unique_ptr<Exp> parseExp() {
        auto exp = parseExp1();
        while (tokens[current].type == TokenType::Equal || tokens[current].type == TokenType::NotEq ||
               tokens[current].type == TokenType::Lt || tokens[current].type == TokenType::Lte ||
               tokens[current].type == TokenType::Gt || tokens[current].type == TokenType::Gte) {
            auto op = parseBinOp();
            auto rhs = parseExp1();
            exp = std::make_unique<BinOpExp>(BinOpExp{op, std::move(exp), std::move(rhs)});
        }
        return exp;
    }

    std::unique_ptr<Exp> parseExp1() {
        auto exp = parseExp2();
        while (tokens[current].type == TokenType::Plus || tokens[current].type == TokenType::Minus) {
            auto op = parseBinOp();
            auto rhs = parseExp2();
            exp = std::make_unique<BinOpExp>(BinOpExp{op, std::move(exp), std::move(rhs)});
        }
        return exp;
    }

    std::unique_ptr<Exp> parseExp2() {
        auto exp = parseExp3();
        while (tokens[current].type == TokenType::Star || tokens[current].type == TokenType::Slash) {
            auto op = parseBinOp();
            auto rhs = parseExp3();
            exp = std::make_unique<BinOpExp>(BinOpExp{op, std::move(exp), std::move(rhs)});
        }
        return exp;
    }

    std::unique_ptr<Exp> parseExp3() {
        if (tokens[current].type == TokenType::Plus || tokens[current].type == TokenType::Minus ||
            tokens[current].type == TokenType::Star || tokens[current].type == TokenType::Amp) {
            auto op = parseUnOp();
            auto operand = parseExp3();
            return std::make_unique<UnOpExp>(UnOpExp{op, std::move(operand)});
        } else {
            return parseExp4();
        }
    }

std::unique_ptr<Exp> parseExp4() {
    auto exp = parseExp5();
    while (true) {
        if (tokens[current].type == TokenType::OpenBracket) {
            expect(TokenType::OpenBracket);
            auto index = parseExp();
            expect(TokenType::CloseBracket);
            exp = std::make_unique<ArrayAccessExp>(ArrayAccessExp{std::move(exp), std::move(index)});
        } else if (tokens[current].type == TokenType::Dot) {
            expect(TokenType::Dot);
            auto field = expect(TokenType::Id).value;
            exp = std::make_unique<FieldAccessExp>(FieldAccessExp{std::move(exp), field});
        } else if (tokens[current].type == TokenType::OpenParen) {
            expect(TokenType::OpenParen);
            std::vector<std::unique_ptr<Exp>> args;
            if (tokens[current].type != TokenType::CloseParen) {
                args.push_back(parseExp());
                while (tokens[current].type == TokenType::Comma) {
                    expect(TokenType::Comma);
                    args.push_back(parseExp());
                }
            }
            expect(TokenType::CloseParen);
            exp = std::make_unique<CallExp>(CallExp{std::move(exp), std::move(args)});
        } else {
            break;
        }
    }
    return exp;
}
    std::unique_ptr<Exp> parseExp5() {
        switch (tokens[current].type) {
            case TokenType::Num:
                //cout<<"numcase in parseexp5"<<endl;
                return std::make_unique<NumExp>(NumExp{std::stoi(expect(TokenType::Num).value)});
            case TokenType::Id:
                return std::make_unique<IdExp>(IdExp{expect(TokenType::Id).value});
            case TokenType::Nil:
                expect(TokenType::Nil);
                return std::make_unique<NilExp>();
            case TokenType::OpenParen: {
                expect(TokenType::OpenParen);
                auto exp = parseExp();
                expect(TokenType::CloseParen);
                return exp;
            }
            default:
            throw std::runtime_error("parse error at token " + std::to_string(current));
        }
    }

    BinaryOp parseBinOp() {
        switch (tokens[current].type) {
            case TokenType::Plus:
                expect(TokenType::Plus);
                return BinaryOp::Add;
            case TokenType::Minus:
                expect(TokenType::Minus);
                return BinaryOp::Sub;
            case TokenType::Star:
                expect(TokenType::Star);
                return BinaryOp::Mul;
            case TokenType::Slash:
                expect(TokenType::Slash);
                return BinaryOp::Div;
            case TokenType::Equal:
                expect(TokenType::Equal);
                return BinaryOp::Equal;
            case TokenType::NotEq:
                expect(TokenType::NotEq);
                return BinaryOp::NotEq;
            case TokenType::Lt:
                expect(TokenType::Lt);
                return BinaryOp::Lt;
            case TokenType::Lte:
                expect(TokenType::Lte);
                return BinaryOp::Lte;
            case TokenType::Gt:
                expect(TokenType::Gt);
                return BinaryOp::Gt;
            case TokenType::Gte:
                expect(TokenType::Gte);
                return BinaryOp::Gte;
            default:
            throw std::runtime_error("parse error at token " + std::to_string(current));
        }
    }

    UnaryOp parseUnOp() {
        switch (tokens[current].type) {
            case TokenType::Plus:
                expect(TokenType::Plus);
                return UnaryOp::Neg;
            case TokenType::Minus:
                expect(TokenType::Minus);
                return UnaryOp::Neg;
            case TokenType::Star:
                expect(TokenType::Star);
                return UnaryOp::Deref;
            case TokenType::Amp:
                expect(TokenType::Amp);
                return UnaryOp::Addr;
            default:
            throw std::runtime_error("parse error at token " + std::to_string(current));
        }
    }

std::unique_ptr<Type> parseType() {
    //cout<<tokens.size()<<endl;
    std::unique_ptr<Type> type;
    int pointerCount = 0;
    while (tokens[current].type == TokenType::Amp) {
        expect(TokenType::Amp);
        pointerCount++;
    }
    switch (tokens[current].type) {
        case TokenType::Underscore:
            expect(TokenType::Underscore);
            type = std::make_unique<Int>(); // Placeholder type for underscore
            break;
        case TokenType::Int:
            expect(TokenType::Int);
            type = std::make_unique<Int>();
            break;
        case TokenType::Id: {
            auto structName = expect(TokenType::Id).value;
            type = std::make_unique<Struct>(Struct{structName, {}});
            break;
        }
        case TokenType::OpenParen: {
            expect(TokenType::OpenParen);
            std::vector<std::unique_ptr<Type>> paramTypes;
            if (tokens[current].type != TokenType::CloseParen) {
                if (tokens[current].type == TokenType::Underscore&& pointerCount>0)
                    throw std::runtime_error("parse error at token " + std::to_string(current));
                paramTypes.push_back(parseType());
                while (tokens[current].type == TokenType::Comma) {
                    expect(TokenType::Comma);
                    paramTypes.push_back(parseType());
                }
            }
            expect(TokenType::CloseParen);
            std::unique_ptr<Type> retType = nullptr;
            if (tokens[current].type == TokenType::Arrow) {
                expect(TokenType::Arrow);
                if (tokens[current].type == TokenType::Underscore) {
                    expect(TokenType::Underscore);
                    // No return type (void)
                } else {
                    retType = parseType();
                }
            }
            type = std::make_unique<Fn>(Fn{std::move(paramTypes), std::move(retType)});
            break;
        }
        default:
            throw std::runtime_error("parse error at token " + std::to_string(current));
    }
    while (pointerCount > 0) {
        type = std::make_unique<Ptr>(Ptr{std::move(type)});
        pointerCount--;
    }
    return type;
}

    std::vector<std::unique_ptr<Stmt>> parseBlock() {
        expect(TokenType::OpenBrace);
        std::vector<std::unique_ptr<Stmt>> stmts;
        while (tokens[current].type != TokenType::CloseBrace) {
            stmts.push_back(parseStmt());
        }
        expect(TokenType::CloseBrace);
        return stmts;
    }

    Token& expect(TokenType type) {
        if (tokens[current].type != type) {
            //std::cout<<"expected: "<<(int)type<<" got: "<<(int)tokens[current].type<<std::endl;
            if(current==tokens.size())
            {
                throw std::runtime_error("parse error at token " + std::to_string(current-1));
            }
            throw std::runtime_error("parse error at token " + std::to_string(current));
        }
        return tokens[current++];
    }
};

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void printProgram(const Program& program);
void printDecl(const Decl& decl);
void printStruct(const Struct& s);
void printFunction(const Function& func);
void printType(const Type& type);
void printExp(const Exp& exp);
void printStmt(const Stmt& stmt);
void printLval(const Lval& lval);
void printRhs(const Rhs& rhs);


void printStruct(const Struct& s) {
    std::cout << "Struct(" << std::endl;
    std::cout << "  name = " << s.name << "," << std::endl;
    std::cout << "  fields = [";
    for (size_t i = 0; i < s.fields.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << "Decl(" << s.fields[i].first << ", ";
        printType(*s.fields[i].second);
        std::cout << ")";
    }
    std::cout << "]" << std::endl;
    std::cout << ")";
}


void printType(const Type& type) {
    if (const Int* i = dynamic_cast<const Int*>(&type)) {
        std::cout << "Int";
    } else if (const Struct* s = dynamic_cast<const Struct*>(&type)) {
        std::cout << "Struct(" << s->name << ")";
    } else if (const Fn* f = dynamic_cast<const Fn*>(&type)) {
        std::cout << "Fn(";
        std::cout << "prms = [";
        for (size_t i = 0; i < f->params.size(); ++i) {
            if (i > 0) std::cout << ", ";
            printType(*f->params[i]);
        }
        std::cout << "], ";
        std::cout << "ret = ";
        if (f->ret) {
            printType(*f->ret);
        } else {
            std::cout << "_";
        }
        std::cout << ")";
    } else if (const Ptr* p = dynamic_cast<const Ptr*>(&type)) {
        std::cout << "Ptr(";
        printType(*p->ref);
        std::cout << ")";
    }
}



void printDecl(const Decl& decl) {
    std::cout << "Decl(" << decl.name << ", ";
    printType(*decl.type);
    std::cout << ")";
}

void printFunction(const Function& func) {
    std::cout << "Function(" << std::endl;
    std::cout << "  name = " << func.name << "," << std::endl;
    std::cout << "  params = [";
    for (size_t i = 0; i < func.params.size(); ++i) {
        if (i > 0) std::cout << ", ";
        printDecl(func.params[i]);
    }
    std::cout << "]," << std::endl;
    std::cout << "  rettyp = ";
    if (func.rettyp) {
        printType(*func.rettyp);
    } else {
        std::cout << "_";
    }
    std::cout << "," << std::endl;
    std::cout << "  locals = [" << std::endl;
    for (size_t i = 0; i < func.locals.size(); ++i) {
        std::cout << "    (";
        printDecl(func.locals[i].first);
        if (func.locals[i].second) {
            std::cout << ", ";
            printExp(*func.locals[i].second);
        } else {
            std::cout << ", _";
        }
        std::cout << ")";
        if (i < func.locals.size() - 1) {
            std::cout << "," << std::endl;
        }
    }
    std::cout << std::endl << "  ]," << std::endl;
    std::cout << "  stmts = [" << std::endl;
    for (size_t i = 0; i < func.stmts.size(); ++i) {
        std::cout << "    ";
        printStmt(*func.stmts[i]);
        if (i < func.stmts.size() - 1) {
            std::cout << "," << std::endl;
        }
    }
    std::cout << std::endl << "  ]" << std::endl;
    std::cout << ")";
}

void printBinOp(const BinaryOp& op) {
    switch (op) {
        case BinaryOp::Add: std::cout << "Add"; break;
        case BinaryOp::Sub: std::cout << "Sub"; break;
        case BinaryOp::Mul: std::cout << "Mul"; break;
        case BinaryOp::Div: std::cout << "Div"; break;
        case BinaryOp::Equal: std::cout << "Equal"; break;
        case BinaryOp::NotEq: std::cout << "NotEq"; break;
        case BinaryOp::Lt: std::cout << "Lt"; break;
        case BinaryOp::Lte: std::cout << "Lte"; break;
        case BinaryOp::Gt: std::cout << "Gt"; break;
        case BinaryOp::Gte: std::cout << "Gte"; break;
    }
}

void printExp(const Exp& exp) {
    if (const NumExp* n = dynamic_cast<const NumExp*>(&exp)) {
        std::cout << "Num(" << n->n << ")";
    } else if (const IdExp* i = dynamic_cast<const IdExp*>(&exp)) {
        std::cout << "Id(" << i->name << ")";
    } else if (dynamic_cast<const NilExp*>(&exp)) {
        std::cout << "Nil";
    } else if (const UnOpExp* u = dynamic_cast<const UnOpExp*>(&exp)) {
        switch (u->op) {
            case UnaryOp::Neg:
                std::cout << "Neg(";
                printExp(*u->operand);
                std::cout << ")";
                break;
            case UnaryOp::Deref:
                std::cout << "Deref(";
                printExp(*u->operand);
                std::cout << ")";
                break;
        }
    } else if (const BinOpExp* b = dynamic_cast<const BinOpExp*>(&exp)) {
        std::cout << "BinOp(" << std::endl;
        std::cout << "  op = ";
        switch (b->op) {
            case BinaryOp::Add: std::cout << "Add"; break;
            case BinaryOp::Sub: std::cout << "Sub"; break;
            case BinaryOp::Mul: std::cout << "Mul"; break;
            case BinaryOp::Div: std::cout << "Div"; break;
            case BinaryOp::Equal: std::cout << "Equal"; break;
            case BinaryOp::NotEq: std::cout << "NotEq"; break;
            case BinaryOp::Lt: std::cout << "Lt"; break;
            case BinaryOp::Lte: std::cout << "Lte"; break;
            case BinaryOp::Gt: std::cout << "Gt"; break;
            case BinaryOp::Gte: std::cout << "Gte"; break;
        }
        std::cout << "," << std::endl;
        std::cout << "  left = ";
        printExp(*b->left);
        std::cout << "," << std::endl;
        std::cout << "  right = ";
        printExp(*b->right);
        std::cout << std::endl << ")";
    } else if (const ArrayAccessExp* a = dynamic_cast<const ArrayAccessExp*>(&exp)) {
        std::cout << "ArrayAccess(" << std::endl;
        std::cout << "  ptr = ";
        printExp(*a->ptr);
        std::cout << "," << std::endl;
        std::cout << "  index = ";
        printExp(*a->index);
        std::cout << std::endl << ")";
    } else if (const FieldAccessExp* f = dynamic_cast<const FieldAccessExp*>(&exp)) {
        std::cout << "FieldAccess(" << std::endl;
        std::cout << "  ptr = ";
        printExp(*f->ptr);
        std::cout << "," << std::endl;
        std::cout << "  field = " << f->field << std::endl;
        std::cout << ")";
    } else if (const CallExp* c = dynamic_cast<const CallExp*>(&exp)) {
        std::cout << "Call(" << std::endl;
        std::cout << "  callee = ";
        printExp(*c->callee);
        std::cout << "," << std::endl;
        std::cout << "  args = [";
        for (size_t i = 0; i < c->args.size(); ++i) {
            if (i > 0) std::cout << ", ";
            printExp(*c->args[i]);
        }
        std::cout << "]" << std::endl << ")";
    }
}

void printStmt(const Stmt& stmt) {
    if (dynamic_cast<const BreakStmt*>(&stmt)) {
        std::cout << "Break";
    } else if (dynamic_cast<const ContinueStmt*>(&stmt)) {
        std::cout << "Continue";
    } else if (const ReturnStmt* r = dynamic_cast<const ReturnStmt*>(&stmt)) {
        std::cout << "Return(" << std::endl;
        if (r->exp) {
            std::cout << "  ";
            printExp(*r->exp);
            std::cout << std::endl;
        }
        else cout<<"_";
        std::cout << ")";
    } else if (const AssignStmt* a = dynamic_cast<const AssignStmt*>(&stmt)) {
        std::cout << "Assign(" << std::endl;
        std::cout << "  lhs = ";
        printLval(*a->lhs);
        std::cout << "," << std::endl;
        std::cout << "  rhs = ";
        printRhs(*a->rhs);
        std::cout << std::endl << ")";
    } else if (const CallStmt* c = dynamic_cast<const CallStmt*>(&stmt)) {
        std::cout << "Call(" << std::endl;
        std::cout << "  callee = ";
        printLval(*c->callee);
        std::cout << "," << std::endl;
        std::cout << "  args = [";
        for (size_t i = 0; i < c->args.size(); ++i) {
            if (i > 0) std::cout << ", ";
            printExp(*c->args[i]);
        }
        std::cout << "]" << std::endl << ")";
    } else if (const IfStmt* i = dynamic_cast<const IfStmt*>(&stmt)) {
        std::cout << "If(" << std::endl;
        std::cout << "  guard = ";
        printExp(*i->guard);
        std::cout << "," << std::endl;
        std::cout << "  tt = [" << std::endl;
        for (size_t j = 0; j < i->tt.size(); ++j) {
            std::cout << "    ";
            printStmt(*i->tt[j]);
            if (j < i->tt.size() - 1) {
                std::cout << "," << std::endl;
            }
        }
        std::cout << std::endl << "  ]," << std::endl;
        std::cout << "  ff = [" << std::endl;
        for (size_t j = 0; j < i->ff.size(); ++j) {
            std::cout << "    ";
            printStmt(*i->ff[j]);
            if (j < i->ff.size() - 1) {
                std::cout << "," << std::endl;
            }
        }
        std::cout << std::endl << "  ]" << std::endl;
        std::cout << ")";
    } else if (const WhileStmt* w = dynamic_cast<const WhileStmt*>(&stmt)) {
        std::cout << "While(" << std::endl;
        std::cout << "  guard = ";
        printExp(*w->guard);
        std::cout << "," << std::endl;
        std::cout << "  body = [" << std::endl;
        for (size_t i = 0; i < w->body.size(); ++i) {
            std::cout << "    ";
            printStmt(*w->body[i]);
            if (i < w->body.size() - 1) {
                std::cout << "," << std::endl;
            }
        }
        std::cout << std::endl << "  ]" << std::endl;
        std::cout << ")";
    }
}

void printLval(const Lval& lval) {
    if (const IdLval* i = dynamic_cast<const IdLval*>(&lval)) {
        std::cout << "Id(" << i->name << ")";
    } else if (const DerefLval* d = dynamic_cast<const DerefLval*>(&lval)) {
        std::cout << "Deref(";
        printLval(*d->lval);
        std::cout << ")";
    } else if (const ArrayAccessLval* a = dynamic_cast<const ArrayAccessLval*>(&lval)) {
        std::cout << "ArrayAccess(" << std::endl;
        std::cout << "  ptr = ";
        printLval(*a->ptr);
        std::cout << "," << std::endl;
        std::cout << "  index = ";
        printExp(*a->index);
        std::cout << std::endl << ")";
    } else if (const FieldAccessLval* f = dynamic_cast<const FieldAccessLval*>(&lval)) {
        std::cout << "FieldAccess(" << std::endl;
        std::cout << "  ptr = ";
        printLval(*f->ptr);
        std::cout << "," << std::endl;
        std::cout << "  field = " << f->field << std::endl;
        std::cout << ")";
    }
}

void printRhs(const Rhs& rhs) {
    if (const RhsExp* e = dynamic_cast<const RhsExp*>(&rhs)) {
        printExp(*e->exp);
    } else if (const NewRhs* n = dynamic_cast<const NewRhs*>(&rhs)) {
        std::cout << "New(";
        printType(*n->type);
        if (n->amount) {
            std::cout << ", ";
            printExp(*n->amount);
        }
        std::cout << ")";
    }
}
void printProgram(const Program& program) {
    std::cout << "Program(" << std::endl;
    std::cout << "  globals = [";
    for (size_t i = 0; i < program.globals.size(); ++i) {
        if (i > 0) std::cout << ", ";
        printDecl(program.globals[i]);
    }
    std::cout << "]," << std::endl;
    std::cout << "  structs = [";
    for (size_t i = 0; i < program.structs.size(); ++i) {
        if (i > 0) std::cout << ", ";
        printStruct(program.structs[i]);
    }
    std::cout << "]," << std::endl;
    std::cout << "  externs = [";
    for (size_t i = 0; i < program.externs.size(); ++i) {
        if (i > 0) std::cout << ", ";
        printDecl(program.externs[i]);
    }
    std::cout << "]," << std::endl;
    std::cout << "  functions = [";
    for (size_t i = 0; i < program.functions.size(); ++i) {
        if (i > 0) std::cout << ", ";
        printFunction(program.functions[i]);
    }
    std::cout << "]" << std::endl;
    std::cout << ")" << std::endl;
}





// ------------------------------------------------------------------------------------------------ //


/*   Errors:
[FUNCTION] in function {}: variable {} has a struct or function type

[FUNCTION] in function {}: variable {} with type {} has initializer of type {}
*/
void function_check(const Program& program, std::unordered_map<std::string, std::unique_ptr<Type>>& gammaR0, const std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<Type>>>& delta) {
    // initialize Γ′ = prms ⊔ locals.decls
    for (const auto& func : program.functions) {
        std::unordered_map<std::string, std::unique_ptr<Type>> gammaPrime;
        for (const auto& param : func.params) {
            gammaPrime[param.name] = param.type->clone();
        }
        for (const auto& local : func.locals) {
            gammaPrime[local.first.name] = local.first.type->clone();    //  if the local declaration overrides a parameter declaration with the same name
        }

    // type check each Decl in gammaPrime
        for (const auto& decl : gammaPrime) {
            if (dynamic_cast<Struct*>(decl.second.get()) || dynamic_cast<Fn*>(decl.second.get())) {
                addTypeError("[FUNCTION] in function " + func.name + ": variable " + decl.first + " has a struct or function type");
            }
        }


         // similarily, define Γ′′ = Γ⊔Γ′ where any entry from Γ′ overrides an entry with the same name in Γ
        std::unordered_map<std::string, std::unique_ptr<Type>> gammaDoublePrime;
        for (auto& entry : gammaR0) {
            gammaDoublePrime[entry.first] = entry.second->clone();
        }
        for (auto& entry : gammaPrime) {
            gammaDoublePrime[entry.first] = std::move(entry.second);
        }

        // for (const auto& entry : gammaDoublePrime) {
        //     string second;
        //     if (!entry.second.get()) {
        //         second = "NULL";
        //     }
        //     else{
        //         second = entry.second.get()->toString();
        //     }
        //     std::cout << entry.first << " : " << second << std::endl;
        // }
    // run judgement on the EXP of each local variable in the function using judgement method
        for (const auto& local : func.locals) {
            // report error if the type of the initializer does not match the type of the variable
            if (local.second) {
                auto init_type = local.second->judgement(func.name, gammaDoublePrime, delta);
                if (dynamic_cast<Any*>(local.first.type.get()) || dynamic_cast<Any*>(init_type)){
                    continue;
                }
                else if (!(local.first.type.get()->operator==(*init_type))) {
                    addTypeError("[FUNCTION] in function " + func.name + ": variable " + local.first.name +
                     " with type " + local.first.type->toString() + " has initializer of type " +
                      init_type->toString());
                }
            }
        }
        
        for (const auto& stmt : func.stmts) {
            if (func.rettyp){
                // run judgement on the EXP of each stmt in the function using judgement method
                stmt->judgement(func.name, false, func.rettyp->clone(), gammaDoublePrime, delta);
            }
            else{
                // run judgement on the EXP of each stmt in the function using judgement method
                stmt->judgement(func.name, false, nullptr, gammaDoublePrime, delta);
            }
        }
    }
}


void global_struct_check(const Program& program, const std::unordered_map<std::string, std::unique_ptr<Type>>& gammaR0, const std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<Type>>>& delta) {
    // Check that the type of the Decl is not a struct type or a function type
    for (const auto& decl : program.globals) {
        if (dynamic_cast<Struct*>(decl.type.get()) || dynamic_cast<Fn*>(decl.type.get())){
            addTypeError("[GLOBAL] global " + decl.name + " has a struct or function type");
        }
    }
    // check that for each Decl in fields of each struct, the type is not a struct type or a function type
    for (const auto& s : program.structs) {
        for (const auto& field : s.fields) {
            if (dynamic_cast<Struct*>(field.second.get()) || dynamic_cast<Fn*>(field.second.get())) {
                addTypeError("[STRUCT] struct " + s.name + " field " + field.first + " has a struct or function type");
            } 
        }
    }
}

// ** gammaR0 holds string->type info for: 
//global variables  |  extern declared functions  |  internal defined functions (except main)

void type_check(const Program& program, std::unordered_map<std::string, std::unique_ptr<Type>>& gammaR0, const std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<Type>>>& delta) {
    global_struct_check(program, gammaR0, delta);   
    function_check(program, gammaR0, delta);
}



int main(int argc, char* argv[]) {

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }

    std::string filename = argv[1];
    std::ifstream inputFile(filename);
    if (!inputFile) {
        std::cerr << "Error: Failed to open file " << filename << std::endl;
        return 1;
    }

    std::vector<Token> tokens;
    std::string line;
    while (std::getline(inputFile, line)) {
        if (line.empty()) continue;
        std::istringstream iss(line);
        std::string tokenStr;
        if (iss >> tokenStr) {
            if (tokenStr.substr(0, 3) == "Id(") {
                tokens.emplace_back(TokenType::Id, tokenStr.substr(3, tokenStr.length() - 4));
            } else if (tokenStr.substr(0, 4) == "Num(") {
                tokens.emplace_back(TokenType::Num, tokenStr.substr(4, tokenStr.length() - 5));
            } else {
                auto it = tokenMap.find(tokenStr);
                if (it != tokenMap.end()) {
                    tokens.emplace_back(it->second);
                } else {
                    std::cerr << "Error: Unknown token '" << tokenStr << "'" << std::endl;
                    return 1;
                }
            }
        }
    }

    Parser parser(std::move(tokens));
    try {
        auto program = parser.parseProgram();
        printProgram(*program);
        std::unordered_map<std::string, std::unique_ptr<Type>> gammaR0;
        std::unordered_map<StructId, std::unordered_map<std::string, std::unique_ptr<Type>>> delta;

        // initialize Γ0 and ∆
        for (const auto& decl : program->globals) {
            gammaR0[decl.name] = decl.type->clone();  
        }
        for (const auto& str : program->structs) {
            std::unordered_map<std::string, std::unique_ptr<Type>> fields;
            for (const auto& field : str.fields) {
                fields[field.first] = field.second->clone();
            }
            delta[str.name] = std::move(fields);
        }
        for (const auto& ext : program->externs) {    // map the function name to its function type
            gammaR0[ext.name] = ext.type->clone();
        }
        for (const auto& func : program->functions) {
            if (func.name == "main") {
                gammaR0[func.name] = func.rettyp->clone();      // continue;  // we can add main function to emit Exp errors if we need the return Type for main
            }                                              // but need to check if main is called in the body of another function
            //  map the function name to a pointer to its function type
            else {
                std::vector<std::unique_ptr<Type>> prms;    //  extract the parameter types to build fn type
                for (const auto& param : func.params) {
                    prms.push_back(param.type->clone());
                }
                unique_ptr<Type> ret = nullptr;
                if (func.rettyp){
                    ret = func.rettyp->clone();
                }
                auto funcType = std::make_unique<Fn>(std::move(prms), std::move(ret));    // segfault
                auto ptrType = std::make_unique<Ptr>(std::move(funcType));
                gammaR0[func.name] = std::move(ptrType);
            }
        }
        // print contents of gamma 
        // for (const auto& entry : gammaR0) {
        //     std::cout << entry.first << " -> ";
        //     printType(*entry.second);
        //     std::cout << std::endl;
        // }

        type_check(*program, gammaR0, delta);
        reportTypeErrors();
        //print out gamma's contents
        // for (const auto& entry : gammaR0) {
        //     if (entry.first == "foo1"){
        //     std::cout << entry.first << " -> ";
        //     printType(*entry.second);
        //     std::cout << std::endl;
        //     std::cout << "printing out all param types for foo1" << std::endl;
        //     auto fn = dynamic_cast<Fn*>(entry.second.get());
        //     for (const auto& param : fn->params) {
        //         printType(*param);
        //         std::cout << std::endl;
        //     }
        //     }
           
        // }
        

    } catch (const std::exception& e) {
        cout  << e.what() <<endl;
    }

    return 0;
}


/*
We initialize Γ0 and ∆ from the the AST Program node’s globals, structs, externs, and functions
fields. Note that for a function inside externs the function name is mapped to its function type, while for
a function inside functions the function name is mapped to a pointer to the function type. Also, we don’t
include the main function in Γ0. At this point, Γ0 has all the type information for global variables, extern
declared functions, and internal defined functions (except main) and ∆ has the typing information for all
struct declarations
*/