#pragma once
#include "lexer.hpp"
#include "ast.hpp"
#include <memory>

class Parser{
public:
    Parser(Lexer& lexer);
    std::unique_ptr<ASTNode> parse();

private:
    Lexer& lexer;
    Token currentToken;

    void eat(TokenType type);

    std::unique_ptr<ASTNode> expression();
    std::unique_ptr<ASTNode> term();
    std::unique_ptr<ASTNode> factor();
};