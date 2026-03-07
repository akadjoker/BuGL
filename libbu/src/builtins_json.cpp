#include "interpreter.hpp"

#ifdef BU_ENABLE_JSON

#include "utf8_utils.h"
#include <cctype>
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

struct JsonStringifyContext
{
    bool pretty = false;
    int indentWidth = 0;
    std::vector<const void *> stack;
    std::string error;
};

static void jsonWriteIndent(std::string &out, int depth, int indentWidth)
{
    if (indentWidth <= 0 || depth <= 0)
    {
        return;
    }
    out.append((size_t)(depth * indentWidth), ' ');
}

static bool jsonContainsPointer(const std::vector<const void *> &stack, const void *ptr)
{
    for (size_t i = 0; i < stack.size(); i++)
    {
        if (stack[i] == ptr)
        {
            return true;
        }
    }
    return false;
}

static void jsonAppendEscapedString(const char *text, std::string &out)
{
    const unsigned char *p = (const unsigned char *)text;
    while (*p)
    {
        unsigned char c = *p++;
        switch (c)
        {
        case '"':
            out += "\\\"";
            break;
        case '\\':
            out += "\\\\";
            break;
        case '\b':
            out += "\\b";
            break;
        case '\f':
            out += "\\f";
            break;
        case '\n':
            out += "\\n";
            break;
        case '\r':
            out += "\\r";
            break;
        case '\t':
            out += "\\t";
            break;
        default:
            if (c < 0x20)
            {
                char buffer[8];
                snprintf(buffer, sizeof(buffer), "\\u%04x", (unsigned int)c);
                out += buffer;
            }
            else
            {
                out += (char)c;
            }
            break;
        }
    }
}

static bool jsonStringifyValue(const Value &value, int depth,
                               JsonStringifyContext &ctx, std::string &out)
{
    switch (value.type)
    {
    case ValueType::NIL:
        out += "null";
        return true;
    case ValueType::BOOL:
        out += value.asBool() ? "true" : "false";
        return true;
    case ValueType::BYTE:
        out += std::to_string((unsigned int)value.asByte());
        return true;
    case ValueType::INT:
        out += std::to_string(value.asInt());
        return true;
    case ValueType::UINT:
        out += std::to_string(value.asUInt());
        return true;
    case ValueType::FLOAT:
    case ValueType::DOUBLE:
    {
        double number = value.asNumber();
        if (!std::isfinite(number))
        {
            ctx.error = "cannot serialize NaN or Infinity";
            return false;
        }

        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%.17g", number);
        out += buffer;
        return true;
    }
    case ValueType::STRING:
        out += '"';
        jsonAppendEscapedString(value.asStringChars(), out);
        out += '"';
        return true;
    case ValueType::ARRAY:
    {
        ArrayInstance *arr = value.asArray();
        if (jsonContainsPointer(ctx.stack, arr))
        {
            ctx.error = "cyclic array detected";
            return false;
        }

        ctx.stack.push_back(arr);

        out += '[';
        if (arr->values.size() > 0)
        {
            if (ctx.pretty)
            {
                out += '\n';
            }

            for (size_t i = 0; i < arr->values.size(); i++)
            {
                if (ctx.pretty)
                {
                    jsonWriteIndent(out, depth + 1, ctx.indentWidth);
                }

                if (!jsonStringifyValue(arr->values[i], depth + 1, ctx, out))
                {
                    ctx.stack.pop_back();
                    return false;
                }

                if (i + 1 < arr->values.size())
                {
                    out += ',';
                }

                if (ctx.pretty)
                {
                    out += '\n';
                }
            }

            if (ctx.pretty)
            {
                jsonWriteIndent(out, depth, ctx.indentWidth);
            }
        }
        out += ']';

        ctx.stack.pop_back();
        return true;
    }
    case ValueType::MAP:
    {
        MapInstance *map = value.asMap();
        if (jsonContainsPointer(ctx.stack, map))
        {
            ctx.error = "cyclic object detected";
            return false;
        }

        ctx.stack.push_back(map);

        out += '{';
        if (map->table.count > 0)
        {
            if (ctx.pretty)
            {
                out += '\n';
            }

            bool first = true;
            bool ok = true;

            map->table.forEach([&](String *key, Value val)
                               {
                if (!ok)
                {
                    return;
                }

                if (!first)
                {
                    out += ',';
                    if (ctx.pretty)
                    {
                        out += '\n';
                    }
                }

                if (ctx.pretty)
                {
                    jsonWriteIndent(out, depth + 1, ctx.indentWidth);
                }

                out += '"';
                jsonAppendEscapedString(key->chars(), out);
                out += '"';
                out += ctx.pretty ? ": " : ":";

                if (!jsonStringifyValue(val, depth + 1, ctx, out))
                {
                    ok = false;
                    return;
                }

                first = false; });

            if (!ok)
            {
                ctx.stack.pop_back();
                return false;
            }

            if (ctx.pretty)
            {
                out += '\n';
                jsonWriteIndent(out, depth, ctx.indentWidth);
            }
        }
        out += '}';

        ctx.stack.pop_back();
        return true;
    }
    default:
        ctx.error = std::string("type '") + valueTypeToString(value.type) +
                    "' is not JSON serializable";
        return false;
    }
}

class JsonParser
{
public:
    JsonParser(Interpreter *vm, const char *src)
        : vm_(vm), src_(src), len_(strlen(src)), pos_(0)
    {
    }

    bool parse(Value *out)
    {
        skipWhitespace();
        if (!parseValue(out))
        {
            return false;
        }
        skipWhitespace();
        if (pos_ != len_)
        {
            setError("unexpected trailing characters");
            return false;
        }
        return true;
    }

    const std::string &error() const
    {
        return error_;
    }

private:
    Interpreter *vm_;
    const char *src_;
    size_t len_;
    size_t pos_;
    std::string error_;

    bool parseValue(Value *out)
    {
        skipWhitespace();
        if (pos_ >= len_)
        {
            setError("unexpected end of input");
            return false;
        }

        char c = src_[pos_];
        if (c == '"')
        {
            std::string text;
            if (!parseString(&text))
            {
                return false;
            }
            *out = vm_->makeString(text.c_str());
            return true;
        }
        if (c == '{')
        {
            return parseObject(out);
        }
        if (c == '[')
        {
            return parseArray(out);
        }
        if (c == 't')
        {
            return parseLiteral("true", vm_->makeBool(true), out);
        }
        if (c == 'f')
        {
            return parseLiteral("false", vm_->makeBool(false), out);
        }
        if (c == 'n')
        {
            return parseLiteral("null", vm_->makeNil(), out);
        }
        if (c == '-' || std::isdigit((unsigned char)c))
        {
            return parseNumber(out);
        }

        setError("unexpected token");
        return false;
    }

    bool parseLiteral(const char *literal, Value literalValue, Value *out)
    {
        size_t n = strlen(literal);
        if (pos_ + n > len_ || strncmp(src_ + pos_, literal, n) != 0)
        {
            setError(std::string("expected '") + literal + "'");
            return false;
        }
        pos_ += n;
        *out = literalValue;
        return true;
    }

    bool parseArray(Value *out)
    {
        if (!consume('['))
        {
            setError("expected '['");
            return false;
        }

        Value arrayValue = vm_->makeArray();
        vm_->push(arrayValue); // Keep root alive while parsing nested elements.
        ArrayInstance *arr = arrayValue.asArray();

        skipWhitespace();
        if (consume(']'))
        {
            *out = arrayValue;
            vm_->pop();
            return true;
        }

        while (true)
        {
            Value element;
            if (!parseValue(&element))
            {
                vm_->pop();
                return false;
            }

            bool keepElement = element.isObject();
            if (keepElement)
            {
                vm_->push(element);
            }
            arr->values.push(element);
            if (keepElement)
            {
                vm_->pop();
            }

            skipWhitespace();
            if (consume(']'))
            {
                *out = arrayValue;
                vm_->pop();
                return true;
            }
            if (!consume(','))
            {
                vm_->pop();
                setError("expected ',' or ']' in array");
                return false;
            }
            skipWhitespace();
        }
    }

    bool parseObject(Value *out)
    {
        if (!consume('{'))
        {
            setError("expected '{'");
            return false;
        }

        Value mapValue = vm_->makeMap();
        vm_->push(mapValue); // Keep root alive while parsing nested values.
        MapInstance *map = mapValue.asMap();

        skipWhitespace();
        if (consume('}'))
        {
            *out = mapValue;
            vm_->pop();
            return true;
        }

        while (true)
        {
            std::string key;
            if (!parseString(&key))
            {
                vm_->pop();
                return false;
            }

            skipWhitespace();
            if (!consume(':'))
            {
                vm_->pop();
                setError("expected ':' after object key");
                return false;
            }

            Value val;
            if (!parseValue(&val))
            {
                vm_->pop();
                return false;
            }

            bool keepValue = val.isObject();
            if (keepValue)
            {
                vm_->push(val);
            }
            map->table.set(vm_->makeString(key.c_str()).asString(), val);
            if (keepValue)
            {
                vm_->pop();
            }

            skipWhitespace();
            if (consume('}'))
            {
                *out = mapValue;
                vm_->pop();
                return true;
            }
            if (!consume(','))
            {
                vm_->pop();
                setError("expected ',' or '}' in object");
                return false;
            }
            skipWhitespace();
        }
    }

    bool parseString(std::string *out)
    {
        if (!consume('"'))
        {
            setError("expected string");
            return false;
        }

        out->clear();

        while (pos_ < len_)
        {
            char c = src_[pos_++];
            if (c == '"')
            {
                return true;
            }

            if (c == '\\')
            {
                if (pos_ >= len_)
                {
                    setError("incomplete escape sequence");
                    return false;
                }

                char esc = src_[pos_++];
                switch (esc)
                {
                case '"':
                    out->push_back('"');
                    break;
                case '\\':
                    out->push_back('\\');
                    break;
                case '/':
                    out->push_back('/');
                    break;
                case 'b':
                    out->push_back('\b');
                    break;
                case 'f':
                    out->push_back('\f');
                    break;
                case 'n':
                    out->push_back('\n');
                    break;
                case 'r':
                    out->push_back('\r');
                    break;
                case 't':
                    out->push_back('\t');
                    break;
                case 'u':
                    if (!appendUnicodeEscape(out))
                    {
                        return false;
                    }
                    break;
                default:
                    setError("invalid escape sequence");
                    return false;
                }
                continue;
            }

            if ((unsigned char)c < 0x20)
            {
                setError("unescaped control character in string");
                return false;
            }

            out->push_back(c);
        }

        setError("unterminated string");
        return false;
    }

    bool appendUnicodeEscape(std::string *out)
    {
        int hi = 0;
        if (!parseHex4(&hi))
        {
            return false;
        }

        int codepoint = hi;
        if (hi >= 0xD800 && hi <= 0xDBFF)
        {
            if (pos_ + 6 > len_ || src_[pos_] != '\\' || src_[pos_ + 1] != 'u')
            {
                setError("expected low surrogate pair");
                return false;
            }
            pos_ += 2;

            int lo = 0;
            if (!parseHex4(&lo))
            {
                return false;
            }
            if (lo < 0xDC00 || lo > 0xDFFF)
            {
                setError("invalid low surrogate");
                return false;
            }

            codepoint = 0x10000 + ((hi - 0xD800) << 10) + (lo - 0xDC00);
        }
        else if (hi >= 0xDC00 && hi <= 0xDFFF)
        {
            setError("unexpected low surrogate");
            return false;
        }

        uint8_t bytes[4];
        int n = Utf8Encode(codepoint, bytes);
        if (n <= 0)
        {
            setError("invalid unicode codepoint");
            return false;
        }
        out->append((const char *)bytes, (size_t)n);
        return true;
    }

    bool parseHex4(int *out)
    {
        if (pos_ + 4 > len_)
        {
            setError("expected 4 hex digits");
            return false;
        }

        int value = 0;
        for (int i = 0; i < 4; i++)
        {
            char c = src_[pos_++];
            int nibble = 0;

            if (c >= '0' && c <= '9')
            {
                nibble = c - '0';
            }
            else if (c >= 'a' && c <= 'f')
            {
                nibble = c - 'a' + 10;
            }
            else if (c >= 'A' && c <= 'F')
            {
                nibble = c - 'A' + 10;
            }
            else
            {
                setError("invalid unicode escape");
                return false;
            }

            value = (value << 4) | nibble;
        }

        *out = value;
        return true;
    }

    bool parseNumber(Value *out)
    {
        size_t start = pos_;

        if (consume('-') && pos_ >= len_)
        {
            setError("invalid number");
            return false;
        }

        if (pos_ >= len_)
        {
            setError("invalid number");
            return false;
        }

        if (src_[pos_] == '0')
        {
            pos_++;
            if (pos_ < len_ && std::isdigit((unsigned char)src_[pos_]))
            {
                setError("leading zeroes are not allowed");
                return false;
            }
        }
        else
        {
            if (!std::isdigit((unsigned char)src_[pos_]))
            {
                setError("invalid number");
                return false;
            }
            while (pos_ < len_ && std::isdigit((unsigned char)src_[pos_]))
            {
                pos_++;
            }
        }

        bool hasFraction = false;
        bool hasExponent = false;

        if (pos_ < len_ && src_[pos_] == '.')
        {
            hasFraction = true;
            pos_++;

            if (pos_ >= len_ || !std::isdigit((unsigned char)src_[pos_]))
            {
                setError("invalid fraction");
                return false;
            }

            while (pos_ < len_ && std::isdigit((unsigned char)src_[pos_]))
            {
                pos_++;
            }
        }

        if (pos_ < len_ && (src_[pos_] == 'e' || src_[pos_] == 'E'))
        {
            hasExponent = true;
            pos_++;

            if (pos_ < len_ && (src_[pos_] == '+' || src_[pos_] == '-'))
            {
                pos_++;
            }

            if (pos_ >= len_ || !std::isdigit((unsigned char)src_[pos_]))
            {
                setError("invalid exponent");
                return false;
            }

            while (pos_ < len_ && std::isdigit((unsigned char)src_[pos_]))
            {
                pos_++;
            }
        }

        std::string text(src_ + start, pos_ - start);

        if (!hasFraction && !hasExponent)
        {
            errno = 0;
            char *endInt = nullptr;
            long long integer = strtoll(text.c_str(), &endInt, 10);

            if (errno != ERANGE && endInt && *endInt == '\0')
            {
                if (integer >= INT_MIN && integer <= INT_MAX)
                {
                    *out = vm_->makeInt((int)integer);
                    return true;
                }
                if (integer >= 0 && integer <= (long long)UINT_MAX)
                {
                    *out = vm_->makeUInt((uint32)integer);
                    return true;
                }
            }
        }

        errno = 0;
        char *end = nullptr;
        double number = strtod(text.c_str(), &end);

        if (!end || *end != '\0')
        {
            setError("invalid number");
            return false;
        }

        if (errno == ERANGE || !std::isfinite(number))
        {
            setError("number out of range");
            return false;
        }

        *out = vm_->makeDouble(number);
        return true;
    }

    void skipWhitespace()
    {
        while (pos_ < len_)
        {
            char c = src_[pos_];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
            {
                pos_++;
                continue;
            }
            break;
        }
    }

    bool consume(char expected)
    {
        if (pos_ < len_ && src_[pos_] == expected)
        {
            pos_++;
            return true;
        }
        return false;
    }

    void setError(const std::string &message)
    {
        if (!error_.empty())
        {
            return;
        }

        size_t line = 1;
        size_t column = 1;

        for (size_t i = 0; i < pos_ && i < len_; i++)
        {
            if (src_[i] == '\n')
            {
                line++;
                column = 1;
            }
            else
            {
                column++;
            }
        }

        error_ = message + " at line " + std::to_string(line) +
                 ", column " + std::to_string(column);
    }
};

int native_json_parse(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isString())
    {
        vm->runtimeError("json.parse expects a JSON string");
        return 0;
    }

    JsonParser parser(vm, args[0].asStringChars());
    Value result;
    if (!parser.parse(&result))
    {
        vm->runtimeError("json.parse: %s", parser.error().c_str());
        return 0;
    }

    vm->push(result);
    return 1;
}

int native_json_stringify(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1)
    {
        vm->runtimeError("json.stringify expects value");
        return 0;
    }

    JsonStringifyContext ctx;
    if (argCount >= 2)
    {
        if (args[1].isBool())
        {
            ctx.pretty = args[1].asBool();
            ctx.indentWidth = ctx.pretty ? 2 : 0;
        }
        else if (args[1].isInt())
        {
            int width = args[1].asInt();
            if (width < 0)
            {
                width = 0;
            }
            if (width > 16)
            {
                width = 16;
            }
            ctx.indentWidth = width;
            ctx.pretty = width > 0;
        }
        else
        {
            vm->runtimeError("json.stringify second argument must be bool or int");
            return 0;
        }
    }

    std::string output;
    if (!jsonStringifyValue(args[0], 0, ctx, output))
    {
        vm->runtimeError("json.stringify: %s", ctx.error.c_str());
        return 0;
    }

    vm->push(vm->makeString(output.c_str()));
    return 1;
}

void Interpreter::registerJSON()
{
    addModule("json")
        .addFunction("parse", native_json_parse, 1)
        .addFunction("stringify", native_json_stringify, -1);
}

#endif
