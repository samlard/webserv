#ifndef TOKEN_HPP
#define TOKEN_HPP

#include <string>

enum TokenType {
    TOKEN_WORD,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_SEMICOLON,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_EOF,
    TOKEN_ERROR
};

class Token {
public:
    Token();
    Token(TokenType type, const std::string& value, size_t line, size_t column);
    Token(const Token& other);
    Token& operator=(const Token& other);
    ~Token();

    TokenType getType() const;
    const std::string& getValue() const;
    size_t getLine() const;
    size_t getColumn() const;

private:
    TokenType _type;
    std::string _value;
    size_t _line;
    size_t _column;
};

#endif
