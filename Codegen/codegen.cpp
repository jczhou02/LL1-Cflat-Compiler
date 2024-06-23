#include "json.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
using json = nlohmann::json;

class LIRToX86CodeGenerator {
public:
    LIRToX86CodeGenerator(const std::string& lirProgram) : lirProgram(lirProgram), lirJson(json::parse(lirProgram)) {
        buildStructFieldOffsets(lirJson);
    }

    std::string generate() {
        generateAssembly();
        return getAssemblyCode();
    }

private:
    std::string lirProgram;
    nlohmann::json lirJson;
    std::vector<std::string> instructions;
    std::unordered_map<std::string, int> localOffsets;
    int stackSize;
    std::unordered_map<std::string, std::unordered_map<std::string, int>> structFieldOffsets;

    void buildStructFieldOffsets(const json& json) {
        if (json.is_object() && json.contains("structs")) {
            const auto& structs = json["structs"];
            for (const auto& structItem : structs.items()) {
                std::string structName = structItem.key();
                int offset = 0;
                const auto& fields = structItem.value();
                for (const auto& field : fields) {

                    // Check for 'name' field
                    if (!field.contains("name") || field["name"].is_null() || !field["name"].is_string()) {
                        std::cerr << "Invalid or missing 'name' in field definition of struct: " << structName << std::endl;
                        throw std::runtime_error("Invalid field definition in struct: " + structName);
                    }

                    // Check for 'typ' field
                    if (!field.contains("typ") || field["typ"].is_null()) {
                        std::cerr << "Missing 'typ' in field definition of struct: " << structName << std::endl;
                        throw std::runtime_error("Invalid field definition in struct: " + structName);
                    }

                    std::string fieldName = field["name"];
                    structFieldOffsets[structName][fieldName] = offset;

                    // Calculate offset based on field type size
                    const auto& typ = field["typ"];
                    if (typ.is_object() && typ.contains("Ptr")) {
                        const auto& ptrType = typ["Ptr"];
                        if (ptrType.is_object() || ptrType.is_string()) {
                            offset += 8; // Assuming pointer size is 8 bytes
                        } else {
                            std::cerr << "Invalid 'Ptr' type in field definition of struct: " << structName << std::endl;
                            throw std::runtime_error("Invalid 'Ptr' type in field definition of struct: " + structName);
                        }
                    } else if (typ.is_string()) {
                        offset += 8; // Assuming all other types (like "Int") are 8 bytes
                    } else {
                        std::cerr << "Invalid 'typ' in field definition of struct: " << structName << std::endl;
                        throw std::runtime_error("Invalid 'typ' in field definition of struct: " + structName);
                    }
                }
            }
        } else {
            std::cerr << "Invalid JSON structure for structs" << std::endl;
            throw std::runtime_error("Invalid JSON structure for structs");
        }
    }

    int getFieldOffset(const std::string& structName, const std::string& fieldName) const {
        auto structIt = structFieldOffsets.find(structName);
        if (structIt != structFieldOffsets.end()) {
            auto fieldIt = structIt->second.find(fieldName);
            if (fieldIt != structIt->second.end()) {
                return fieldIt->second;
            }
        }
        return -1; // return invalid offset if not found
    }

    void processInstruction(const json& inst) {
        if (inst.contains("Copy")) { // COPY INSTRUCTIONS
            auto copyInst = inst["Copy"];
            std::string lhs = copyInst["lhs"]["name"];
            std::string lhsAccess = getAccessMode(lhs); // determine lhs access mode (local or global)

            std::string rhs;
            std::string rhsAccess;

            // determine rhs access mode (local or global)
            if (copyInst["op"].contains("CInt")) {
                rhs = "$" + std::to_string(static_cast<int>(copyInst["op"]["CInt"]));
            } else if (copyInst["op"].contains("Var")) {
                rhs = copyInst["op"]["Var"]["name"];
                rhsAccess = getAccessMode(rhs);

                //check if variable type is a function pointer
                if (copyInst["op"]["Var"]["typ"].contains("Ptr") && copyInst["op"]["Var"]["typ"]["Ptr"].contains("Fn")) {
                    // check if variable is global
                    if (copyInst["op"]["Var"]["scope"].is_null() || copyInst["op"]["Var"]["scope"] == "global") {
                        rhs = rhs + "_";
                        rhsAccess = rhs + "(%rip)";
                    }
                }
            }

            // Emit appropriate movq instructions
            if (copyInst["op"].contains("CInt")) {
                emit("  movq " + rhs + ", " + lhsAccess);
            } else {
                emit("  movq " + rhsAccess + ", %r8");
                
                // emit movq with underscore for global function pointers
                if (copyInst["lhs"]["typ"].contains("Ptr") && copyInst["lhs"]["typ"]["Ptr"].contains("Fn") &&
                    (copyInst["lhs"]["scope"].is_null() || copyInst["lhs"]["scope"] == "global")) {
                        lhs += "_";
                }

                lhsAccess = getAccessMode(lhs);

                emit("  movq %r8, " + lhsAccess);
            }
        } else if (inst.contains("Arith")) { // ARITHMETIC INSTRUCTIONS
            auto arithInst = inst["Arith"];
            std::string lhs = arithInst["lhs"]["name"];
            std::string op = arithInst["aop"];
            std::string rhs1 = getOperandAccess(arithInst["op1"]);
            std::string rhs2 = getOperandAccess(arithInst["op2"]);

            if (op == "Add") {
                emit("  movq " + rhs1 + ", %r8");
                emit("  addq " + rhs2 + ", %r8");
                emit("  movq %r8, " + std::to_string(localOffsets[lhs]) + "(%rbp)");
            } else if (op == "Subtract") {
                emit("  movq " + rhs1 + ", %r8");
                emit("  subq " + rhs2 + ", %r8");
                emit("  movq %r8, " + std::to_string(localOffsets[lhs]) + "(%rbp)");
            } else if (op == "Multiply") {
                emit("  movq " + rhs1 + ", %r8");
                emit("  imulq " + rhs2 + ", %r8");
                emit("  movq %r8, " + std::to_string(localOffsets[lhs]) + "(%rbp)");
            } else if (op == "Divide") {
                emit("  movq " + rhs1 + ", %rax");
                emit("  cqo");
                if (arithInst["op2"].contains("CInt")) {
                    emit("  movq " + rhs2 + ", %r8");
                    emit("  idivq %r8");
                } else {
                    emit("  idivq " + rhs2);
                }
                emit("  movq %rax, " + std::to_string(localOffsets[lhs]) + "(%rbp)");
            }
        } else if (inst.contains("Cmp")) { // COMPARE INSTRUCTIONS
            auto cmpInst = inst["Cmp"];
            std::string lhs = cmpInst["lhs"]["name"];
            std::string rop = cmpInst["rop"];
            std::string op1 = getOperandAccess(cmpInst["op1"]);
            std::string op2 = getOperandAccess(cmpInst["op2"]);

            if (cmpInst["op1"].contains("CInt")) {
                emit("  movq " + op1 + ", %r8");
                emit("  cmpq " + op2 + ", %r8");
            } else if (cmpInst["op2"].contains("CInt")) {
                emit("  cmpq " + op2 + ", " + op1);
            } else { // both operands are variables
                emit("  movq " + op1 + ", %r8");
                emit("  cmpq " + op2 + ", %r8");
            }

            std::string setInstr;
            if (rop == "Eq") setInstr = "sete";
            else if (rop == "Neq") setInstr = "setne";
            else if (rop == "Less") setInstr = "setl";
            else if (rop == "LessEq") setInstr = "setle";
            else if (rop == "Greater") setInstr = "setg";
            else if (rop == "GreaterEq") setInstr = "setge";

            emit("  movq $0, %r8");
            emit("  " + setInstr + " %r8b");
            emit("  movq %r8, " + std::to_string(localOffsets[lhs]) + "(%rbp)");
        } else if (inst.contains("CallExt")) { // EXTERNAL CALL INSTRUCTIONS
            auto callExtInst = inst["CallExt"];
            if (callExtInst.contains("ext_callee") && !callExtInst["ext_callee"].is_null()) {
                std::string extCallee = callExtInst["ext_callee"];
                std::string lhs;
                if (callExtInst.contains("lhs") && !callExtInst["lhs"].is_null() && callExtInst["lhs"].contains("name") && callExtInst["lhs"]["name"].is_string()) {
                    lhs = callExtInst["lhs"]["name"];
                }
                std::vector<std::string> argRegisters = {"%rdi", "%rsi", "%rdx", "%rcx", "%r8", "%r9"};
                int numArgs = callExtInst["args"].size();
                int numStackArgs = numArgs > 6 ? numArgs - 6 : 0;

                // emit instructions for first 6 arguments
                for (int i = 0; i < std::min(numArgs, 6); ++i) {
                    const auto& arg = callExtInst["args"][i];
                    if (arg.contains("Var")) {
                        std::string varName = arg["Var"]["name"];
                        std::string varAccess = getAccessMode(varName);
                        emit("  movq " + varAccess + ", " + argRegisters[i]);
                    } else if (arg.contains("CInt")) {
                        int constValue = arg["CInt"];
                        emit("  movq $" + std::to_string(constValue) + ", " + argRegisters[i]);
                    }
                }
                    
                // emit instructions for additional arguments to be pushed onto the stack
                for (int i = numArgs - 1; i >= 6; --i) {
                    const auto& arg = callExtInst["args"][i];
                    if (arg.contains("Var")) {
                        std::string varName = arg["Var"]["name"];
                        std::string varAccess = getAccessMode(varName);
                        emit("  pushq " + varAccess);
                    } else if (arg.contains("CInt")) {
                        int constValue = arg["CInt"];
                        emit("  pushq $" + std::to_string(constValue));
                    }
                }

                if (numStackArgs > 0 && numStackArgs % 2 != 0) {
                    emit("  subq $8, %rsp");
                }

                // generate call to external function
                emit("  call " + extCallee);

                // move result from %rax to lhs if lhs is NOT null
                if (callExtInst.contains("lhs") && !callExtInst["lhs"].is_null()) {
                    // std::string lhs = callExtInst["lhs"]["name"];
                    emit("  movq %rax, " + std::to_string(localOffsets[lhs]) + "(%rbp)");
                }

                // adjust stack pointer to remove pushed arguments plus any alignment adjustment
                if (numStackArgs > 0) {
                    int stackAdjustment = numStackArgs * 8;
                    if (numStackArgs % 2 != 0) {
                        stackAdjustment += 8;
                    }
                    emit("  addq $" + std::to_string(stackAdjustment) + ", %rsp");
                }
            } else {
                throw std::runtime_error("Malformed CallExt instruction: ext_callee is missing or null");
            }
        } else if (inst.contains("Load")) { // LOAD INSTRUCTIONS
            auto loadInst = inst["Load"];
            std::string lhs = loadInst["lhs"]["name"];
            std::string lhsAccess = getAccessMode(lhs);

            std::string src = loadInst["src"]["name"];
            std::string srcAccess = getAccessMode(src);

            emit("  movq " + srcAccess + ", %r8"); // load address of source into %r8
            emit("  movq 0(%r8), %r9"); // load value at address in %r8 into %r9
            emit("  movq %r9, " + lhsAccess); // store value from %r9 into destination
        } else if (inst.contains("Gep")) { // GET ELEMENT POINTER INSTRUCTIONS
            auto gepInst = inst["Gep"];
            std::string lhs = gepInst["lhs"]["name"];
            std::string lhsAccess = getAccessMode(lhs);

            std::string src = gepInst["src"]["name"];
            std::string srcAccess = getAccessMode(src);

            std::string idx, idxAccess;
            bool isConstantIndex = false;
            if (gepInst["idx"].contains("Var")) {
                idx = gepInst["idx"]["Var"]["name"];
                idxAccess = getAccessMode(idx);
            } else if (gepInst["idx"].contains("CInt")) {
                idx = "$" + std::to_string(static_cast<int>(gepInst["idx"]["CInt"]));
                isConstantIndex = true;
            }

            // generate assembly code for GEP instruction
            if (isConstantIndex) {
                emit("  movq " + idx + ", %r8"); // load constant index into %r8
            } else {
                emit("  movq " + idxAccess + ", %r8");
            }

            emit("  cmpq $0, %r8");
            emit("  jl .out_of_bounds");
            emit("  movq " + srcAccess + ", %r9");
            emit("  movq -8(%r9), %r10");
            emit("  cmpq %r10, %r8");
            emit("  jge .out_of_bounds");
            emit("  imulq $8, %r8");
            emit("  addq %r9, %r8");
            emit("  movq %r8, " + lhsAccess);
        } else if (inst.contains("Alloc")) { // ALLOC INSTRUCTIONS
            auto allocInst = inst["Alloc"];
            std::string lhs = allocInst["lhs"]["name"];
            std::string lhsAccess = getAccessMode(lhs);

            std::string allocID = allocInst["id"]["name"];
            std::string allocAccess = getAccessMode(allocID);

            if (allocInst["num"].contains("CInt")) {
                int numElements = allocInst["num"]["CInt"];
                emit("  movq $" + std::to_string(numElements) + ", %r8");
                emit("  cmpq $0, %r8");
                emit("  jle .invalid_alloc_length");
                emit("  movq $1, %rdi");
                emit("  imulq %r8, %rdi");
                emit("  incq %rdi");
                emit("  call _cflat_alloc");
                emit("  movq $" + std::to_string(numElements) + ", %r8");
            } else {
                std::string numVarName = allocInst["num"]["Var"]["name"];
                std::string numAccess = getAccessMode(numVarName);

                emit("  cmpq $0, " + numAccess);
                emit("  jle .invalid_alloc_length");
                emit("  movq $1, %rdi");
                emit("  imulq " + numAccess + ", %rdi");
                emit("  incq %rdi");
                emit("  call _cflat_alloc");
                emit("  movq " + numAccess + ", %r8");
            }

            emit("  movq %r8, 0(%rax)");
            emit("  addq $8, %rax");
            emit("  movq %rax, " + lhsAccess);
        } else if (inst.contains("Store")) { // STORE INSTRUCTIONS
            auto storeInst = inst["Store"];
            std::string dst = storeInst["dst"]["name"];
            std::string dstAccess = getAccessMode(dst);

            std::string src;
            std::string srcAccess;
            
            if (storeInst["op"].contains("Var")) {
                src = storeInst["op"]["Var"]["name"];
                srcAccess = getAccessMode(src);

                // check if src is a global function pointer
                if (storeInst["op"]["Var"]["typ"].contains("Ptr") && storeInst["op"]["Var"]["typ"]["Ptr"].contains("Fn")) {
                    if (storeInst["op"]["Var"]["scope"].is_null() || storeInst["op"]["Var"]["scope"] == "global") {
                        src += "_";
                        srcAccess = src + "(%rip)";
                    }
                }
                emit("  movq " + srcAccess + ", %r8");
            } else if (storeInst["op"].contains("CInt")) {
                int constValue = storeInst["op"]["CInt"];
                emit("  movq $" + std::to_string(constValue) + ", %r8");
            }

            emit("  movq " + dstAccess + ", %r9");
            emit("  movq %r8, 0(%r9)");
        } else if (inst.contains("Gfp")) { // GFP INSTRUCTIONS
            auto gfpInst = inst["Gfp"];
            std::string lhs = gfpInst["lhs"]["name"];
            // std::cerr << lhs << std::endl;
            std::string lhsAccess = getAccessMode(lhs);

            std::string src = gfpInst["src"]["name"];
            // std::cerr << src << std::endl;
            std::string srcAccess = getAccessMode(src);

            std::string field = gfpInst["field"]["name"];
            // std::cerr << field << std::endl;
            std::string structName;

            if (gfpInst["src"]["typ"].is_object() && gfpInst["src"]["typ"].contains("Ptr")) {
                const auto& ptrType = gfpInst["src"]["typ"]["Ptr"];
                if (ptrType.is_object() && ptrType.contains("Struct")) {
                    structName = ptrType["Struct"];
                    // std::cerr << structName << std::endl;
                } else {
                    throw std::runtime_error("Invalid 'Ptr' type in 'src.typ' of Gfp isntruction");
                }
            } else {
                throw std::runtime_error("Invalid 'typ' in 'src' of Gfp instruction");
            }

            int fieldOffset = getFieldOffset(structName, field);

            if (fieldOffset == -1) {
                throw std::runtime_error("Invalid field offset for structure: " + field);
            }

            emit("  movq " + srcAccess + ", %r8");
            emit("  leaq " + std::to_string(fieldOffset) + "(%r8), %r9");
            emit("  movq %r9, " + lhsAccess);
        }
    }

    void processTerminalConditions(const json& term, const std::string & funcName) {
        if (term.contains("Ret")) { // RETURN INSTRUCTIONS
            auto retInst = term["Ret"];

            if (retInst.contains("Var")) {
                std::string varName = retInst["Var"]["name"];
                std::string varAccess = getAccessMode(varName);

                emit("  movq " + varAccess + ", %rax");
            } else if (retInst.contains("CInt")) {
                int constValue = retInst["CInt"];
                emit("  movq $" + std::to_string(constValue) + ", %rax");
            }

            emit("  jmp " + funcName + "_epilogue\n");
        } else if (term.contains("Jump")) { // JUMP INSTRUCTIONS
            std::string target = term["Jump"];
            emit("  jmp " + funcName + "_" + target + "\n");
        } else if (term.contains("Branch")) { // BRANCH INSTRUCTIONS
            auto branchInst = term["Branch"];
            std::string cond;
            std::string tt = branchInst["tt"];
            std::string ff = branchInst["ff"];

            if (branchInst["cond"].contains("CInt")) {
                cond = "$" + std::to_string(static_cast<int>(branchInst["cond"]["CInt"]));
                emit("  movq " + cond + ", %r8");
                emit("  cmpq $0, %r8");
            } else if (branchInst["cond"].contains("Var")) {
                std::string varName = branchInst["cond"]["Var"]["name"];
                cond = getAccessMode(varName);
                emit("  cmpq $0, " + cond);
            }

            emit("  jne " + funcName + "_" + tt);
            emit("  jmp " + funcName + "_" + ff + "\n");
        } else if (term.contains("CallDirect")) { // DIRECT CALL INSTRUCTIONS (TERMINAL)
            auto callDirectInst = term["CallDirect"];

            // std::cout << "Debug: CallDirect instruction: " << callDirectInst.dump(4) << std::endl;

            if (callDirectInst.contains("callee") && callDirectInst["callee"].is_string()) {
                std::string callee = callDirectInst["callee"];
                std::string lhs, next_bb;
                // *NOTE: lhs field can be null, this handles that condition
                if (callDirectInst.contains("lhs") && !callDirectInst["lhs"].is_null() && callDirectInst["lhs"].contains("name") && callDirectInst["lhs"]["name"].is_string()) {
                    lhs = callDirectInst["lhs"]["name"];
                }

                if (callDirectInst.contains("next_bb") && callDirectInst["next_bb"].is_string()) {
                    next_bb = callDirectInst["next_bb"];
                } else {
                    throw std::runtime_error("Malformed CallDirect instruction: next_bb is missing or not a string (null)");
                }

                // calculate if need to align stack
                if (!callDirectInst["args"].empty()) {
                    int numArgs = callDirectInst["args"].size();
                    bool needAlignment = (numArgs % 2 != 0);
                    if (needAlignment) {
                        emit("  subq $8, %rsp");
                    }

                    // push arguments onto the stack in reverse order (right-to-left evaluation)
                    for (auto it = callDirectInst["args"].rbegin(); it != callDirectInst["args"].rend(); ++it) {
                        const auto& arg = *it;
                        if (arg.contains("Var")) {
                            if (arg["Var"].contains("name") && arg["Var"]["name"].is_string()) {
                                std::string varName = arg["Var"]["name"];
                                std::string varAccess = getAccessMode(varName);
                                emit("  pushq " + varAccess);
                            } else {
                                throw std::runtime_error("Malformed argument: Var name is missing or not a string");
                            }
                        } else if (arg.contains("CInt")) {
                            int constValue = arg["CInt"];
                            emit("  pushq $" + std::to_string(constValue));
                        }
                    }

                    // call the function
                    emit("  call " + callee);

                    // move the result to lhs if lhs is not null
                    if (!lhs.empty()) {
                        emit("  movq %rax, " + std::to_string(localOffsets[lhs]) + "(%rbp)");
                    }

                    // adjust stack pointer to remove pushed arguments if any
                    int stackAdjustment = numArgs * 8;
                    if (needAlignment) {
                        stackAdjustment += 8;
                    }
                    emit("  addq $" + std::to_string(stackAdjustment) + ", %rsp");
                } else {
                    // call function without arguments
                    emit("  call " + callee);

                    // move result to lhs if lhs is not null
                    if (!lhs.empty()) {
                        emit("  movq %rax, " + std::to_string(localOffsets[lhs]) + "(%rbp)");
                    }
                }
                // jump to next basic block
                emit("  jmp " + funcName + "_" + next_bb + "\n");
            } else {
                throw std::runtime_error("Malformed CallDirect instruction: callee is missing or not a string");
            }
        } else if (term.contains("CallIndirect")) { // INDIRECT CALL INSTRUCTIONS (TERMINAL)
            auto callIndirectInst = term["CallIndirect"];
            if (callIndirectInst.contains("callee") && callIndirectInst["callee"].is_object()) {
                std::string callee;
                if (callIndirectInst["callee"].contains("name") && callIndirectInst["callee"]["name"].is_string()) {
                    callee = callIndirectInst["callee"]["name"];
                } else {
                    throw std::runtime_error("Malformed CallIndirect instruction: callee name is missing or not a string");
                }

                std::string lhs, next_bb;
                if (callIndirectInst.contains("lhs") && callIndirectInst["lhs"].is_object() && callIndirectInst["lhs"].contains("name") && callIndirectInst["lhs"]["name"].is_string()) {
                    lhs = callIndirectInst["lhs"]["name"];
                }

                if (callIndirectInst.contains("next_bb") && callIndirectInst["next_bb"].is_string()) {
                    next_bb = callIndirectInst["next_bb"];
                } else {
                    throw std::runtime_error("Malformed CallIndirect instruction: next_bb is missing or not a string");
                }

                if (!callIndirectInst["args"].empty()) {
                    int numArgs = callIndirectInst["args"].size();
                    bool needAlignment = (numArgs % 2 != 0);

                    if (needAlignment) {
                        emit("  subq $8, %rsp");
                    }

                    // push arguments onto stack in reverse order
                    for (auto it = callIndirectInst["args"].rbegin(); it != callIndirectInst["args"].rend(); ++it) {
                        const auto& arg = *it;
                        if (arg.contains("Var")) {
                            if (arg["Var"].contains("name") && arg["Var"]["name"].is_string()) {
                                std::string varName = arg["Var"]["name"];
                                std::string varAccess = getAccessMode(varName);
                                emit("  pushq " + varAccess);
                            } else {
                                throw std::runtime_error("Malformed argument: Var name is missing or not a string");
                            }
                        } else if (arg.contains("CInt")) {
                            int constValue = arg["CInt"];
                            emit("  pushq $" + std::to_string(constValue));
                        }
                    }

                    // call function indirectly
                    std::string calleeAccess = getAccessMode(callee);
                    emit("  call *" + calleeAccess);

                    // move result to lhs if lhs is not null
                    if (!lhs.empty()) {
                        emit("  movq %rax, " + std::to_string(localOffsets[lhs]) + "(%rbp)");
                    }

                    // adjust stack pointer to remove pushed arguments plus any alignment adjustment
                    int stackAdjustment = numArgs * 8;
                    if (needAlignment) {
                        stackAdjustment += 8;
                    }
                    emit("  addq $" + std::to_string(stackAdjustment) + ", %rsp");
                } else {
                    // call function indirectly without arguments
                    std::string calleeAccess = getAccessMode(callee);
                    emit("  call *" + calleeAccess);

                    // move result to lhs if not null
                    if (!lhs.empty()) {
                        emit("  movq %rax, " + std::to_string(localOffsets[lhs]) + "(%rbp)");
                    }
                }

                // jump to next basic block
                emit("  jmp " + funcName + "_" + next_bb + "\n");
            } else {
                throw std::runtime_error("Malformed CallIndirect instruction: callee is missing or not an object");
            }
        }
    }

    void generateAssembly() {
        emit(".data\n");

        for (const auto& global : lirJson["globals"]) {
            std::string name = global["name"];
            std::string underscoreName = name + "_";

            if (global["typ"].contains("Ptr") && global["typ"]["Ptr"].contains("Fn")) { // global function pointer
                emit(".globl " + underscoreName);
                emit(underscoreName + ": .quad \"" + name + "\"");
            } else { // global variable
                emit(".globl " + name);
                emit(name + ": .zero 8");
            }
            emit("\n");
        }

        // Emit data section
        emit("out_of_bounds_msg: .string \"out-of-bounds array access\"");
        emit("invalid_alloc_msg: .string \"invalid allocation amount\"\n");

        // Emit text section
        emit(".text\n");

        // Emit functions
        for (const auto& [funcName, funcDetails] : lirJson["functions"].items()) {
            emitFunction(funcName, funcDetails);
        }

        // Out-of-bounds and invalid allocation handlers
        emit(".out_of_bounds:");
        emit("  lea out_of_bounds_msg(%rip), %rdi");
        emit("  call _cflat_panic\n");

        emit(".invalid_alloc_length:");
        emit("  lea invalid_alloc_msg(%rip), %rdi");
        emit("  call _cflat_panic");
    }

    void emitFunction(const std::string& funcName, const json& funcDetails) {
        emit(".globl " + funcName);
        emit(funcName + ":");
        emit("  pushq %rbp");
        emit("  movq %rsp, %rbp");

        int stackSize = calculateStackSize(funcDetails);
        emit("  subq $" + std::to_string(stackSize) + ", %rsp");

        zeroInitializeLocals(funcDetails["locals"]);

        emit("  jmp " + funcName + "_entry\n");

        // adjust local offsets to include parameters
        adjustLocalOffsetsWithParams(funcDetails);

        // Emit function body
        for (const auto& [label, body] : funcDetails["body"].items()) {
            emit(funcName + "_" + label + ":");

            // Process instructions
            for (const auto& inst : body["insts"]) {
                processInstruction(inst);
            }

            // Process terminal conditions
            processTerminalConditions(body["term"], funcName);
        }

        // emit("\n");

        // Emit epilogue
        emit(funcName + "_epilogue:");
        emit("  movq %rbp, %rsp");
        emit("  popq %rbp");
        emit("  ret\n");
    }

    void adjustLocalOffsetsWithParams(const json& funcDetails) {
        int paramOffset = 16;
        for (const auto& param : funcDetails["params"]) {
            std::string paramName = param["name"];
            localOffsets[paramName] = paramOffset;
            paramOffset += 8;
        }

        int localOffset = -8;
        for (const auto& local : funcDetails["locals"]) {
            std::string localName = local["name"];
            localOffsets[localName] = localOffset;
            localOffset -= 8;
        }
    }

    void emitZeroInitialization() {
        int currentOffset = -8;
        while (currentOffset >= -stackSize) {
            emit("  movq $0, " + std::to_string(currentOffset) + "(%rbp)");
            currentOffset -= 8;
        }
    }

    void zeroInitializeLocals(const json& locals) { // for function locals
        int currentOffset = -8;
        for (const auto& local : locals) {
            emit("  movq $0, " + std::to_string(currentOffset) + "(%rbp)");
            currentOffset -= 8;
        }
    }

    void emit(const std::string& instruction) {
        instructions.push_back(instruction);
    }

    int calculateStackSize(const json& funcDetails) {
        int localCount = funcDetails["locals"].size();
        int stackSize = localCount * 8;

        if (stackSize % 16 != 0) {
            stackSize += 8;
        }

        return stackSize;
    }

    std::string getAccessMode(const std::string& name) {
        if (localOffsets.find(name) != localOffsets.end()) {
            return std::to_string(localOffsets[name]) + "(%rbp)";
        } else {
            return name + "(%rip)";
        }
    }

    std::string getOperandAccess(const json& operand) {
        if (operand.contains("CInt")) {
            return "$" + std::to_string(static_cast<int>(operand["CInt"]));
        } else if (operand.contains("Var")) {
            std::string varName = operand["Var"]["name"];
            return getAccessMode(varName);
        }
    }

    std::string getAssemblyCode() const {
        std::string assemblyCode;
        for (const auto& instr : instructions) {
            assemblyCode += instr + "\n";
        }
        return assemblyCode;
    }
};

std::string readFile(const json& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file");
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Example usage
int main(int argc, char *argv[]) {
    std::string lirProgram = readFile(argv[1]);

    LIRToX86CodeGenerator generator(lirProgram);
    std::string assemblyCode = generator.generate();

    std::cout << assemblyCode << std::endl;

    return 0;
}
