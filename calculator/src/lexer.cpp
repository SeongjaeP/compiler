#include "lexer.hpp"
#include <cctype>
#include <stdexcept>

Lexer::Lexer(const std::string& input) : text(input), pos(0) {
    current = text[pos];
}

Token Lexer::getNextToken() {
    while (current != '\0') {
        if (std::isspace(current)) {
            advance();
            continue;
        }
        if (std::isdigit(current)) {
            return Token{TokenType::NUMBER, integer()};
        }
        if (current == '+') {
            advance();
            return Token{TokenType::PLUS, "+"};
        }
        if (current == '-') {
            advance();
            return Token{TokenType::MINUS, "-"};
        }
        if (current == '*') {
            advance();
            return Token{TokenType::MUL, "*"};
        }
        if (current == '/') {
            advance();
            return Token{TokenType::DIV, "/"};
        }
        if (current == '(') {
            advance();
            return Token{TokenType::LPAREN, "("};
        }
        if (current == ')') {
            advance();
            return Token{TokenType::RPAREN, ")"};
        }
        throw std::runtime_error("Invalid character");
    }
    return Token{TokenType::END, ""};
}

void Lexer::advance() {
    pos++;
    if (pos >= text.length()) {
        current = '\0';
    } else {
        current = text[pos];
    }
}

std::string Lexer::integer() {
    std::string result;
    while (std::isdigit(current)) {
        result += current;
        advance();
    }
    return result;
}
