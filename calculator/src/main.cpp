#include <iostream>
#include "parser.hpp"

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

        throw std::runtime_error("Unknown operation" + bin -> op);
    }
        throw std::runtime_error("Unknown AST node");
}

int main() {
    std::string code = "3 + 4 * 2";
    Lexer lexer(code);
    Parser parser(lexer);

    auto tree = parser.parse();

    int result = evaluate(tree.get());
    std::cout << "Result: " << result << std::endl;

    return 0;
}