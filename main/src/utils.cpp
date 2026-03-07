
 
#include "utils.hpp"

#if defined(PLATFORM_DESKTOP) && defined(_WIN32) && (defined(_MSC_VER) || defined(__TINYC__))

#include "wdirent.h" // Required for: DIR, opendir(), closedir() [Used in LoadDirectoryFiles()]
#else
#include <dirent.h> // Required for: DIR, opendir(), closedir() [Used in LoadDirectoryFiles()]
#endif

#if defined(_WIN32)
#include <direct.h>    // Required for: _getch(), _chdir()
#define GETCWD _getcwd // NOTE: MSDN recommends not to use getcwd(), chdir()
#define CHDIR _chdir
#include <io.h> // Required for: _access() [Used in FileExists()]
#else
#include <unistd.h> // Required for: getch(), chdir() (POSIX), access()
#define GETCWD getcwd
#define CHDIR chdir
#endif

#ifdef PLATFORM_WIN

#define CONSOLE_COLOR_RESET ""
#define CONSOLE_COLOR_GREEN ""
#define CONSOLE_COLOR_RED ""
#define CONSOLE_COLOR_PURPLE ""

#else

#define CONSOLE_COLOR_RESET "\033[0m"
#define CONSOLE_COLOR_GREEN "\033[1;32m"
#define CONSOLE_COLOR_RED "\033[1;31m"
#define CONSOLE_COLOR_PURPLE "\033[1;35m"

#endif

#define MAX_TEXT_BUFFER_LENGTH 512


#include <SDL2/SDL.h>
#include <time.h>
#include <sys/stat.h> 
#include <climits>
#include <vector>
#include <string>
#include <cstdarg>

static void LogMessage(int level, const char *msg, va_list args)
{
    time_t rawTime;
    struct tm *timeInfo;
    char timeBuffer[80];

    time(&rawTime);
    timeInfo = localtime(&rawTime);

    strftime(timeBuffer, sizeof(timeBuffer), "[%H:%M:%S]", timeInfo);

    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), msg, args);

    switch (level)
    {
    case 0:
        SDL_LogInfo(0, "%s%s: %s%s", CONSOLE_COLOR_GREEN, timeBuffer, buffer, CONSOLE_COLOR_RESET);
        break;
    case 1:
        SDL_LogError(0, "%s%s: %s%s", CONSOLE_COLOR_RED, timeBuffer, buffer, CONSOLE_COLOR_RESET);
        break;
    case 2:
        SDL_LogWarn(0, "%s%s: %s%s", CONSOLE_COLOR_PURPLE, timeBuffer, buffer, CONSOLE_COLOR_RESET);
        break;
    }
}

void LogError(const char *msg, ...)
{

    va_list args;
    va_start(args, msg);
    LogMessage(1, msg, args);
    va_end(args);
}

void LogWarning(const char *msg, ...)
{

    va_list args;
    va_start(args, msg);
    LogMessage(2, msg, args);
    va_end(args);
}

void LogInfo(const char *msg, ...)
{
    va_list args;
    va_start(args, msg);
    LogMessage(0, msg, args);
    va_end(args);
}

namespace Utils
{

    bool FileExists(const char *fileName)
    {
        bool result = false;

#if defined(_WIN32)
        if (_access(fileName, 0) != -1)
            result = true;
#else
        if (access(fileName, F_OK) != -1)
            result = true;
#endif
        return result;
    }

    bool DirectoryExists(const char *dirPath)
    {
        bool result = false;

        DIR *dir = opendir(dirPath);
        if (dir != NULL)
        {
            result = true;
            closedir(dir);
        }

        return result;
    }

    bool IsFileExtension(const char *fileName, const char *ext)
    {
        bool result = false;

        const char *fileExt = strrchr(fileName, '.');

        if (fileExt != NULL)
        {
            if (strcmp(fileExt, ext) == 0)
                result = true;
        }

        return result;
    }

    const char *GetFileExtension(const char *fileName)
    {
        const char *fileExt = strrchr(fileName, '.');

        if (fileExt != NULL)
            return fileExt;

        return NULL;
    }

    const char *GetFileName(const char *filePath)
    {
        const char *fileName = strrchr(filePath, '/');

        if (fileName != NULL)
            return fileName + 1;

        return filePath;
    }

    const char *GetFileNameWithoutExt(const char *filePath)
    {
        static char baseName[256];
        strcpy(baseName, GetFileName(filePath));

        char *dot = strrchr(baseName, '.');
        if (dot)
            *dot = '\0';

        return baseName;
    }

    const char *GetDirectoryPath(const char *filePath)
    {
        static char dirPath[256];
        strcpy(dirPath, filePath);

        char *lastSlash = strrchr(dirPath, '/');

        if (lastSlash != NULL)
            lastSlash[1] = '\0';
        else
            dirPath[0] = '\0';

        return dirPath;
    }

    const char *GetPrevDirectoryPath(const char *dirPath)
    {
        static char prevDirPath[256];
        strcpy(prevDirPath, dirPath);

        char *lastSlash = strrchr(prevDirPath, '/');

        if (lastSlash != NULL)
        {
            lastSlash[0] = '\0';
            lastSlash = strrchr(prevDirPath, '/');
            if (lastSlash != NULL)
                lastSlash[1] = '\0';
            else
                prevDirPath[0] = '\0';
        }
        else
            prevDirPath[0] = '\0';

        return prevDirPath;
    }

    char *GetWorkingDirectory()
    {

        return SDL_GetBasePath();
    }

    char *GetApplicationDirectory()
    {
        static char appDir[256];

        if (GETCWD(appDir, sizeof(appDir)) != NULL)
        {
            return appDir;
        }
        else
        {
            LogError("Failed to get application directory");
            return NULL;
        }
    }

    bool ChangeDirectory(const char *dir)
    {
        return CHDIR(dir) == 0;
    }

    bool IsPathFile(const char *path)
    {
        bool result = false;

        DIR *dir = opendir(path);
        if (dir == NULL)
            result = true;

        return result;
    }

    unsigned char *LoadFileData(const char *fileName, unsigned int *bytesRead)
    {
        unsigned char *data = NULL;
        *bytesRead = 0;

        SDL_RWops *file = SDL_RWFromFile(fileName, "rb");

        if (file != NULL)
        {
            unsigned int size = (int)SDL_RWsize(file);

            if (size > 0)
            {
                data = (unsigned char *)SDL_malloc(size * sizeof(unsigned char));

                unsigned int count = (unsigned int)SDL_RWread(file, data, sizeof(unsigned char), size);
                *bytesRead = count;

                LogInfo("FILEIO: [%s] File loaded successfully", fileName);
            }
            else
                LogError("FILEIO: [%s] Failed to read file", fileName);
            SDL_RWclose(file);
        }
        else
            LogError("FILEIO: [%s] Failed to open file", fileName);

        return data;
    }

    bool SaveFileData(const char *fileName, void *data, unsigned int bytesToWrite)
    {
        bool success = false;

        SDL_RWops *file = SDL_RWFromFile(fileName, "wb");
        if (file != NULL)
        {
            unsigned int count = (unsigned int)SDL_RWwrite(file, data, sizeof(unsigned char), bytesToWrite);
            if (count == 0)
                LogError("FILEIO: [%s] Failed to write file", fileName);

            else
                LogInfo("FILEIO: [%s] File saved successfully", fileName);

            int result = SDL_RWclose(file);
            if (result == 0)
                success = true;
        }
        else
            LogError("FILEIO: [%s] Failed to open file", fileName);

        return success;
    }

    char *LoadFileText(const char *fileName)
    {
        char *text = NULL;

        SDL_RWops *textFile = SDL_RWFromFile(fileName, "rt");
        if (textFile != NULL)
        {
            unsigned int size = (int)SDL_RWsize(textFile);
            if (size > 0)
            {
                text = (char *)SDL_malloc((size + 1) * sizeof(char));
                unsigned int count = (unsigned int)SDL_RWread(textFile, text, sizeof(char), size);
                if (count < size)
                    text = (char *)SDL_realloc(text, count + 1);
                text[count] = '\0';

                LogInfo("FILEIO: [%s] Text file loaded successfully", fileName);
            }
            else
                LogError("FILEIO: [%s] Failed to read text file", fileName);

            SDL_RWclose(textFile);
        }
        else
            LogError("FILEIO: [%s] Failed to open text file", fileName);

        return text;
    }

    bool SaveFileText(const char *fileName, char *text)
    {
        bool success = false;

        SDL_RWops *file = SDL_RWFromFile(fileName, "wt");
        if (file != NULL)
        {
            size_t strLen = SDL_strlen(text);
            int count = SDL_RWwrite(file, text, 1, strLen);
            if (count < 0)
                SDL_LogError(0, "FILEIO: [%s] Failed to write text file", fileName);
            else
                LogInfo("FILEIO: [%s] Text file saved successfully", fileName);

            int result = SDL_RWclose(file);
            if (result == 0)
                success = true;
        }
        else
            SDL_LogError(0, "FILEIO: [%s] Failed to open text file", fileName);

        return success;
    }

    bool ScanDirectoryFiles(const char *basePath, std::vector<std::string> &files, const char *filter)
    {
        static char path[512] = {0};
        SDL_memset(path, 0, 512);

        struct dirent *dp = NULL;
        DIR *dir = opendir(basePath);

        if (dir != NULL)
        {
            while ((dp = readdir(dir)) != NULL)
            {
                if ((strcmp(dp->d_name, ".") != 0) &&
                    (strcmp(dp->d_name, "..") != 0))
                {
                    sprintf(path, "%s/%s", basePath, dp->d_name);

                    if (filter != NULL)
                    {
                        if (IsFileExtension(path, filter))
                        {
                            files.push_back(path);
                        }
                    }
                    else
                    {
                        files.push_back(path);
                    }
                }
            }

            closedir(dir);
            return true;
        }

        LogError("FILEIO: Directory cannot be opened (%s)", basePath);
        return false;
    }

    // Scan all files and directories recursively from a base path
    bool ScanDirectoryFilesRecursively(const char *basePath, std::vector<std::string> &files, const char *filter)
    {
        char path[512] = {0};
        SDL_memset(path, 0, 512);

        struct dirent *dp = NULL;
        DIR *dir = opendir(basePath);

        if (dir != NULL)
        {
            while (((dp = readdir(dir)) != NULL))
            {
                if ((strcmp(dp->d_name, ".") != 0) && (strcmp(dp->d_name, "..") != 0))
                {
                    // Construct new path from our base path
                    sprintf(path, "%s/%s", basePath, dp->d_name);

                    if (IsPathFile(path))
                    {
                        if (filter != NULL)
                        {
                            if (IsFileExtension(path, filter))
                            {
                                files.push_back(path);
                            }
                        }
                        else
                        {
                            files.push_back(path);
                        }
                    }
                    else
                        ScanDirectoryFilesRecursively(path, files, filter);
                }
            }

            closedir(dir);
            return true;
        }
        LogError("FILEIO: Directory cannot be opened (%s)", basePath);
        return false;
    }

    const char *strprbrk(const char *s, const char *charset)
    {
        const char *latestMatch = NULL;
        for (; s = strpbrk(s, charset), s != NULL; latestMatch = s++)
        {
        }
        return latestMatch;
    }

    bool LoadDirectoryFiles(const char *dirPath, std::vector<std::string> &files)
    {

        DIR *dir = opendir(dirPath);

        if (dir != NULL) // It's a directory
        {
            closedir(dir);
            ScanDirectoryFiles(dirPath, files, NULL);
            return true;
        }
        LogError("FILEIO: Failed to open requested directory"); // Maybe it's a file...
        return false;
    }

    const char *TextFormat(const char *text, ...)
    {

#define MAX_TEXTFORMAT_BUFFERS 4 // Maximum number of static buffers for text formatting
        static char buffers[MAX_TEXTFORMAT_BUFFERS][MAX_TEXT_BUFFER_LENGTH] = {0};
        static int index = 0;
        char *currentBuffer = buffers[index];
        memset(currentBuffer, 0, MAX_TEXT_BUFFER_LENGTH); // Clear buffer before using
        va_list args;
        va_start(args, text);
        vsprintf(currentBuffer, text, args);
        va_end(args);
        index += 1; // Move to next buffer for next function call
        if (index >= MAX_TEXTFORMAT_BUFFERS)
            index = 0;

        return currentBuffer;
    }

    void UnloadFileData(void *data)
    {
        if (data)
        {
            SDL_free(data);
        }
    }

    signed long int GetFileModTime(const char *fileName)
    {
        struct stat fileStat;
        if (stat(fileName, &fileStat) == 0)
            return fileStat.st_mtime;
        return 0;
    }

    std::string TrimLeft(const std::string &str)
    {
        size_t start = 0;
        while (start < str.length() && std::isspace(str[start]))
            start++;
        return str.substr(start);
    }

    std::string TrimRight(const std::string &str)
    {
        size_t end = str.length();
        while (end > 0 && std::isspace(str[end - 1]))
            end--;
        return str.substr(0, end);
    }
}