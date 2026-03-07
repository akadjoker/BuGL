#include "config.hpp"
#include <cstdio>
#include <time.h>
#include <stdarg.h>
#include "platform.hpp"



#define BUFFER_SIZE 64

const char *doubleToString(double value)
{
	static char buffer[BUFFER_SIZE];
	snprintf(buffer, BUFFER_SIZE, "%f", value);
	return buffer;
}

const char *longToString(long value)
{
	static char buffer[BUFFER_SIZE];
	snprintf(buffer, BUFFER_SIZE, "%ld", value);
	return buffer;
}

 


static void Log(int severity, const char *fmt, va_list args)
{
	const char *type;
	const char *color;
	switch (severity)
	{
	case 0:
		type = "info";
		color = CONSOLE_COLOR_GREEN;
		break;
	case 1:
		type = "warning";
		color = CONSOLE_COLOR_PURPLE;
		break;
	case 2:
		type = "error";
		color = CONSOLE_COLOR_RED;
		break;
	case 3:
		type = "> ";
		color = CONSOLE_COLOR_YELLOW;
		break;
	default:
		type = "unknown";
		color = CONSOLE_COLOR_RESET;
	}

	time_t rawTime;
	struct tm *timeInfo;
	char timeBuffer[80];

	time(&rawTime);
	timeInfo = localtime(&rawTime);

	strftime(timeBuffer, sizeof(timeBuffer), "[%H:%M:%S]", timeInfo);

	char consoleFormat[1024];
	snprintf(consoleFormat, sizeof(consoleFormat), "%s%s %s%s%s: %s\n", CONSOLE_COLOR_CYAN,
			 timeBuffer, color, type, CONSOLE_COLOR_RESET, fmt);

	char fileFormat[1024];
	snprintf(fileFormat, sizeof(fileFormat), "%s %s: %s", timeBuffer, type, fmt);

	va_list argsCopy;
	va_copy(argsCopy, args);

	char consoleMessage[4096];
	vsnprintf(consoleMessage, sizeof(consoleMessage), consoleFormat, args);
	OsPrintf("%s", consoleMessage);

	va_end(argsCopy);
}

void Trace(int severity,  const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	Log(severity, fmt, args);
	va_end(args);
}


void Warning( const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	Log(1, fmt, args);
	va_end(args);
}

void Info( const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	Log(0, fmt, args);
	va_end(args);
}


void Error( const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	Log(2, fmt, args);
	va_end(args);
}

void Print( const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	Log(3, fmt, args);
	va_end(args);
}

char *LoadTextFile(const char *fileName)
{
    char *text = nullptr;

    if (fileName != nullptr)
    {
        FILE *file = fopen(fileName, "rt");

        if (file != nullptr)
        {
            fseek(file, 0, SEEK_END);
            long fileSize = ftell(file);
            fseek(file, 0, SEEK_SET);

            if (fileSize > 0)
            {
                size_t size = static_cast<size_t>(fileSize);
                text = (char *)aAlloc(size + 1);  // malloc em vez de realloc(NULL)
                
                if (text != nullptr)
                {
                    size_t count = fread(text, sizeof(char), size, file);
                    
                    // Shrink se leu menos (opcional)
                    if (count < size) {
                        char* newText = (char *)aRealloc(text, count + 1);
                        if (newText) {
                            text = newText;
                        }
                    }
                    
                    text[count] = '\0';
                }
                else {
                    Trace(1, "Failed to allocate memory for %s reading", fileName);
                }
            }
            else {
                Trace(1, "Failed to read text from %s", fileName);
            }

            fclose(file);
        }
    }
    return text;
}

void FreeTextFile(char *text)
{
	if (text != NULL)
		aFree(text);
}