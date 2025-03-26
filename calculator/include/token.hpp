#pragma once
#include <string>

enum class TokenType{
    PLUS,
    NUMBER,
    MINUS,
    DIV,
    MUL,
    LPAREN,
    RPAREN,
    END
};

struct Token{
    TokenType type;
    std::string value;
};