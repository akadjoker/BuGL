#pragma once

#include <string>
#include <cstdint>

enum TokenType
{
    // Literals
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_STRING,
    TOKEN_IDENTIFIER,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_NIL,

    // Keywords
    TOKEN_VAR,
    TOKEN_DEF,
    TOKEN_IF,
    TOKEN_ELIF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_FOREACH,
    TOKEN_IN,
    TOKEN_RETURN,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_DO,
    TOKEN_LOOP,
    TOKEN_SWITCH,
    TOKEN_CASE,
    TOKEN_DEFAULT,
    


    // Operators - Arithmetic
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PERCENT,

    // Operators - Comparison
    TOKEN_EQUAL,
    TOKEN_EQUAL_EQUAL,
    TOKEN_BANG_EQUAL,
    TOKEN_LESS,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER,
    TOKEN_GREATER_EQUAL,

    // Operators - Logical
    TOKEN_AND_AND,
    TOKEN_OR_OR,
    TOKEN_BANG,

    // Delimiters
    TOKEN_LPAREN,  // (
    TOKEN_RPAREN,  // )
    TOKEN_LBRACE,  // {
    TOKEN_RBRACE,  // }
    TOKEN_COMMA,   // ,
    TOKEN_SEMICOLON, // ;
    TOKEN_AT,              // @

 // Compound assignment
    TOKEN_PLUS_EQUAL,      // +=
    TOKEN_MINUS_EQUAL,     // -=
    TOKEN_STAR_EQUAL,      // *=
    TOKEN_SLASH_EQUAL,     // /=
    TOKEN_PERCENT_EQUAL,   // %=
    
    // Increment/Decrement
    TOKEN_PLUS_PLUS,       // ++
    TOKEN_MINUS_MINUS,     // --

    TOKEN_COLON,           // :
    TOKEN_QUESTION,        // ?
    TOKEN_RBRACKET,        //[
    TOKEN_LBRACKET,        //]


    TOKEN_DOT, //.
    TOKEN_LABEL,
    TOKEN_GOTO,
    TOKEN_GOSUB,


    // Built-ins
    TOKEN_PRINT,
 

    TOKEN_PROCESS,
    TOKEN_TYPE,
    TOKEN_FRAME,
    TOKEN_EXIT,
    TOKEN_LEN,
    TOKEN_FREE,
    TOKEN_PROC,
    TOKEN_GET_ID,

    TOKEN_STRUCT,
    TOKEN_ENUM,
    TOKEN_CLASS,
    TOKEN_SELF,
    TOKEN_THIS,
    TOKEN_SUPER,


    TOKEN_AMPERSAND,      // &
    TOKEN_PIPE,           // |
    TOKEN_CARET,          // ^
    TOKEN_TILDE,          // ~
    TOKEN_LEFT_SHIFT,     // <<
    TOKEN_RIGHT_SHIFT,    // >>

    TOKEN_INCLUDE,
    TOKEN_IMPORT,
    TOKEN_USING,
    TOKEN_REQUIRE,

    // Exceptions
    TOKEN_TRY,
    TOKEN_CATCH,
    TOKEN_FINALLY,
    TOKEN_THROW,

    //MATH
    TOKEN_SIN,
    TOKEN_COS,
    TOKEN_SQRT,
    TOKEN_ABS,
    TOKEN_FLOOR,
    TOKEN_CEIL,
    TOKEN_DEG,
    TOKEN_RAD,
    TOKEN_TAN,
    TOKEN_ASIN,
    TOKEN_ACOS,
    TOKEN_ATAN,
    TOKEN_ATAN2,
    TOKEN_POW,
    TOKEN_LOG,
    TOKEN_EXP,

    //array
    TOKEN_PUSH,

    //timer
    TOKEN_CLOCK,
    TOKEN_TIME,
    
    

    // Special
    TOKEN_EOF,
    TOKEN_ERROR,
    TOKEN_COUNT
};

struct Token
{
    TokenType type;
    std::string lexeme;

    int line;   // Linha (1-indexed)
    int column; // Coluna (1-indexed)

    Token();

    Token(TokenType t, const std::string &lex, int l, int c);

    std::string toString() const;
    std::string locationString() const; // "line 5, column 12"
};

const char *tokenTypeToString(TokenType type);
