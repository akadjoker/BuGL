#pragma once
#include "config.hpp"

struct Function;
class Code;

class Debug
{
public:
    // Para uso do Compiler - setar nomes globais antes de dump
    static void setGlobalNames(const char** names, int count);
    static void clearGlobalNames();

    // Disassemble um chunk inteiro
    static void disassembleChunk(const Code &chunk, const char *name);

    static void dumpFunction(const Function *func);

    // Disassemble uma única instrução
    static size_t disassembleInstruction(const Code &chunk, size_t offset);

private:
    // Helpers por tipo de instrução
    static size_t simpleInstruction(const char *name, size_t offset);

    static size_t constantInstruction(
        const char *name,
        const Code &chunk,
        size_t offset);

    // Para globals (imprime nome da variável se for string)
    static size_t constantNameInstruction(
        const char *name,
        const Code &chunk,
        size_t offset);

    // Para globals com índice direto no array (OPTIMIZATION)
    static size_t globalIndexInstruction(
        const char *name,
        const Code &chunk,
        size_t offset);

    static size_t byteInstruction(
        const char *name,
        const Code &chunk,
        size_t offset);

    static size_t shortInstruction(
        const char *name,
        const Code &chunk,
        size_t offset);

    static size_t jumpInstruction(
        const char *name,
        int sign,
        const Code &chunk,
        size_t offset);

 
};