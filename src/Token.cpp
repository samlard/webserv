#include "../include/Token.hpp"

Token::Token() : _type(TOKEN_ERROR), _line(0), _column(0) {}

Token::Token(TokenType type, const std::string& value, size_t line, size_t column) :
    _type(type), _value(value), _line(line), _column(column) {}

Token::Token(const Token& other) :
    _type(other._type), _value(other._value), _line(other._line), _column(other._column) {}

Token& Token::operator=(const Token& other) {
    if (this != &other) {
        _type = other._type;
        _value = other._value;
        _line = other._line;
        _column = other._column;
    }
    return *this;
}

Token::~Token() {}

TokenType Token::getType() const { return _type; }
const std::string& Token::getValue() const { return _value; }
size_t Token::getLine() const { return _line; }
size_t Token::getColumn() const { return _column; }
