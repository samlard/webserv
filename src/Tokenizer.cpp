#include "../include/Tokenizer.hpp"

Tokenizer::Tokenizer(const std::string& input) :
    _input(input), _position(0), _line(1), _column(1) {}

Tokenizer::Tokenizer(const Tokenizer& other) :
    _input(other._input), _position(other._position),
    _line(other._line), _column(other._column) {}

Tokenizer& Tokenizer::operator=(const Tokenizer& other) {
    if (this != &other) {
        _input = other._input;
        _position = other._position;
        _line = other._line;
        _column = other._column;
    }
    return *this;
}

Tokenizer::~Tokenizer() {}

char Tokenizer::peek() const {
    if (_position >= _input.length())
        return '\0';
    return _input[_position];
}

char Tokenizer::advance() {
    if (_position >= _input.length())
        return '\0';
    char c = _input[_position++];
    if (c == '\n') {
        _line++;
        _column = 1;
    } else {
        _column++;
    }
    return c;
}

void Tokenizer::skipWhitespace() {
    while (peek() == ' ' || peek() == '\t' || peek() == '\n' || peek() == '\r')
        advance();
}

void Tokenizer::skipComment() {
    if (peek() == '#') {
        while (peek() != '\n' && peek() != '\0')
            advance();
    }
}

bool Tokenizer::isWordChar(char c) const {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') || c == '_' || c == '-' ||
           c == '.' || c == '/' || c == ':';
}

bool Tokenizer::isDigit(char c) const {
    return c >= '0' && c <= '9';
}

Token Tokenizer::readWord() {
    size_t startLine = _line;
    size_t startColumn = _column;
    std::string value;
    
    while (isWordChar(peek())) {
        value += advance();
    }
    
    return Token(TOKEN_WORD, value, startLine, startColumn);
}

Token Tokenizer::readNumber() {
    size_t startLine = _line;
    size_t startColumn = _column;
    std::string value;
    
    while (isDigit(peek())) {
        value += advance();
    }
    
    return Token(TOKEN_NUMBER, value, startLine, startColumn);
}

Token Tokenizer::readString() {
    size_t startLine = _line;
    size_t startColumn = _column;
    std::string value;
    char quote = advance(); // consume opening quote
    
    while (peek() != quote && peek() != '\0' && peek() != '\n') {
        if (peek() == '\\') {
            advance(); // consume backslash
            if (peek() != '\0') {
                value += advance();
            }
        } else {
            value += advance();
        }
    }
    
    if (peek() == quote) {
        advance(); // consume closing quote
        return Token(TOKEN_STRING, value, startLine, startColumn);
    }
    
    return Token(TOKEN_ERROR, "Unterminated string", startLine, startColumn);
}

std::vector<Token> Tokenizer::tokenize() {
    std::vector<Token> tokens;
    
    while (peek() != '\0') {
        skipWhitespace();
        
        if (peek() == '\0')
            break;
            
        if (peek() == '#') {
            skipComment();
            continue;
        }
        
        size_t startLine = _line;
        size_t startColumn = _column;
        
        char c = peek();
        
        if (c == ';') {
            advance();
            tokens.push_back(Token(TOKEN_SEMICOLON, ";", startLine, startColumn));
        } else if (c == '{') {
            advance();
            tokens.push_back(Token(TOKEN_LBRACE, "{", startLine, startColumn));
        } else if (c == '}') {
            advance();
            tokens.push_back(Token(TOKEN_RBRACE, "}", startLine, startColumn));
        } else if (c == '"' || c == '\'') {
            tokens.push_back(readString());
        } else if (isDigit(c)) {
            tokens.push_back(readNumber());
        } else if (isWordChar(c)) {
            tokens.push_back(readWord());
        } else {
            advance();
            tokens.push_back(Token(TOKEN_ERROR, std::string(1, c), startLine, startColumn));
        }
    }
    
    tokens.push_back(Token(TOKEN_EOF, "", _line, _column));
    return tokens;
}
