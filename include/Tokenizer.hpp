#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include <string>
#include <vector>
#include "Token.hpp"

class Tokenizer {
public:
    Tokenizer(const std::string& input);
    Tokenizer(const Tokenizer& other);
    Tokenizer& operator=(const Tokenizer& other);
    ~Tokenizer();

    std::vector<Token> tokenize();

private:
    std::string _input;
    size_t _position;
    size_t _line;
    size_t _column;

    char peek() const;
    char advance();
    void skipWhitespace();
    void skipComment();
    Token readWord();
    Token readNumber();
    Token readString();
    bool isWordChar(char c) const;
    bool isDigit(char c) const;
};

#endif
