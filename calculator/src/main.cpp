#include <iostream>
#include "parser.hpp"

std::unordered_map<std::string, int> symbolTable;

int evaluate(const ASTNode* node){
    if (const auto* num = dynamic_cast<const NumberNode*> (node)){
        return num -> value;
    }

    
    if (const auto* bin = dynamic_cast<const BinaryOpNode*> (node)){
        int left = evaluate(bin -> left.get());
        int right = evaluate(bin -> right.get());

        if (bin -> op == "+") return left + right;
        if (bin -> op == "-") return left - right;
        if (bin -> op == "*") return left * right;
        if (bin -> op == "/") return left / right;
    }

    if (const auto* var = dynamic_cast<const VariableNode*>(node)){
        auto it = symbolTable.find(var->name);
        if (it == symbolTable.end()) throw std::runtime_error("Undefined variable:" + var->name);
        return it->second;
    }

    if (const auto* assign = dynamic_cast<const AssignNode*>(node)){
        int val = evaluate(assign -> value.get());
        symbolTable[assign->name] = val;
        return val;
    }
        throw std::runtime_error("Unknown AST node");
}

int main() {
    // 1. x = 3
    {
        std::string code1 = "x = 3";
        Lexer lexer1(code1);
        Parser parser1(lexer1);
        auto tree1 = parser1.parse();
        evaluate(tree1.get());  // 변수 x에 3 저장됨
    }

    // 2. x + 5
    {
        std::string code2 = "x + 5";
        Lexer lexer2(code2);
        Parser parser2(lexer2);
        auto tree2 = parser2.parse();
        int result = evaluate(tree2.get());
        std::cout << "x + 5 = " << result << std::endl;  // → 8 출력!
    }

    return 0;
}