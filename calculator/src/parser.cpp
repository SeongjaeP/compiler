#include "parser.hpp"
#include <stdexcept>
#include <iostream>

Parser::Parser(Lexer& lexer) : lexer(lexer){
    currentToken = lexer.getNextToken();
}

void Parser::eat(TokenType type){
    if (currentToken.type == type){
        currentToken = lexer.getNextToken();
    } else{
        throw std::runtime_error("Unexpected token");
    }
}

std::unique_ptr<ASTNode> Parser::expression(){
    // ID 추가
    if (currentToken.type == TokenType::ID){
        std::string varName = currentToken.value;
        eat(TokenType::ID);

        if (currentToken.type == TokenType::ASSIGN) {
            eat(TokenType::ASSIGN);
            auto expr = expression();
            return std::make_unique<AssignNode>(varName, std::move(expr));
        } else {
            return std::make_unique<VariableNode>(varName);
        }
    }

    auto node = term();

    while (currentToken.type == TokenType::PLUS ||
            currentToken.type == TokenType::MINUS){
        std::string op = currentToken.value;
        eat(currentToken.type);
        node = std::make_unique<BinaryOpNode>(op, std::move(node), term());
    }
    return node;
}

std::unique_ptr<ASTNode> Parser::term(){
    auto node = factor();

    while (currentToken.type == TokenType::MUL ||
            currentToken.type == TokenType::DIV){
        std::string op = currentToken.value;
        eat(currentToken.type);
        node = std::make_unique<BinaryOpNode>(op, std::move(node), factor());
    }

    return node;
}


std::unique_ptr<ASTNode> Parser::factor(){
    std::cout << "Current token in factor(): " << currentToken.value << std::endl;
    if (currentToken.type == TokenType::ID){
        std::string name = currentToken.value;
        eat(TokenType::ID);
        return std::make_unique<VariableNode>(name);  // ← x, y, z 
    }
    
    if (currentToken.type == TokenType::MINUS){
        eat(TokenType::MINUS);
        return std::make_unique<BinaryOpNode>("-", std::make_unique<NumberNode>(0), factor());
    }
    if (currentToken.type == TokenType::NUMBER){
        int val = std::stoi(currentToken.value);
        eat(TokenType::NUMBER);
        return std::make_unique<NumberNode>(val);
    }
    if (currentToken.type == TokenType::LPAREN){
        eat(TokenType::LPAREN);
        auto node = expression();
        eat(TokenType::RPAREN);
        return node;
    }
    throw std::runtime_error("Invalid factor");
}

std::unique_ptr<ASTNode> Parser::parse() {
    return expression();
}