#include "interpreter.hpp"

#ifdef BU_ENABLE_PATH

#include "platform.hpp"

#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <sys/stat.h>
#define getcwd _getcwd
#define chdir _chdir
#else
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>
#endif

int native_path_join(Interpreter *vm, int argCount, Value *args)
{
    std::string result;
    for (int i = 0; i < argCount; i++)
    {
        if (!args[i].isString()) continue;
        
        if (i > 0 && !result.empty() && result.back() != '/' && result.back() != '\\')
        {
#ifdef _WIN32
            result += '\\';
#else
            result += '/';
#endif
        }
        result += args[i].asStringChars();
    }
    vm->push(vm->makeString(result.c_str()));
    return 1;
}

int native_path_normalize(Interpreter *vm, int argCount, Value *args)
{
    if (!args[0].isString())
        return 0;
    
    std::string path = args[0].asStringChars();
    std::string result;
    std::vector<std::string> parts;
    
    size_t start = 0;
    for (size_t i = 0; i <= path.size(); i++)
    {
        if (i == path.size() || path[i] == '/' || path[i] == '\\')
        {
            if (i > start)
            {
                std::string part = path.substr(start, i - start);
                if (part == "..")
                {
                    if (!parts.empty()) parts.pop_back();
                }
                else if (part != ".")
                {
                    parts.push_back(part);
                }
            }
            start = i + 1;
        }
    }
    
    for (size_t i = 0; i < parts.size(); i++)
    {
        if (i > 0) result += "/";
        result += parts[i];
    }
    
    vm->push(vm->makeString(result.c_str()));
    return 1;
}

int native_path_basename(Interpreter *vm, int argCount, Value *args)
{
    if (!args[0].isString())
        return 0;
    
    std::string path = args[0].asStringChars();
    size_t pos = path.find_last_of("/\\");
    
    if (pos != std::string::npos)
    {
        vm->push(vm->makeString(path.substr(pos + 1).c_str()));
        return 1;
    }
    
    vm->push(args[0]);
    return 1;
}

int native_path_dirname(Interpreter *vm, int argCount, Value *args)
{
    if (!args[0].isString())
        return 0;
    
    std::string path = args[0].asStringChars();
    size_t pos = path.find_last_of("/\\");
    
    if (pos != std::string::npos)
    {
        vm->push(vm->makeString(path.substr(0, pos).c_str()));
        return 1;
    }
    
    vm->push(vm->makeString("."));
    return 1;
}

int native_path_exists(Interpreter *vm, int argCount, Value *args)
{
    if (!args[0].isString())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }
    
    vm->push(vm->makeBool(OsFileExists(args[0].asStringChars())));
    return 1;
}

int native_path_extname(Interpreter *vm, int argCount, Value *args)
{
    if (!args[0].isString())
    {
        vm->push(vm->makeString(""));
        return 1;
    }
    
    std::string path = args[0].asStringChars();
    size_t pos = path.find_last_of('.');
    size_t slash = path.find_last_of("/\\");
    
    if (pos != std::string::npos && 
        (slash == std::string::npos || pos > slash))
    {
        vm->push(vm->makeString(path.substr(pos).c_str()));
        return 1;
    }
    
    vm->push(vm->makeString(""));
    return 1;
}

int native_path_isdir(Interpreter *vm, int argCount, Value *args)
{
    if (!args[0].isString())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }
    
    const char *path = args[0].asStringChars();
    
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path);
    vm->push(vm->makeBool(attrs != INVALID_FILE_ATTRIBUTES && 
                         (attrs & FILE_ATTRIBUTE_DIRECTORY)));
#else
    struct stat st;
    vm->push(vm->makeBool(stat(path, &st) == 0 && S_ISDIR(st.st_mode)));
#endif
    return 1;
}

int native_path_isfile(Interpreter *vm, int argCount, Value *args)
{
    if (!args[0].isString())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }
    
    const char *path = args[0].asStringChars();
    
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path);
    vm->push(vm->makeBool(attrs != INVALID_FILE_ATTRIBUTES && 
                         !(attrs & FILE_ATTRIBUTE_DIRECTORY)));
#else
    struct stat st;
    vm->push(vm->makeBool(stat(path, &st) == 0 && S_ISREG(st.st_mode)));
#endif
    return 1;
}
 

void Interpreter::registerPath()
{
     addModule("path")
        .addFunction("join", native_path_join, -1)
        .addFunction("normalize", native_path_normalize, 1)
        .addFunction("dirname", native_path_dirname, 1)
        .addFunction("basename", native_path_basename, 1)
        .addFunction("extname", native_path_extname, 1)
        .addFunction("exists", native_path_exists, 1)
        .addFunction("isdir", native_path_isdir, 1)
        .addFunction("isfile", native_path_isfile, 1);
}

#endif