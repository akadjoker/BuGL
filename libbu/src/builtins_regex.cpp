#include "interpreter.hpp"

#ifdef BU_ENABLE_REGEX

#include <regex>
#include <string>

int native_regex_match(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 2 || !args[0].isString() || !args[1].isString())
    {
        vm->runtimeError("regex.match expects (pattern, text)");
        return 0;
    }

    try
    {
        const std::regex re(args[0].asStringChars());
        const bool matched = std::regex_match(args[1].asStringChars(), re);
        vm->push(vm->makeBool(matched));
        return 1;
    }
    catch (const std::regex_error &e)
    {
        vm->runtimeError("regex.match invalid pattern: %s", e.what());
        return 0;
    }
}

int native_regex_search(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 2 || !args[0].isString() || !args[1].isString())
    {
        vm->runtimeError("regex.search expects (pattern, text)");
        return 0;
    }

    try
    {
        const std::regex re(args[0].asStringChars());
        const bool found = std::regex_search(args[1].asStringChars(), re);
        vm->push(vm->makeBool(found));
        return 1;
    }
    catch (const std::regex_error &e)
    {
        vm->runtimeError("regex.search invalid pattern: %s", e.what());
        return 0;
    }
}

int native_regex_replace(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 3 || !args[0].isString() || !args[1].isString() || !args[2].isString())
    {
        vm->runtimeError("regex.replace expects (pattern, replacement, text)");
        return 0;
    }

    try
    {
        const std::regex re(args[0].asStringChars());
        const std::string replaced = std::regex_replace(
            std::string(args[2].asStringChars()),
            re,
            std::string(args[1].asStringChars()));
        vm->push(vm->makeString(replaced.c_str()));
        return 1;
    }
    catch (const std::regex_error &e)
    {
        vm->runtimeError("regex.replace invalid pattern: %s", e.what());
        return 0;
    }
}

int native_regex_findall(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 2 || !args[0].isString() || !args[1].isString())
    {
        vm->runtimeError("regex.findall expects (pattern, text)");
        return 0;
    }

    try
    {
        const std::regex re(args[0].asStringChars());
        const std::string text = args[1].asStringChars();

        Value out = vm->makeArray();
        ArrayInstance *arr = out.asArray();

        std::sregex_iterator it(text.begin(), text.end(), re);
        std::sregex_iterator end;
        for (; it != end; ++it)
        {
            arr->values.push(vm->makeString(it->str().c_str()));
        }

        vm->push(out);
        return 1;
    }
    catch (const std::regex_error &e)
    {
        vm->runtimeError("regex.findall invalid pattern: %s", e.what());
        return 0;
    }
}

void Interpreter::registerRegex()
{
    addModule("regex")
        .addFunction("match", native_regex_match, 2)
        .addFunction("search", native_regex_search, 2)
        .addFunction("replace", native_regex_replace, 3)
        .addFunction("findall", native_regex_findall, 2);
}

#endif
