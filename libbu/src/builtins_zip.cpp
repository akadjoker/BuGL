#include "interpreter.hpp"
#include "platform.hpp"

#ifdef BU_ENABLE_ZIP

#include "miniz.h"

#include <cerrno>
#include <cctype>
#include <climits>
#include <cstring>
#include <string>

#ifdef _WIN32
#include <direct.h>
#include <sys/stat.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

static bool zipIsSeparator(char c)
{
    return c == '/' || c == '\\';
}

static bool zipIsDirectoryPath(const std::string &path)
{
    struct stat st;
    if (stat(path.c_str(), &st) != 0)
    {
        return false;
    }
    return S_ISDIR(st.st_mode) != 0;
}

static bool zipCreateDirSingle(const std::string &path)
{
    if (path.empty() || path == "/")
    {
        return true;
    }

    if (zipIsDirectoryPath(path))
    {
        return true;
    }

#ifdef _WIN32
    if (_mkdir(path.c_str()) == 0 || errno == EEXIST)
    {
        return true;
    }
#else
    if (mkdir(path.c_str(), 0755) == 0 || errno == EEXIST)
    {
        return true;
    }
#endif
    return zipIsDirectoryPath(path);
}

static bool zipEnsureDirRecursive(const std::string &inputPath)
{
    if (inputPath.empty())
    {
        return true;
    }

    std::string path = inputPath;
    for (size_t i = 0; i < path.size(); i++)
    {
        if (path[i] == '\\')
        {
            path[i] = '/';
        }
    }

    size_t start = 0;
    if (path.size() >= 2 && std::isalpha((unsigned char)path[0]) && path[1] == ':')
    {
        start = 2;
    }
    if (!path.empty() && path[0] == '/')
    {
        start = 1;
    }

    for (size_t i = start; i <= path.size(); i++)
    {
        if (i != path.size() && path[i] != '/')
        {
            continue;
        }

        std::string current = path.substr(0, i);
        if (current.empty() || current == "/" || (current.size() == 2 && current[1] == ':'))
        {
            continue;
        }

        if (!zipCreateDirSingle(current))
        {
            return false;
        }
    }

    return true;
}

static std::string zipParentPath(const std::string &path)
{
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos)
    {
        return "";
    }
    return path.substr(0, pos);
}

static std::string zipBaseName(const std::string &path)
{
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos)
    {
        return path;
    }
    return path.substr(pos + 1);
}

static bool zipIsUnsafeEntryName(const char *entryName)
{
    if (!entryName || entryName[0] == '\0')
    {
        return true;
    }

    std::string name(entryName);

    if (name[0] == '/' || name[0] == '\\')
    {
        return true;
    }
    if (name.size() >= 2 && std::isalpha((unsigned char)name[0]) && name[1] == ':')
    {
        return true;
    }

    std::string component;
    for (size_t i = 0; i <= name.size(); i++)
    {
        if (i < name.size() && !zipIsSeparator(name[i]))
        {
            component += name[i];
            continue;
        }

        if (component == "..")
        {
            return true;
        }
        component.clear();
    }

    return false;
}

static std::string zipNormalizeEntryName(const char *entryName)
{
    std::string result(entryName ? entryName : "");
    for (size_t i = 0; i < result.size(); i++)
    {
        if (result[i] == '\\')
        {
            result[i] = '/';
        }
    }

    while (!result.empty() && result[0] == '/')
    {
        result.erase(result.begin());
    }

    return result;
}

int native_zip_list(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isString())
    {
        vm->runtimeError("zip.list expects (archivePath)");
        return 0;
    }

    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));

    if (!mz_zip_reader_init_file(&zip, args[0].asStringChars(), 0))
    {
        vm->pushNil();
        return 1;
    }

    Value out = vm->makeArray();
    ArrayInstance *arr = out.asArray();

    mz_uint numFiles = mz_zip_reader_get_num_files(&zip);
    for (mz_uint i = 0; i < numFiles; i++)
    {
        mz_zip_archive_file_stat st;
        if (!mz_zip_reader_file_stat(&zip, i, &st))
        {
            continue;
        }
        arr->values.push(vm->makeString(st.m_filename));
    }

    mz_zip_reader_end(&zip);
    vm->push(out);
    return 1;
}

int native_zip_read(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 2 || !args[0].isString() || !args[1].isString())
    {
        vm->runtimeError("zip.read expects (archivePath, entryName)");
        return 0;
    }

    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));

    if (!mz_zip_reader_init_file(&zip, args[0].asStringChars(), 0))
    {
        vm->pushNil();
        return 1;
    }

    size_t size = 0;
    void *data = mz_zip_reader_extract_file_to_heap(
        &zip, args[1].asStringChars(), &size, 0);

    if (!data)
    {
        mz_zip_reader_end(&zip);
        vm->pushNil();
        return 1;
    }

    std::string text((const char *)data, size);
    mz_free(data);
    mz_zip_reader_end(&zip);

    vm->push(vm->makeString(text.c_str()));
    return 1;
}

int native_zip_read_buffer(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 2 || !args[0].isString() || !args[1].isString())
    {
        vm->runtimeError("zip.read_buffer expects (archivePath, entryName)");
        return 0;
    }

    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));

    if (!mz_zip_reader_init_file(&zip, args[0].asStringChars(), 0))
    {
        vm->pushNil();
        return 1;
    }

    size_t size = 0;
    void *data = mz_zip_reader_extract_file_to_heap(
        &zip, args[1].asStringChars(), &size, 0);

    if (!data)
    {
        mz_zip_reader_end(&zip);
        vm->pushNil();
        return 1;
    }

    if (size > (size_t)INT_MAX)
    {
        mz_free(data);
        mz_zip_reader_end(&zip);
        vm->runtimeError("zip.read_buffer: entry too large");
        return 0;
    }

    Value bufferValue = vm->makeBuffer((int)size, 0); // UINT8
    BufferInstance *buf = bufferValue.asBuffer();
    if (!buf || !buf->data)
    {
        mz_free(data);
        mz_zip_reader_end(&zip);
        vm->pushNil();
        return 1;
    }

    if (size > 0)
    {
        memcpy(buf->data, data, size);
    }
    buf->cursor = 0;

    mz_free(data);
    mz_zip_reader_end(&zip);

    vm->push(bufferValue);
    return 1;
}

int native_zip_extract(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 2 || !args[0].isString() || !args[1].isString())
    {
        vm->runtimeError("zip.extract expects (archivePath, outputDir)");
        return 0;
    }

    std::string outDir = args[1].asStringChars();
    if (!zipEnsureDirRecursive(outDir))
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));

    if (!mz_zip_reader_init_file(&zip, args[0].asStringChars(), 0))
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    bool ok = true;
    mz_uint numFiles = mz_zip_reader_get_num_files(&zip);

    for (mz_uint i = 0; i < numFiles && ok; i++)
    {
        mz_zip_archive_file_stat st;
        if (!mz_zip_reader_file_stat(&zip, i, &st))
        {
            ok = false;
            break;
        }

        if (zipIsUnsafeEntryName(st.m_filename))
        {
            ok = false;
            break;
        }

        std::string entry = zipNormalizeEntryName(st.m_filename);
        std::string dstPath = outDir;
        if (!dstPath.empty() && !zipIsSeparator(dstPath[dstPath.size() - 1]))
        {
            dstPath += "/";
        }
        dstPath += entry;

        if (st.m_is_directory)
        {
            if (!zipEnsureDirRecursive(dstPath))
            {
                ok = false;
                break;
            }
            continue;
        }

        if (!zipEnsureDirRecursive(zipParentPath(dstPath)))
        {
            ok = false;
            break;
        }

        if (!mz_zip_reader_extract_to_file(&zip, i, dstPath.c_str(), 0))
        {
            ok = false;
            break;
        }
    }

    mz_zip_reader_end(&zip);
    vm->push(vm->makeBool(ok));
    return 1;
}

int native_zip_create(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 2 || !args[0].isString() || !args[1].isArray())
    {
        vm->runtimeError("zip.create expects (archivePath, filesArray, [level])");
        return 0;
    }

    int level = 6;
    if (argCount >= 3 && args[2].isInt())
    {
        level = args[2].asInt();
    }
    if (level < 0)
    {
        level = 0;
    }
    if (level > 10)
    {
        level = 10;
    }

    ArrayInstance *files = args[1].asArray();

    mz_zip_archive zip;
    memset(&zip, 0, sizeof(zip));

    if (!mz_zip_writer_init_file(&zip, args[0].asStringChars(), 0))
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    bool ok = true;
    for (size_t i = 0; i < files->values.size(); i++)
    {
        Value item = files->values[i];
        if (!item.isString())
        {
            ok = false;
            break;
        }

        std::string sourcePath = item.asStringChars();
        std::string archiveName = zipBaseName(sourcePath);

        if (archiveName.empty() || !OsFileExists(sourcePath.c_str()))
        {
            ok = false;
            break;
        }

        if (!mz_zip_writer_add_file(
                &zip, archiveName.c_str(), sourcePath.c_str(),
                NULL, 0, (mz_uint)level))
        {
            ok = false;
            break;
        }
    }

    if (ok && !mz_zip_writer_finalize_archive(&zip))
    {
        ok = false;
    }

    mz_zip_writer_end(&zip);

    vm->push(vm->makeBool(ok));
    return 1;
}

void Interpreter::registerZip()
{
    addModule("zip")
        .addFunction("list", native_zip_list, 1)
        .addFunction("read", native_zip_read, 2)
        .addFunction("read_buffer", native_zip_read_buffer, 2)
        .addFunction("extract", native_zip_extract, 2)
        .addFunction("create", native_zip_create, -1);
}

#endif
