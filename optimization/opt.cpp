#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include "json.hpp"
#include <map>
#include <string>
#include <cctype>
#include <algorithm>

using json = nlohmann::json;

void initFunc(std::map<std::string, std::string> &init, json &jsonData) {
    for (auto &var : jsonData["functions"]["test"]["params"])
    {
        init[var["name"]] = "Top";
    }
    for (auto &var : jsonData["functions"]["test"]["locals"])
    {
        init[var["name"]] = "0";
    }
}

void botFunc(std::map<std::string, std::string> &Bottom, json &jsonData) {
    for (auto &var : jsonData["functions"]["test"]["params"])
    {
        Bottom[var["name"]] = "Bottom";
    }
    for (auto &var : jsonData["functions"]["test"]["locals"])
    {
        Bottom[var["name"]] = "Bottom";
    }
}

std::map<std::string, std::string> join(std::map<std::string, std::string> &IN, std::map<std::string, std::string> &OUT) {
    std::map<std::string, std::string> store;
    for (auto &var : IN)
    {
        std::string IN_val = var.second;
        std::string name = var.first;
        std::string OUT_val = OUT[name];
        // same values
        if ((IN_val != "Bottom" && IN_val != "Top") && (OUT_val != "Bottom" && OUT_val != "Top") && IN_val == OUT_val)
        {
            store[name] = IN_val;
        }
        // diff values
        else if ((IN_val != "Bottom" && IN_val != "Top") && (OUT_val != "Bottom" && OUT_val != "Top") && IN_val != OUT_val)
        {
            store[name] = "Top";
        }
        // either val is Top
        else if (IN_val == "Top" || OUT_val == "Top")
        {
            store[name] = "Top";
        }
        // in val is Bottom
        else if (IN_val == "Bottom")
        {
            store[name] = OUT_val;
        }
        else
        {
            store[name] = IN_val;
        }
    }
    return store;
}

std::map<std::string, std::map<std::string, std::string>> varsInitFunc(json &jsonData, std::map<std::string, std::string> &init, std::map<std::string, std::string> &Bottom) {
    std::string lbl;
    std::map<std::string, std::map<std::string, std::string>> bbData;
    // iterate through bbs
    for (auto &bb : jsonData["functions"]["test"]["body"])
    {
        lbl = bb["id"];
        // check if bb is entry
        if (lbl == "entry")
        {
            bbData["IN_" + lbl] = init;
            bbData["OUT_" + lbl] = Bottom;
        }
        else
        {
            bbData["IN_" + lbl] = Bottom;
            bbData["IN_" + lbl] = Bottom;
        }
    }
    return bbData;
}

std::string abstractAdd(const std::string& x, const std::string& y) {
    if (x == "Bottom" || y == "Bottom") return "Bottom";
    else if (x == "Top" || y == "Top") return "Top";

    try {
        int result = std::stoi(x) + std::stoi(y);
        return std::to_string(result);
    } catch (const std::invalid_argument&) {
        return x + " + " + y;
    }
}

std::string abstractSub(const std::string& x, const std::string& y) {
    if (x == "Bottom" || y == "Bottom") return "Bottom";
    else if (x == "Top" || y == "Top")  return "Top";

    try {
        int result = std::stoi(x) - std::stoi(y);
        return std::to_string(result);
    } catch (const std::invalid_argument&) {
        return x + " - " + y;
    }
}

std::string abstractMul(const std::string& x, const std::string& y) {
    if (x == "Bottom" || y == "Bottom") {
        return "Bottom";
    } else if (x == "0" || y == "0") {
        return "0";
    } else if (x == "Top" || y == "Top") {
        return "Top";
    } else {
        try {
            int result = std::stoi(x) * std::stoi(y);
            return std::to_string(result);
        } catch (const std::invalid_argument&) {
            return x + " * " + y;
        }
    }
}

std::string abstractDiv(const std::string& x, const std::string& y) {
    if (x == "Bottom" || y == "Bottom") {
        return "Bottom";
    } else if (x == "0" && y != "0") {
        return "0";
    } else if (y == "0") {
        return "Bottom";
    } else if (x == "Top" || y == "Top") {
        return "Top";
    } else {
        try {
            int result = std::stoi(x) / std::stoi(y);
            return std::to_string(result);
        } catch (const std::invalid_argument&) {
            return x + " / " + y;
        }
    }
}

std::string abstractCmp(const std::string& x, const std::string& y, const std::string& rop) {
    if (x == "Top" || y == "Top") return "Top";
    else if (x == "Bottom" || y == "Bottom") return "Bottom";

    try {
        int int_x = std::stoi(x);
        int int_y = std::stoi(y);
        if (rop == "Eq") {
            return (int_x == int_y) ? "1" : "0";
        } else if (rop == "Neq") {
            return (int_x != int_y) ? "1" : "0";
        } else if (rop == "Less") {
            return (int_x < int_y) ? "1" : "0";
        } else if (rop == "LessEq") {
            return (int_x <= int_y) ? "1" : "0";
        } else if (rop == "Greater") {
            return (int_x > int_y) ? "1" : "0";
        } else if (rop == "GreaterEq") {
            return (int_x >= int_y) ? "1" : "0";
        }
    } catch (const std::invalid_argument&) {
        return x + " " + rop + " " + y;
    }
}

void updateStore(const json& inst, std::map<std::string, std::string>& store, std::queue<std::pair<std::string, std::string>>& worklist, const std::string& lbl) {
    auto getValue = [&](const json& operand) -> std::string {
        if (operand.contains("Var")) {
            std::string varName = operand["Var"]["name"];
            // debug print
            // std::cout << "Getting value for Var: " << varName << " from store\n";
            return store.count(varName) ? store[varName] : varName;
        } else if (operand.contains("CInt")) {
            int value = operand["CInt"];
            // debug print
            // std::cout << "Getting value for CInt: " << value << "\n";
            return std::to_string(value);
        }
        return "Bottom";
    };

    if (inst.contains("Arith")) {
        std::string lhs = inst["Arith"]["lhs"]["name"];
        std::string aop = inst["Arith"]["aop"];
        std::string op1 = getValue(inst["Arith"]["op1"]);
        std::string op2 = getValue(inst["Arith"]["op2"]);

        // debug print
        // std::cout << "Processing Arith operation: " << aop << " with op1: " << op1 << " and op2: " << op2 << "\n";

        if (aop == "Add") {
            store[lhs] = abstractAdd(op1, op2);
        } else if (aop == "Subtract") {
            store[lhs] = abstractSub(op1, op2);
        } else if (aop == "Multiply") {
            store[lhs] = abstractMul(op1, op2);
        } else if (aop == "Divide") {
            store[lhs] = abstractDiv(op1, op2);
        }

        // debug print
        // std::cout << "Stored " << lhs << " as " << store[lhs] << " in store\n";

    } else if (inst.contains("Copy")) {
        std::string lhs = inst["Copy"]["lhs"]["name"];
        std::string rhs = getValue(inst["Copy"]["op"]);

        store[lhs] = rhs;

        // debug print
        // std::cout << "Copy operation: stored " << lhs << " as " << store[lhs] << " in store\n";
    } else if (inst.contains("Cmp")) {
        std::string lhs = inst["Cmp"]["lhs"]["name"];
        std::string rop = inst["Cmp"]["rop"];
        std::string op1 = getValue(inst["Cmp"]["op1"]);
        std::string op2 = getValue(inst["Cmp"]["op2"]);

        // debug print
        // std::cout << "Processing Cmp operation: " << rop << " with op1: " << op1 << " and op2: " << op2 << "\n";

        store[lhs] = abstractCmp(op1, op2, rop);

        // debug print
        // std::cout << "Stored " << lhs << " as " << store[lhs] << " in store\n";
    }
}

bool isConstant(const std::string& value) {
    try {
        std::stoi(value);
        return true;
    } catch (const std::invalid_argument&) {
        return false;
    }
}

std::string toLowerCase(const std::string& str) {
    std::string lowerStr = str; // Create a copy of the input string
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return lowerStr;
}

std::string convertOperator(const std::string& rop) {
    // map for conversion
    std::map<std::string, std::string> conversionMap = {
        {"lesseq", "lte"},
        {"less", "lt"},
        {"greatereq", "gte"},
        {"greater", "gt"},
        {"eq", "eq"},
        {"neq", "neq"}
    };

    // convert op to lowercase
    std::string lowerOp = toLowerCase(rop);

    // lookup operator in map, return short form
    if (conversionMap.find(lowerOp) != conversionMap.end()) {
        return conversionMap[lowerOp];
    } else {
        return lowerOp;
    }
}


std::string translateLir(const json& inst, const std::map<std::string, std::string>& store) {
    auto getValue = [&](const json& operand) -> std::string {
        if (operand.contains("Var")) {
            std::string varName = operand["Var"]["name"];
            // debug print
            // std::cout << "Translating value for Var: " << varName << "\n";
            return (store.count(varName) && store.at(varName) != "Top" && store.at(varName) != "Bottom") ? store.at(varName) : varName;
        } else if (operand.contains("CInt")) {
            int value = operand["CInt"];
            // debug print
            // std::cout << "Translating value for CInt: " << value << "\n";
            return std::to_string(value);
        }
        return "";
    };


    if (inst.contains("Arith")) {
        std::string lhs = inst["Arith"]["lhs"]["name"];
        std::string aop = inst["Arith"]["aop"];
        std::string op1 = getValue(inst["Arith"]["op1"]);
        std::string op2 = getValue(inst["Arith"]["op2"]);

        // debug print
        // std::cout << "Processing Arith operation: " << aop << " for LHS: " << lhs << " with op1: " << op1 << " and op2: " << op2 << "\n";

        // Handle Add operation
        if (aop == "Add") {
            // Directly return a Copy if one operand is zero
            if (op1 == "0") {
                return "Copy(" + lhs + ", " + op2 + ")";
            } else if (op2 == "0") {
                return "Copy(" + lhs + ", " + op1 + ")";
            } else {
                // If both operands are constants, compute the result
                try {
                    int result = std::stoi(op1) + std::stoi(op2);
                    return "Copy(" + lhs + ", " + std::to_string(result) + ")";
                } catch (const std::invalid_argument&) {
                    return "Arith(" + lhs + ", add, " + op1 + ", " + op2 + ")";
                }
            }
        }

        // Handle Subtract operation
        else if (aop == "Subtract") {
            // Directly return a Copy if subtracting zero or both operands are equal
            if (op2 == "0") {
                return "Copy(" + lhs + ", " + op1 + ")";
            } else if (op1 == op2) {
                if (isConstant(op1) && isConstant(op2)) {
                    int result = std::stoi(op1) - std::stoi(op2);
                    return "Copy(" + lhs + ", " + std::to_string(result) + ")";
                }
                return "Arith(" + lhs + ", sub, " + op1 + ", " + op2 + ")"; 
            } else {
                // If both operands are constants, compute the result
                try {
                    int result = std::stoi(op1) - std::stoi(op2);
                    return "Copy(" + lhs + ", " + std::to_string(result) + ")";
                } catch (const std::invalid_argument&) {
                    return "Arith(" + lhs + ", sub, " + op1 + ", " + op2 + ")";
                }
            }
        }

        // Handle Multiple operation
        else if (aop == "Multiply") {
            if (op1 == "1" && op2 != "1") {
                return "Copy(" + lhs + ", " + op2 + ")";
            } else if (op2 == "1" && op1 != "1") {
                return "Copy(" + lhs + ", " + op1 + ")";
            } else if ((op1 == "0" || op2 == "0") && store.at(lhs) != "Bottom") {
                return "Copy(" + lhs + ", 0)"; 
            } else {
                try {
                    int result = std::stoi(op1) * std::stoi(op2);
                    return "Copy(" + lhs + ", " + std::to_string(result) + ")";
                } catch (const std::invalid_argument&) {
                    return "Arith(" + lhs + ", mul, " + op1 + ", " + op2 + ")";
                }
            }
        }

        // Handle Division operation
        else if (aop == "Divide") {
            if (op2 == "1") {
                return "Copy(" + lhs + ", " + op1 + ")";
            } else if (op2 == "0" || (store.count(op2) && store.at(op2) == "0")) {
                return "Arith(" + lhs + ", div, " + op1 + ", " + op2 + ")";
            } else if (op1 == "0" && store.at(lhs) != "Bottom") {
                return "Copy(" + lhs + ", " + op1 + ")";
            } else {
                try {
                    int result = std::stoi(op1) / std::stoi(op2);
                    return "Copy(" + lhs + ", " + std::to_string(result) + ")";
                } catch (const std::invalid_argument&) {
                    return "Arith(" + lhs + ", div, " + op1 + ", " + op2 + ")";
                } catch (const std::out_of_range&) {
                    return "Arith(" + lhs + ", div, " + op1 + ", " + op2 + ")";
                }
            }
        }
    }

    // Handle Copy operation
    else if (inst.contains("Copy")) {
        std::string lhs = inst["Copy"]["lhs"]["name"];
        std::string rhs = getValue(inst["Copy"]["op"]);
        return "Copy(" + lhs + ", " + rhs + ")";
    }

    else if (inst.contains("Cmp")) {
        std::string lhs = inst["Cmp"]["lhs"]["name"];
        std::string rop = inst["Cmp"]["rop"];
        std::string op1 = getValue(inst["Cmp"]["op1"]);
        std::string op2 = getValue(inst["Cmp"]["op2"]);
        
        // debug print
        // std::cout << "Processing Cmp operation: " << rop << " for LHS: " << lhs << " with op1: " << op1 << " and op2: " << op2 << "\n";
        int result = 0;
        try {
            int intOp1 = std::stoi(op1);
            int intOp2 = std::stoi(op2);

            if (rop == "Eq") {
                result = (intOp1 == intOp2) ? 1 : 0;
            } else if (rop == "Neq") {
                result = (intOp1 != intOp2) ? 1 : 0;
            } else if (rop == "Less") {
                result = (intOp1 < intOp2) ? 1 : 0;
            } else if (rop == "LessEq") {
                result = (intOp1 <= intOp2) ? 1 : 0;
            } else if (rop == "Greater") {
                result = (intOp1 > intOp2) ? 1 : 0;
            } else if (rop == "GreaterEq") {
                result = (intOp1 >= intOp2) ? 1 : 0;
            }
        } catch (const std::invalid_argument&) {
            return "Cmp(" + lhs + ", " + convertOperator(rop) + ", " + op1 + ", " + op2 + ")";
        }

        // return simplified Copy operation if possible
        return "Copy(" + lhs + ", " + std::to_string(result) + ")";
    }

    // Handle Ret operation
    else if (inst.contains("Ret")) {
        std::string retValue = getValue(inst["Ret"]);
        return "Ret(" + retValue + ")";
    }

    return "";
}


int main(int argc, char *argv[]) {
    // read in json data
    std::ifstream file(argv[1]);
    json jsonData;
    file >> jsonData;
    file.close();

    // init
    std::map<std::string, std::string> init;
    initFunc(init, jsonData);

    // Bottom
    std::map<std::string, std::string> Bottom;
    botFunc(Bottom, jsonData);

    // store
    std::map<std::string, std::string> store;

    // initialize IN_bb/OUT_bb data structure
    std::map<std::string, std::map<std::string, std::string>> bbData;

    // iterate through bbs
    bbData = varsInitFunc(jsonData, init, Bottom);

    // initialize worklist with entry
    std::queue<std::pair<std::string, std::string>> worklist;
    worklist.push(std::make_pair("entry", ""));

    // DFA
    std::string lbl;
    std::string pred;

    // output
    std::string params = "";
    std::string name, type;
    for (auto &var : jsonData["functions"]["test"]["params"])
    {
        if (params != "")
        {
            params += ", ";
        }
        name = var["name"];
        type = var["typ"];
        params += name + ":" + type;
    }
    std::string rettype = jsonData["functions"]["test"]["ret_ty"];
    std::cout << "Function test(" << params << ") -> " << rettype << " {" << std::endl;
    std::cout << "  Locals" << std::endl;
    for (auto &var : jsonData["functions"]["test"]["locals"])
    {
        name = var["name"];
        type = var["typ"];
        std::cout << "    " << name << " : " << type << std::endl;
    }

    // dfa/optimization
    while (!worklist.empty())
    {
        lbl = worklist.front().first;
        pred = worklist.front().second;
        worklist.pop();

        // check if there is a pred
        if (pred == "")
        {
            // join with out_lbl
            bbData["IN_" + lbl] = join(bbData["IN_" + lbl], bbData["OUT_" + lbl]);
        }
        else
        {
            // join with out_pred
            bbData["IN_" + lbl] = join(bbData["IN_" + lbl], bbData["OUT_" + pred]);
        }

        // copy in_lbl into store
        store = bbData["IN_" + lbl];

        std::cout << std::endl;

        std::cout << "  " << lbl << ":" << std::endl;

        // iterate through instructions
        for (auto &inst : jsonData["functions"]["test"]["body"][lbl]["insts"])
        {
            // update store
            updateStore(inst, store, worklist, lbl);
            std::cout << "    " << translateLir(inst, store) << std::endl;
        }
        // terminal logic
        if (store != bbData["OUT_" + lbl])
        {
            bbData["OUT_" + lbl] = store;
        }
        updateStore(jsonData["functions"]["test"]["body"][lbl]["term"], store, worklist, lbl);
        std::cout << "    " << translateLir(jsonData["functions"]["test"]["body"][lbl]["term"], store) << std::endl;
    }

    std::cout << "}\n" << std::endl;

    return 0;
}