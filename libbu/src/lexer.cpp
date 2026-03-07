#include "token.hpp"
#include "lexer.hpp"
#include "utf8_utils.h"
#include <cctype>
#include <cstdio>

Lexer::Lexer(const std::string &src)
    : source(src),
      start(0),
      current(0),
      line(1),
      column(1),
      tokenColumn(1),
      hasPendingError(false),
      pendingErrorMessage(""),
      pendingErrorLine(0),
      pendingErrorColumn(0)
{
    initKeywords();
}

Lexer::Lexer(const char *src, size_t len)
    : source(src, len), start(0), current(0), line(1),
      column(1),
      tokenColumn(1),
      hasPendingError(false),
      pendingErrorMessage(""),
      pendingErrorLine(0),
      pendingErrorColumn(0)
{
    initKeywords();
}

void Lexer::setPendingError(const std::string &message)
{
    if (!hasPendingError)
    {
        hasPendingError = true;
        pendingErrorMessage = message;
        pendingErrorLine = line;
        pendingErrorColumn = column;
    }
}

void Lexer::initKeywords()
{
    keywords = {
        {"var", TOKEN_VAR},
        {"def", TOKEN_DEF},
        {"if", TOKEN_IF},
        {"elif", TOKEN_ELIF},
        {"else", TOKEN_ELSE},
        {"while", TOKEN_WHILE},
        {"for", TOKEN_FOR},
        {"foreach", TOKEN_FOREACH},
        {"in", TOKEN_IN},
        {"return", TOKEN_RETURN},
        {"break", TOKEN_BREAK},
        {"continue", TOKEN_CONTINUE},
        {"do", TOKEN_DO},
        {"loop", TOKEN_LOOP},
        {"switch", TOKEN_SWITCH},
        {"case", TOKEN_CASE},
        {"default", TOKEN_DEFAULT},
        {"true", TOKEN_TRUE},
        {"false", TOKEN_FALSE},
        {"nil", TOKEN_NIL},
        {"print", TOKEN_PRINT},
        {"process", TOKEN_PROCESS},
        {"type", TOKEN_TYPE},
        {"frame", TOKEN_FRAME},
        {"len", TOKEN_LEN},
        {"free", TOKEN_FREE},
        {"proc", TOKEN_PROC},
        {"get_id", TOKEN_GET_ID},
        {"exit", TOKEN_EXIT},
        {"label", TOKEN_LABEL},
        {"goto", TOKEN_GOTO},
        {"gosub", TOKEN_GOSUB},
        {"struct", TOKEN_STRUCT},
        {"class", TOKEN_CLASS},
        {"self", TOKEN_SELF},
        {"super", TOKEN_SUPER},
        {"include", TOKEN_INCLUDE},
        {"import", TOKEN_IMPORT},
        {"using", TOKEN_USING},
        {"require", TOKEN_REQUIRE},
        {"try", TOKEN_TRY},
        {"catch", TOKEN_CATCH},
        {"finally", TOKEN_FINALLY},
        {"throw", TOKEN_THROW},

        {"sin", TOKEN_SIN},
        {"cos", TOKEN_COS},
        {"asin", TOKEN_ASIN},
        {"acos", TOKEN_ACOS},
        {"atan", TOKEN_ATAN},
        {"atan2", TOKEN_ATAN2},
        {"sqrt", TOKEN_SQRT},
        {"pow", TOKEN_POW},
        {"log", TOKEN_LOG},
        {"abs", TOKEN_ABS},
        {"floor", TOKEN_FLOOR},
        {"ceil", TOKEN_CEIL},
        {"deg", TOKEN_DEG},
        {"rad", TOKEN_RAD},
        {"tan", TOKEN_TAN},
        {"exp", TOKEN_EXP},

 
        {"clock", TOKEN_CLOCK},


        


        

    };
}

void Lexer::reset()
{
    start = 0;
    current = 0;
    line = 1;
    column = 1;
    tokenColumn = 1;
}

bool Lexer::isAtEnd() const
{
    return current >= source.length();
}

char Lexer::advance()
{
    if (isAtEnd())
        return '\0';

    char c = source[current++];

    if (c == '\n')
    {
        line++;
        column = 1;
    }
    else
    {
        column++;
    }

    return c;
}

char Lexer::peek() const
{
    if (isAtEnd())
        return '\0';
    return source[current];
}

char Lexer::peekNext() const
{
    if (current + 1 >= source.length())
        return '\0';
    return source[current + 1];
}

bool Lexer::match(char expected)
{
    if (isAtEnd())
        return false;
    if (source[current] != expected)
        return false;
    advance();
    return true;
}

void Lexer::skipWhitespace()
{
    while (!isAtEnd())
    {
        char c = peek();

        switch (c)
        {
        case ' ':
        case '\r':
        case '\t':
            advance();
            break;

        case '\n':
            advance();
            break;

        case '/':
            if (peekNext() == '/')
            {
                // Comentário linha
                advance();
                advance();
                while (peek() != '\n' && !isAtEnd())
                {
                    advance();
                }
            }
            else if (peekNext() == '*')
            {
                advance(); // /
                advance(); // *

                bool closed = false;
                size_t commentStart = current;
                const size_t MAX_COMMENT_LENGTH = 100000;

                while (!isAtEnd())
                {
                    if (current - commentStart > MAX_COMMENT_LENGTH)
                    {
                        setPendingError("Comment too long (max 100k chars)");
                        while (!isAtEnd() && !(peek() == '*' && peekNext() == '/'))
                        {
                            advance();
                        }
                        if (peek() == '*')
                        {
                            advance();
                            advance();
                        }
                        return; // Sai para reportar erro
                    }

                    if (peek() == '*' && peekNext() == '/')
                    {
                        advance(); // *
                        advance(); // /
                        closed = true;
                        break;
                    }

                    advance();
                }

                if (!closed && isAtEnd())
                {
                    setPendingError("Unterminated block comment");
                    //  Não há mais nada a fazer, já estamos no fim
                }
            }
            else
            {
                return; // É operador /, não comentário
            }
            break;

        default:
            return;
        }
    }
}

Token Lexer::makeToken(TokenType type, const std::string &lexeme)
{
    return Token(type, lexeme, line, tokenColumn);
}

Token Lexer::errorToken(const std::string &message)
{
    return Token(TOKEN_ERROR, message, line, tokenColumn);
}

bool Lexer::isKeyword(const std::string &name)
{
    return keywords.find(name) != keywords.end();
}

Token Lexer::number()
{
    // Hex? Verifica no INÍCIO do número
    if (source[start] == '0' && (peek() == 'x' || peek() == 'X'))
    {
        advance(); // consome 'x'

        while (readHexDigit() != -1)
        {
            advance();
        }

        std::string numStr = source.substr(start, current - start);
        return makeToken(TOKEN_INT, numStr);
    }

    // Normal int/float
    while (isdigit(peek()))
    {
        advance();
    }

    TokenType type = TOKEN_INT;
    if (peek() == '.' && isdigit(peekNext()))
    {
        type = TOKEN_FLOAT;
        advance();
        while (isdigit(peek()))
        {
            advance();
        }
    }

    std::string numStr = source.substr(start, current - start);
    return makeToken(type, numStr);
}
 

Token Lexer::identifier()
{
    const size_t MAX_IDENTIFIER_LENGTH = 255;
    size_t startPos = current - 1;

    while (isalnum(peek()) || peek() == '_')
    {
        advance();

        if (current - startPos > MAX_IDENTIFIER_LENGTH)
        {
            return errorToken("Identifier too long (max 255 chars)");
        }
    }

    std::string text = source.substr(start, current - start);

    auto it = keywords.find(text);
    if (it != keywords.end())
    {
        return makeToken(it->second, text);
    }

 

    return makeToken(TOKEN_IDENTIFIER, text);
}


Token Lexer::string()
{
    const size_t MAX_STRING_LENGTH = 10000;
    size_t startPos = current;
    std::string value;

    while (peek() != '"' && !isAtEnd())
    {
        if (current - startPos > MAX_STRING_LENGTH)
        {
            return errorToken("String too long (max 10000 chars)");
        }

        char c = advance();

        if (c == '\\')
        {
            if (isAtEnd())
            {
                return errorToken("Unterminated string");
            }

            char next = advance();
            switch (next)
            {
            case 'n':
                value += '\n';
                break;
            case 't':
                value += '\t';
                break;
            case 'r':
                value += '\r';
                break;
            case '\\':
                value += '\\';
                break;
            case '"':
                value += '"';
                break;
            case '0':
                value += '\0';
                break;
            case 'a':
                value += '\a';
                break;
            case 'b':
                value += '\b';
                break;
            case 'f':
                value += '\f';
                break;
            case 'v':
                value += '\v';
                break;
            case 'e':
                value += '\33';
                break;

            // Hex escape: \xHH
            case 'x':
            {
                int hex1 = readHexDigit();
                if (hex1 == -1)
                    return errorToken("Invalid hex escape \\x");
                advance();

                int hex2 = readHexDigit();
                if (hex2 == -1)
                    return errorToken("Invalid hex escape \\x");
                advance();

                uint8_t byte = (hex1 << 4) | hex2;
                value += (char)byte;
                break;
            }

            // Unicode escape: \uHHHH
            case 'u':
            {
                int codepoint = 0;
                for (int i = 0; i < 4; i++)
                {
                    int hex = readHexDigit();
                    if (hex == -1)
                        return errorToken("Invalid unicode escape \\u");
                    advance();
                    codepoint = (codepoint << 4) | hex;
                }
 
                
                // Verifica range máximo
                if (codepoint > 0x10FFFF)
                {
                    return errorToken("Unicode codepoint out of range (max U+10FFFF)");
                }

                // Verifica surrogates (inválidos em UTF-8)
                if (codepoint >= 0xD800 && codepoint <= 0xDFFF)
                {
                    return errorToken("Invalid unicode surrogate pair");
                }

                // Verifica noncharacters
                if ((codepoint >= 0xFDD0 && codepoint <= 0xFDEF) ||
                    (codepoint & 0xFFFE) == 0xFFFE)
                {
                    return errorToken("Unicode noncharacter not allowed");
                }

                uint8_t bytes[4];
                int numBytes = Utf8Encode(codepoint, bytes);
                
                if (numBytes <= 0)
                {
                    return errorToken("Invalid unicode encoding");
                }

                for (int i = 0; i < numBytes; i++)
                {
                    value += (char)bytes[i];
                }
                break;
            }

            // Unicode escape: \UHHHHHHHH
            case 'U':
            {
                int codepoint = 0;
                for (int i = 0; i < 8; i++)
                {
                    int hex = readHexDigit();
                    if (hex == -1)
                        return errorToken("Invalid unicode escape \\U");
                    advance();
                    codepoint = (codepoint << 4) | hex;
                }

           
                
                // Verifica range máximo
                if (codepoint > 0x10FFFF)
                {
                    return errorToken("Unicode codepoint out of range (max U+10FFFF)");
                }

                // Verifica surrogates (inválidos em UTF-8)
                if (codepoint >= 0xD800 && codepoint <= 0xDFFF)
                {
                    return errorToken("Invalid unicode surrogate pair");
                }

                // Verifica noncharacters
                if ((codepoint >= 0xFDD0 && codepoint <= 0xFDEF) ||
                    (codepoint & 0xFFFE) == 0xFFFE)
                {
                    return errorToken("Unicode noncharacter not allowed");
                }

                uint8_t bytes[4];
                int numBytes = Utf8Encode(codepoint, bytes);
                
                if (numBytes <= 0)
                {
                    return errorToken("Invalid unicode encoding");
                }

                for (int i = 0; i < numBytes; i++)
                {
                    value += (char)bytes[i];
                }
                break;
            }

            default:
                value += '\\';
                value += next;
                break;
            }
        }
        else
        {
            value += c;
        }
    }

    if (isAtEnd())
    {
        return errorToken("Unterminated string");
    }

    advance(); // fecha "
    return makeToken(TOKEN_STRING, value);
}

Token Lexer::verbatimString()
{
    const size_t MAX_STRING_LENGTH = 10000;
    size_t startPos = current;
    std::string value;

    while (!isAtEnd())
    {
        if (current - startPos > MAX_STRING_LENGTH)
        {
            return errorToken("Verbatim string too long (max 10000 chars)");
        }

        char c = peek();

        if (c == '"')
        {
            advance();
            // Verifica se é uma aspa duplicada "" (escape de aspas em verbatim string)
            if (peek() == '"')
            {
                advance();
                value += '"'; // adiciona uma aspa literal
                continue;
            }
            // Fim da string verbatim
            return makeToken(TOKEN_STRING, value);
        }

        // Adiciona caractere literal (incluindo quebras de linha)
        value += advance();
    }

    return errorToken("Unterminated verbatim string");
}
// ============================================
// MAIN API: scanToken()
// ============================================
Token Lexer::scanToken()
{
    skipWhitespace();
    start = current;
    tokenColumn = column;

    if (isAtEnd())
    {
        return makeToken(TOKEN_EOF, "");
    }

    char c = advance();

    // Numbers
    if (isdigit(c))
    {
        return number();
    }

    // Identifiers and keywords
    if (isalpha(c) || c == '_')
    {
        return identifier();
    }

    switch (c)
    {
    // Single-char tokens
    case '(':
        return makeToken(TOKEN_LPAREN, "(");
    case ')':
        return makeToken(TOKEN_RPAREN, ")");
    case '{':
        return makeToken(TOKEN_LBRACE, "{");
    case '}':
        return makeToken(TOKEN_RBRACE, "}");
    case '[':
        return makeToken(TOKEN_LBRACKET, "[");
    case ']':
        return makeToken(TOKEN_RBRACKET, "]");
    case ',':
        return makeToken(TOKEN_COMMA, ",");
    case ';':
        return makeToken(TOKEN_SEMICOLON, ";");
    case ':':
        return makeToken(TOKEN_COLON, ":");
    case '?':
        return makeToken(TOKEN_QUESTION, "?");
    case '.':
        return makeToken(TOKEN_DOT, ".");
    
    case '@':
        // Verifica se é uma verbatim string @"..."
        if (peek() == '"')
        {
            advance(); // consome o "
            return verbatimString();
        }
        return makeToken(TOKEN_AT, "@");

    // Operators com compound assignment e increment/decrement
    case '+':
        if (match('+'))
            return makeToken(TOKEN_PLUS_PLUS, "++");
        if (match('='))
            return makeToken(TOKEN_PLUS_EQUAL, "+=");
        return makeToken(TOKEN_PLUS, "+");

    case '-':
        if (match('-'))
            return makeToken(TOKEN_MINUS_MINUS, "--");
        if (match('='))
            return makeToken(TOKEN_MINUS_EQUAL, "-=");
        return makeToken(TOKEN_MINUS, "-");

    case '*':
        if (match('='))
            return makeToken(TOKEN_STAR_EQUAL, "*=");
        return makeToken(TOKEN_STAR, "*");

    case '/':
        if (match('='))
            return makeToken(TOKEN_SLASH_EQUAL, "/=");
        return makeToken(TOKEN_SLASH, "/");

    case '%':
        if (match('='))
            return makeToken(TOKEN_PERCENT_EQUAL, "%=");
        return makeToken(TOKEN_PERCENT, "%");

    // Two-char tokens
    case '=':
        if (match('='))
        {
            return makeToken(TOKEN_EQUAL_EQUAL, "==");
        }
        return makeToken(TOKEN_EQUAL, "=");

    case '!':
        if (match('='))
        {
            return makeToken(TOKEN_BANG_EQUAL, "!=");
        }
        return makeToken(TOKEN_BANG, "!");

    case '&':
        if (match('&'))
            return makeToken(TOKEN_AND_AND, "&&");
        return makeToken(TOKEN_AMPERSAND, "&");

    case '|':
        if (match('|'))
            return makeToken(TOKEN_OR_OR, "||");
        return makeToken(TOKEN_PIPE, "|");

    case '^':
        return makeToken(TOKEN_CARET, "^");

    case '~':
        return makeToken(TOKEN_TILDE, "~");

    case '<':
        if (match('<'))
            return makeToken(TOKEN_LEFT_SHIFT, "<<");
        if (match('='))
            return makeToken(TOKEN_LESS_EQUAL, "<=");
        return makeToken(TOKEN_LESS, "<");

    case '>':
        if (match('>'))
            return makeToken(TOKEN_RIGHT_SHIFT, ">>");
        if (match('='))
            return makeToken(TOKEN_GREATER_EQUAL, ">=");
        return makeToken(TOKEN_GREATER, ">");

    // String literals
    case '"':
        return string();

    default:
        return errorToken("Unexpected character");
    }
}
Token Lexer::nextToken()
{

    if (hasPendingError)
    {
        Token errorTok(TOKEN_ERROR, pendingErrorMessage,
                       pendingErrorLine, pendingErrorColumn);
        hasPendingError = false; // Limpa erro
        return errorTok;
    }

    skipWhitespace();

    if (hasPendingError)
    {
        Token errorTok(TOKEN_ERROR, pendingErrorMessage,
                       pendingErrorLine, pendingErrorColumn);
        hasPendingError = false;
        return errorTok;
    }

    start = current;
    tokenColumn = column;

    return scanToken();
}

// ============================================
// BATCH API: For tools/debugging
// ============================================

std::vector<Token> Lexer::scanAll()
{
    std::vector<Token> tokens;
    tokens.reserve(256); // Pre-allocate for performance

    Token token;
    do
    {
        token = nextToken();
        tokens.push_back(token);

    } while (token.type != TOKEN_EOF && !hasPendingError);

    return tokens;
}

void Lexer::printTokens(const std::vector<Token> &toks) const
{
    for (const Token &token : toks)
    {
        const std::string text = token.toString();
        std::fputs(text.c_str(), stdout);
        std::fputc('\n', stdout);
    }
}

int Lexer::readHexDigit()
{

    char c = peek();
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1; // Inválido
}
