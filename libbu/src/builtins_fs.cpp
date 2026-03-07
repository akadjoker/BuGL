#include "interpreter.hpp"

#ifdef BU_ENABLE_FILE_IO

#include "platform.hpp"
#include "utils.hpp"
#include <string>
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

int native_fs_read(Interpreter *vm, int argCount, Value *args)
{
    if (!args[0].isString())
    {
        vm->push(vm->makeNil());
        return 1;
    }
     
    const char *path = args[0].asStringChars();
    int size = OsFileSize(path);

    if (size < 0)
    {
        vm->push(vm->makeNil());
        return 1;
    }

    char *buffer = (char *)malloc(size + 1);
    int bytesRead = OsFileRead(path, buffer, size);

    if (bytesRead < 0)
    {
        free(buffer);
         vm->push(vm->makeNil());
        return 1;
    }

    buffer[bytesRead] = '\0';
    vm->push(vm->makeString(buffer));
    free(buffer);

    return 1;
}

int native_fs_write(Interpreter *vm, int argCount, Value *args)
{
    if (!args[0].isString() || !args[1].isString())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    const char *path = args[0].asStringChars();
    const char *data = args[1].asStringChars();

    vm->push(vm->makeBool(OsFileWrite(path, data, strlen(data)) > 0));
    return 1;
}

int native_fs_list(Interpreter *vm, int argCount, Value *args)
{
    if (!args[0].isString())
        return 0;

    Value arr = vm->makeArray();
    const char *dirPath = args[0].asStringChars();

#ifdef _WIN32
    WIN32_FIND_DATAA findData;
    std::string searchPath = std::string(dirPath) + "\\*";
    HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (strcmp(findData.cFileName, ".") != 0 &&
                strcmp(findData.cFileName, "..") != 0)
            {
                arr.asArray()->values.push(vm->makeString(findData.cFileName));
            }
        } while (FindNextFileA(hFind, &findData));
        FindClose(hFind);
    }
#else
    DIR *dir = opendir(dirPath);
    if (dir)
    {
        struct dirent *entry;
        while ((entry = readdir(dir)) != nullptr)
        {
            if (strcmp(entry->d_name, ".") != 0 &&
                strcmp(entry->d_name, "..") != 0)
            {
                arr.asArray()->values.push(vm->makeString(entry->d_name));
            }
        }
        closedir(dir);
    }
#endif

    vm->push(arr);
    return 1;
}

int native_fs_mkdir(Interpreter *vm, int argCount, Value *args)
{
    if (!args[0].isString())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    const char *path = args[0].asStringChars();

#ifdef _WIN32
    vm->push(vm->makeBool(CreateDirectoryA(path, NULL) != 0 ||
                          GetLastError() == ERROR_ALREADY_EXISTS));
#else
    vm->push(vm->makeBool(mkdir(path, 0755) == 0 || errno == EEXIST));
#endif
    return 1;
}

int native_fs_rmdir(Interpreter *vm, int argCount, Value *args)
{
    if (!args[0].isString())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    const char *path = args[0].asStringChars();

#ifdef _WIN32
    vm->push(vm->makeBool(RemoveDirectoryA(path) != 0));
#else
    vm->push(vm->makeBool(rmdir(path) == 0));
#endif
    return 1;
}

int native_fs_remove(Interpreter *vm, int argCount, Value *args)
{
    if (!args[0].isString())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    const char *path = args[0].asStringChars();

#ifdef _WIN32
    vm->push(vm->makeBool(DeleteFileA(path) != 0));
#else
    vm->push(vm->makeBool(remove(path) == 0));
#endif
    return 1;
}

int native_fs_append(Interpreter *vm, int argCount, Value *args)
{
    if (!args[0].isString() || !args[1].isString())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    const char *path = args[0].asStringChars();
    const char *data = args[1].asStringChars();

#ifdef _WIN32
    HANDLE hFile = CreateFileA(path, FILE_APPEND_DATA, 0, NULL,
                               OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    DWORD written;
    BOOL result = WriteFile(hFile, data, strlen(data), &written, NULL);
    CloseHandle(hFile);
    vm->push(vm->makeBool(result != 0));
    return 1;
#else
    FILE *f = fopen(path, "a");
    if (!f)
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    size_t written = fwrite(data, 1, strlen(data), f);
    fclose(f);
    vm->push(vm->makeBool(written == strlen(data)));
    return 1;
#endif
}

int native_fs_stat(Interpreter *vm, int argCount, Value *args)
{
    if (!args[0].isString())
    {
        vm->push(vm->makeNil());
        return 1;
    }

    Value map = vm->makeMap();
    MapInstance *m = map.asMap();

    const char *path = args[0].asStringChars();

#ifdef _WIN32
    WIN32_FILE_ATTRIBUTE_DATA fileInfo;
    if (!GetFileAttributesExA(path, GetFileExInfoStandard, &fileInfo))
    {
        vm->push(vm->makeNil());
        return 1;
    }

    LARGE_INTEGER size;
    size.LowPart = fileInfo.nFileSizeLow;
    size.HighPart = fileInfo.nFileSizeHigh;

    m->table.set(vm->makeString("size"), vm->makeInt((int)size.QuadPart));
    m->table.set(vm->makeString("isdir"),
                 vm->makeBool(fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY));
    m->table.set(vm->makeString("isfile"),
                 vm->makeBool(!(fileInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)));

#else
    struct stat st;
    if (stat(path, &st) != 0)
    {

        vm->push(vm->makeNil());
        return 1;
    }

    m->table.set(vm->makeString("size").asString(), vm->makeInt((int)st.st_size));
    m->table.set(vm->makeString("isdir").asString(), vm->makeBool(S_ISDIR(st.st_mode)));
    m->table.set(vm->makeString("isfile").asString(), vm->makeBool(S_ISREG(st.st_mode)));
    m->table.set(vm->makeString("mode").asString(), vm->makeInt(st.st_mode));
    m->table.set(vm->makeString("mtime").asString(), vm->makeInt((int)st.st_mtime));
#endif

    vm->push(map);
    return 1;
}

void Interpreter::registerFS()
{
    addModule("fs")
        .addFunction("read", native_fs_read, 1)
        .addFunction("write", native_fs_write, 2)
        .addFunction("append", native_fs_append, 2)
        .addFunction("remove", native_fs_remove, 1)
        .addFunction("mkdir", native_fs_mkdir, 1)
        .addFunction("rmdir", native_fs_rmdir, 1)
        .addFunction("list", native_fs_list, 1)
        .addFunction("stat", native_fs_stat, 1);
}

#endif