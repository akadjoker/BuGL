#pragma once
#include "config.hpp"


static inline const char* formatBytes(size_t bytes)
{
    static char buffer[32];

    if (bytes < 1024)
        snprintf(buffer, sizeof(buffer), "%zu B", bytes);
    else if (bytes < 1024 * 1024)
        snprintf(buffer, sizeof(buffer), "%zu KB", bytes / 1024);
    else
        snprintf(buffer, sizeof(buffer), "%zu MB", bytes / (1024 * 1024));

    return buffer;
}


char *LoadTextFile(const char *fileName);
void FreeTextFile(char *text);


const char *longToString(long value);
const char *doubleToString(double value);