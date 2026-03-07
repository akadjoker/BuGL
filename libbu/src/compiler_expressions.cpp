#include "compiler.hpp"
#include "interpreter.hpp"
#include "value.hpp"
#include "opcode.hpp"

void Compiler::expression()
{
    parsePrecedence(PREC_ASSIGNMENT);
}

// ============================================
// PREFIX FUNCTIONS
// ============================================

void Compiler::lengthExpression(bool canAssign)
{
    consume(TOKEN_LPAREN, "Expect '(' after len");

    expression(); // empilha o valor (array, string, etc.)
    if (hadError)
        return;
    consume(TOKEN_RPAREN, "Expect ')' after expression");

    emitByte(OP_FUNC_LEN);
}

void Compiler::freeExpression(bool canAssign)
{
    consume(TOKEN_LPAREN, "Expect '(' after 'free'");

    expression();
    if (hadError)
        return;
    consume(TOKEN_RPAREN, "Expect ')' after expression");

    emitByte(OP_FREE);
}

// 1. Para funções tipo: sin(x)
void Compiler::mathUnary(bool canAssign)
{
    TokenType type = previous.type; // Guardamos qual foi (SIN, COS, etc)

    consume(TOKEN_LPAREN, "Expect '('");
    expression();
    if (hadError)
        return;
    consume(TOKEN_RPAREN, "Expect ')'");

    switch (type)
    {
    case TOKEN_SIN:
        emitByte(OP_SIN);
        break;
    case TOKEN_COS:
        emitByte(OP_COS);
        break;
    case TOKEN_TAN:
        emitByte(OP_TAN);
        break;
    case TOKEN_ASIN:
        emitByte(OP_ASIN);
        break;
    case TOKEN_ACOS:
        emitByte(OP_ACOS);
        break;
    case TOKEN_ATAN:
        emitByte(OP_ATAN);
        break;
    case TOKEN_SQRT:
        emitByte(OP_SQRT);
        break;
    case TOKEN_ABS:
        emitByte(OP_ABS);
        break;
    case TOKEN_FLOOR:
        emitByte(OP_FLOOR);
        break;
    case TOKEN_CEIL:
        emitByte(OP_CEIL);
        break;
    case TOKEN_DEG:
        emitByte(OP_DEG);
        break;
    case TOKEN_RAD:
        emitByte(OP_RAD);
        break;
    case TOKEN_LOG:
        emitByte(OP_LOG);
        break;
    case TOKEN_EXP:
        emitByte(OP_EXP);
        break;
    default:
        return; // Erro
    }
}

// 2. Para funções tipo: atan2(y, x) ou pow(base, exp)
void Compiler::mathBinary(bool canAssign)
{
    TokenType type = previous.type;

    consume(TOKEN_LPAREN, "Expect '('");
    expression(); // Arg 1
    if (hadError)
        return;
    consume(TOKEN_COMMA, "Expect ','");
    expression(); // Arg 2
    if (hadError)
        return;
    consume(TOKEN_RPAREN, "Expect ')'");

    switch (type)
    {
    case TOKEN_ATAN2:
        emitByte(OP_ATAN2);
        break;
    case TOKEN_POW:
        emitByte(OP_POW);
        break;
    default:
        return;
    }
}

void Compiler::expressionClock(bool canAssign)
{
    (void)canAssign;

    consume(TOKEN_LPAREN, "Expect '(' after clock");
    consume(TOKEN_RPAREN, "Expect ')' after '('");

    emitByte(OP_CLOCK);
}

void Compiler::typeExpression(bool canAssign)
{
    (void)canAssign;
    consume(TOKEN_IDENTIFIER, "Expect process name after 'type'");
    emitConstant(vm_->makeString(previous.lexeme.c_str()));
    emitByte(OP_TYPE);
}

void Compiler::procExpression(bool canAssign)
{
    (void)canAssign;
    consume(TOKEN_LPAREN, "Expect '(' after 'proc'");
    expression();
    if (hadError) return;
    consume(TOKEN_RPAREN, "Expect ')' after expression");
    emitByte(OP_PROC);
}

void Compiler::getIdExpression(bool canAssign)
{
    (void)canAssign;
    consume(TOKEN_LPAREN, "Expect '(' after 'get_id'");
    expression();
    if (hadError) return;
    consume(TOKEN_RPAREN, "Expect ')' after expression");
    emitByte(OP_GET_ID);
}

void Compiler::number(bool canAssign)
{
    (void)canAssign;
    const char *str = previous.lexeme.c_str();

    if (previous.type == TOKEN_INT)
    {
        errno = 0;
        char *endptr = nullptr;
        long long value;

        if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X'))
        {
            value = std::strtoll(str, &endptr, 16);
        }
        else
        {
            value = std::strtoll(str, &endptr, 10);
        }

        // Verifica overflow
        if (errno == ERANGE)
        {
            error("Integer literal out of range");
            emitConstant(vm_->makeInt(0)); // Valor default
            return;
        }

        // Verifica parse válido
        if (endptr == str)
        {
            error("Invalid integer literal");
            emitConstant(vm_->makeInt(0));
            return;
        }

        // Verifica caracteres extras
        if (*endptr != '\0')
        {
            error("Invalid characters in integer literal");
            emitConstant(vm_->makeInt(0));
            return;
        }

        // Verifica range específico se necessário
        if (options.checkIntegerOverflow)
        {
            if (value > INT64_MAX || value < INT64_MIN)
            {
                error("Integer literal exceeds 64-bit range");
                emitConstant(vm_->makeInt(0));
                return;
            }
        }

        // Emite valor apropriado
        if (value > INT32_MAX || value < INT32_MIN)
        {
            emitConstant(vm_->makeUInt(value));
        }
        else
        {
            emitConstant(vm_->makeInt((int)value));
        }
    }
    else // TOKEN_FLOAT
    {
        errno = 0;
        char *endptr = nullptr;
        double value = std::strtod(str, &endptr);

        // Verifica overflow/underflow
        if (errno == ERANGE)
        {
            if (value == HUGE_VAL || value == -HUGE_VAL)
            {
                error("Float literal overflow");
            }
            else
            {
                error("Float literal underflow");
            }
            emitConstant(vm_->makeDouble(0.0));
            return;
        }

        // Verifica parse válido
        if (endptr == str)
        {
            error("Invalid float literal");
            emitConstant(vm_->makeDouble(0.0));
            return;
        }

        // Verifica caracteres extras
        if (*endptr != '\0')
        {
            error("Invalid characters in float literal");
            emitConstant(vm_->makeDouble(0.0));
            return;
        }

        // Verifica NaN/Inf
        if (std::isnan(value))
        {
            error("Float literal is NaN");
            emitConstant(vm_->makeDouble(0.0));
            return;
        }

        if (std::isinf(value))
        {
            error("Float literal is infinite");
            emitConstant(vm_->makeDouble(0.0));
            return;
        }

        emitConstant(vm_->makeDouble(value));
    }
}

void Compiler::string(bool canAssign)
{
    (void)canAssign;
    emitConstant(vm_->makeString(previous.lexeme.c_str()));
}

void Compiler::literal(bool canAssign)
{
    (void)canAssign;
    switch (previous.type)
    {
    case TOKEN_TRUE:
        emitByte(OP_TRUE);
        break;
    case TOKEN_FALSE:
        emitByte(OP_FALSE);
        break;
    case TOKEN_NIL:
        emitByte(OP_NIL);
        break;
    default:
        return;
    }
}

void Compiler::grouping(bool canAssign)
{
    (void)canAssign;
    expression();
    if (hadError)
        return;
    consume(TOKEN_RPAREN, "Expect ')' after expression");
}

void Compiler::unary(bool canAssign)
{
    (void)canAssign;
    TokenType operatorType = previous.type;

    parsePrecedence(PREC_UNARY);

    switch (operatorType)
    {
    case TOKEN_MINUS:
        emitByte(OP_NEGATE);
        break;
    case TOKEN_BANG:
        emitByte(OP_NOT);
        break;
    case TOKEN_TILDE:
        emitByte(OP_BITWISE_NOT);
        break;
    default:
        return;
    }
}
void Compiler::binary(bool canAssign)
{
    (void)canAssign;
    TokenType operatorType = previous.type;
    ParseRule *rule = getRule(operatorType);

    parsePrecedence((Precedence)(rule->prec + 1));

    switch (operatorType)
    {
    case TOKEN_PLUS:
        emitByte(OP_ADD);
        break;
    case TOKEN_MINUS:
        emitByte(OP_SUBTRACT);
        break;
    case TOKEN_STAR:
        emitByte(OP_MULTIPLY);
        break;
    case TOKEN_SLASH:
        emitByte(OP_DIVIDE);
        break;
    case TOKEN_PERCENT:
        emitByte(OP_MODULO);
        break;
    case TOKEN_EQUAL_EQUAL:
        emitByte(OP_EQUAL);
        break;
    case TOKEN_BANG_EQUAL:
        emitByte(OP_EQUAL);
        emitByte(OP_NOT);
        break;

    case TOKEN_LESS:
        emitByte(OP_LESS);
        break;
    case TOKEN_LESS_EQUAL:
        emitByte(OP_GREATER);
        emitByte(OP_NOT);
        break;
    case TOKEN_GREATER:
        emitByte(OP_GREATER);
        break;
    case TOKEN_GREATER_EQUAL:
        emitByte(OP_LESS);
        emitByte(OP_NOT);
        break;
    case TOKEN_PIPE:
        emitByte(OP_BITWISE_OR);
        break;
    case TOKEN_AMPERSAND:
        emitByte(OP_BITWISE_AND);
        break;
    case TOKEN_CARET:
        emitByte(OP_BITWISE_XOR);
        break;
    case TOKEN_LEFT_SHIFT:
        emitByte(OP_SHIFT_LEFT);
        break;
    case TOKEN_RIGHT_SHIFT:
        emitByte(OP_SHIFT_RIGHT);
        break;
    default:
        return;
    }
}

void Compiler::ternary(bool canAssign)
{
    (void)canAssign;

    // Condition value is already on stack.
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP); // Pop condition for true branch

    // True expression
    parsePrecedence(PREC_ASSIGNMENT);
    consume(TOKEN_COLON, "Expect ':' in conditional expression");

    int endJump = emitJump(OP_JUMP);

    // False expression
    patchJump(elseJump);
    emitByte(OP_POP); // Pop condition for false branch
    parsePrecedence(PREC_ASSIGNMENT);

    patchJump(endJump);
}

void Compiler::bufferLiteral(bool canAssign)
{
    (void)canAssign; // Buffers não podem ser l-values

    consume(TOKEN_LPAREN, "Expect '(' after '@'");

    // Parse da expressão de tamanho (SIZE)
    // Pode ser: 4, x+2, getSize(), etc.
    expression();
    if (hadError)
        return;
    // Espera vírgula
    consume(TOKEN_COMMA, "Expect ',' in buffer literal");

    // Parse da expressão de tipo (TYPE)
    // Pode ser: TYPE_UINT8, getType(), etc.
    expression();
    if (hadError)
        return;
    // Espera ')' de fechamento
    consume(TOKEN_RPAREN, "Expect ')' after buffer literal");

    // Emite o opcode para criar o buffer
    emitByte(OP_NEW_BUFFER);
}

void Compiler::arrayLiteral(bool canAssign)
{
    (void)canAssign;

    int count = 0;

    if (!check(TOKEN_RBRACKET))
    {
        do
        {
            expression();
            if (hadError)
                return;
            count++;

            if (count > 65535)
            {
                error("Cannot have more than 65535 array elements on initialize.");

                while (!check(TOKEN_RBRACKET) && !check(TOKEN_EOF))
                {
                    if (match(TOKEN_COMMA))
                    {
                        expression();
                        if (hadError)
                            return;
                    }
                    else
                    {
                        advance();
                    }
                }
                break;
            }
        } while (match(TOKEN_COMMA));
    }

    consume(TOKEN_RBRACKET, "Expect ']' after array elements");

    if (!hadError)
    {
        emitByte(OP_DEFINE_ARRAY);
        emitShort((uint16)count);
    }
}

void Compiler::mapLiteral(bool canAssign)
{
    (void)canAssign;

    int count = 0;

    if (!check(TOKEN_RBRACE))
    {
        do
        {
            if (match(TOKEN_IDENTIFIER))
            {
                Token key = previous;
                emitConstant(vm_->makeString(key.lexeme.c_str()));
                consume(TOKEN_COLON, "Expect ':' after map key");
                expression();
                if (hadError)
                    return;
            }
            else if (match(TOKEN_STRING))
            {
                Token key = previous;
                emitConstant(vm_->makeString(key.lexeme.c_str()));
                consume(TOKEN_COLON, "Expect ':' after map key");
                expression();
                if (hadError)
                    return;
            }
            else
            {
                error("Expect identifier or string as map key");
                break;
            }

            count++;

            if (count > 65535)
            {
                error("Cannot have more than 65535 map entries");

                while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF))
                {
                    if (match(TOKEN_COMMA))
                    {
                        if (match(TOKEN_IDENTIFIER) || match(TOKEN_STRING))
                        {
                            consume(TOKEN_COLON, "Expect ':'");
                            expression();
                            if (hadError)
                                return;
                        }
                    }
                    else
                    {
                        advance();
                    }
                }
                break;
            }

        } while (match(TOKEN_COMMA));
    }

    consume(TOKEN_RBRACE, "Expect '}' after map elements");

    if (!hadError)
    {
        emitByte(OP_DEFINE_MAP);
        emitShort((uint16)count);
    }
}
