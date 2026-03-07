#pragma once
#include "config.hpp"
#include "string.hpp"


typedef const char* (*FileLoaderCallback)(const char* filename, size_t* outSize, void* userdata);



static constexpr int MAX_PRIVATES = 28;
 
static constexpr int STACK_MAX = 1024;
static constexpr int FRAMES_MAX = 1024;
static constexpr int GOSUB_MAX = 16;
static constexpr int TRY_MAX = 8;

enum class InterpretResult : uint8
{
    OK,
    COMPILE_ERROR,
    RUNTIME_ERROR
};

enum class ProcessState : uint8
{
    RUNNING,
    SUSPENDED,
    FROZEN,
    DEAD
};


enum class FunctionType  : uint8
{
    TYPE_FUNCTION,      // def normal
    TYPE_METHOD,        // método de class
    TYPE_INITIALIZER,   // init (construtor)
    TYPE_SCRIPT         // top-level script
};

