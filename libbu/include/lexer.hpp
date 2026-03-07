#pragma once

#include "token.hpp"
#include <vector>
#include <unordered_map>
#include <string>



class Lexer
{
public:
    Lexer(const std::string &source);
    Lexer(const char* src, size_t len);

    Token scanToken();

    Token nextToken();

    std::vector<Token> scanAll();
    void printTokens(const std::vector<Token> &tokens) const;
    bool isKeyword(const std::string& name);
    void reset();

private:
    std::string source;

    size_t start;
    size_t current;
    int line;
    int column;
    int tokenColumn;

    bool hasPendingError;
    std::string pendingErrorMessage;
    int pendingErrorLine;
    int pendingErrorColumn;

    std::unordered_map<std::string, TokenType> keywords;

    // Helper methods
    bool isAtEnd() const;
    char advance();
    char peek() const;
    char peekNext() const;
    bool match(char expected);

    void setPendingError(const std::string &message);
    void skipWhitespace();

    int readHexDigit();
    Token makeToken(TokenType type, const std::string &lexeme);
    Token errorToken(const std::string &message);


    // Token scanners
    Token number();
    Token string();
    Token verbatimString();
    Token identifier();

    void initKeywords();
};