#pragma once
#include <memory>
#include <string>

// left right operation

struct ASTNode{
    virtual ~ASTNode() = default;
};

struct NumberNode : ASTNode{
    int value;
    NumberNode(int val) : value(val) {}
};


struct BinaryOpNode : ASTNode{
    std::string op;
    std::unique_ptr<ASTNode> left;
    std::unique_ptr<ASTNode> right;

    BinaryOpNode(const std::string& op,
                std::unique_ptr<ASTNode> left,
                std::unique_ptr<ASTNode> right)
            : op(op), left(std::move(left)), right(std::move(right)) {}
};

// 변수 이름을 받는 Node
struct VariableNode : ASTNode {
    std::string name;
    VariableNode(const std::string& name) : name(name) {}
};

// 
struct AssignNode: ASTNode{
    std::string name;
    std::unique_ptr<ASTNode> value;

    AssignNode(const std::string& name, std::unique_ptr<ASTNode> value)
        : name(name), value(std::move(value)) {}
};