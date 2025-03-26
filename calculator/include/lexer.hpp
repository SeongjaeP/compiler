#pragma once
#include "token.hpp"
#include <string>

class Lexer {
public:
    Lexer(const std::string& input);
    Token getNextToken();

private:
    std::string text;
    size_t pos;
    char current;

    void advance();
    std::string integer();
};
