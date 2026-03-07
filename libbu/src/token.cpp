#include "token.hpp"
#include <sstream>

Token::Token()
{
    type = TOKEN_EOF;
    lexeme = "";
    line = 0;
    column = 0;
}

Token::Token(TokenType t, const std::string &lex, int l, int c)
    : type(t), lexeme(lex), line(l), column(c) {}

std::string Token::toString() const
{
    std::ostringstream oss;
    oss << "Token(" << tokenTypeToString(type)
        << ", '" << lexeme << "', " << locationString() << ")";
    return oss.str();
}

std::string Token::locationString() const
{
    std::ostringstream oss;
    oss << "line " << line << ", column " << column;
    return oss.str();
}

const char *tokenTypeToString(TokenType type)
{
    switch (type)
    {
    case TOKEN_INT:
        return "INT";
    case TOKEN_FLOAT:
        return "FLOAT";
    case TOKEN_STRING:
        return "STRING";
    case TOKEN_IDENTIFIER:
        return "IDENTIFIER";
    case TOKEN_TRUE:
        return "TRUE";
    case TOKEN_FALSE:
        return "FALSE";
    case TOKEN_NIL:
        return "NIL";

    case TOKEN_VAR:
        return "VAR";
    case TOKEN_DEF:
        return "DEF";
    case TOKEN_IF:
        return "IF";
    case TOKEN_ELIF:
        return "ELIF";
    case TOKEN_ELSE:
        return "ELSE";
    case TOKEN_WHILE:
        return "WHILE";
    case TOKEN_FOR:
        return "FOR";
    case TOKEN_RETURN:
        return "RETURN";
    case TOKEN_BREAK:
        return "BREAK";
    case TOKEN_CONTINUE:
        return "CONTINUE";
    case TOKEN_DO:
        return "DO";
    case TOKEN_LOOP:
        return "LOOP";
    case TOKEN_SWITCH:
        return "SWITCH";
    case TOKEN_CASE:
        return "CASE";
    case TOKEN_DEFAULT:
        return "DEFAULT";

    case TOKEN_PLUS:
        return "PLUS";
    case TOKEN_MINUS:
        return "MINUS";
    case TOKEN_STAR:
        return "STAR";
    case TOKEN_SLASH:
        return "SLASH";
    case TOKEN_PERCENT:
        return "PERCENT";
    

    case TOKEN_EQUAL:
        return "EQUAL";
    case TOKEN_EQUAL_EQUAL:
        return "EQUAL_EQUAL";
    case TOKEN_BANG_EQUAL:
        return "BANG_EQUAL";
    case TOKEN_LESS:
        return "LESS";
    case TOKEN_LESS_EQUAL:
        return "LESS_EQUAL";
    case TOKEN_GREATER:
        return "GREATER";
    case TOKEN_GREATER_EQUAL:
        return "GREATER_EQUAL";

    case TOKEN_AND_AND:
        return "AND_AND";
    case TOKEN_OR_OR:
        return "OR_OR";
    case TOKEN_BANG:
        return "BANG";
    case TOKEN_COLON:
        return "COLON";
    case TOKEN_QUESTION:
        return "QUESTION";
    
    case TOKEN_PROCESS:
        return "PROCESS";
    case TOKEN_TYPE:
        return "TYPE";
    case TOKEN_PROC:
        return "PROC";
    case TOKEN_GET_ID:
        return "GET_ID";
    case TOKEN_FRAME:
        return "FRAME";
    case TOKEN_EXIT:
        return "EXIT";
    

    case TOKEN_LPAREN:
        return "LPAREN";
    case TOKEN_RPAREN:
        return "RPAREN";
    case TOKEN_LBRACE:
        return "LBRACE";
    case TOKEN_RBRACE:
        return "RBRACE";
    case TOKEN_COMMA:
        return "COMMA";
    case TOKEN_SEMICOLON:
        return "SEMICOLON";

    case TOKEN_PRINT:
        return "PRINT";
 

    case TOKEN_EOF:
        return "EOF";
    case TOKEN_ERROR:
        return "ERROR";
    case TOKEN_IMPORT:
        return "IMPORT";
    case TOKEN_USING:
        return "USING";
    case TOKEN_REQUIRE:
        return "REQUIRE";

    default:
        return "UNKNOWN";
    }
}
