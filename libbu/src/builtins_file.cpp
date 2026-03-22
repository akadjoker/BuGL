#include "interpreter.hpp"

#ifdef BU_ENABLE_FILE_IO

#include "platform.hpp"
#include "utils.hpp"
#include <string>
#include <vector>
#include <cstring>

// ============================================
// FILE MODULE - COMPLETO (ADAPTADO)
// ============================================

enum class FileMode
{
    READ,
    WRITE,
    READ_WRITE
};

struct FileBuffer
{
    std::vector<uint8_t> data;
    size_t cursor;
    std::string path;
    FileMode mode;
    bool modified;
};

static std::vector<FileBuffer *> openFiles;
static int nextFileId = 1;

static FileBuffer *get_open_file(int id)
{
    if (id <= 0 || id > (int)openFiles.size())
        return nullptr;
    return openFiles[id - 1];
}

static bool flush_file_buffer(FileBuffer *fb)
{
    if (!fb)
        return false;

    const size_t size = fb->data.size();
    const void *bytes = (size > 0) ? (const void *)fb->data.data() : (const void *)"";
    int written = OsFileWrite(fb->path.c_str(), bytes, size);
    return written >= 0 && (size_t)written == size;
}

// ============================================
// CLEANUP
// ============================================

static void FileModuleCleanup()
{
    for (auto fb : openFiles)
    {
        if (fb)
        {
            if (fb->modified && fb->mode != FileMode::READ)
                flush_file_buffer(fb);
            delete fb;
        }
    }
    openFiles.clear();
}

// ============================================
// EXISTS - Verificar se arquivo existe
// ============================================

int native_file_exists(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isString())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    const char *path = args[0].asStringChars();
    int fileSize = OsFileSize(path);
    bool exists = (fileSize >= 0);
    
    vm->push(vm->makeBool(exists));
    return 1;
}

// ============================================
// OPEN
// ============================================

int native_file_open(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isString())
    {
        vm->runtimeError("file.open expects (path, mode?)");
        return 0;
    }

    const char *path = args[0].asStringChars();
    const char *modeStr = "r";

    if (argCount >= 2 && args[1].isString())
        modeStr = args[1].asStringChars();

    FileMode mode;
    if (strcmp(modeStr, "r") == 0)
        mode = FileMode::READ;
    else if (strcmp(modeStr, "w") == 0)
        mode = FileMode::WRITE;
    else if (strcmp(modeStr, "rw") == 0)
        mode = FileMode::READ_WRITE;
    else
    {
        vm->runtimeError("Invalid mode '%s'. Use 'r', 'w', or 'rw'", modeStr);
        return 0;
    }

    FileBuffer *fb = new FileBuffer();
    fb->path = path;
    fb->cursor = 0;
    fb->mode = mode;
    fb->modified = (mode == FileMode::WRITE);

    if (mode == FileMode::READ || mode == FileMode::READ_WRITE)
    {
        int fileSize = OsFileSize(path);

        if (fileSize >= 0)
        {
            if (fileSize > 0)
            {
                fb->data.resize(fileSize);
            }
            int bytesRead = (fileSize > 0) ? OsFileRead(path, fb->data.data(), fileSize) : 0;

            if (bytesRead < 0)
            {
                delete fb;
                vm->runtimeError("Failed to read file '%s'", path);
                return 0;
            }

            fb->data.resize(bytesRead);
        }
        else if (mode == FileMode::READ)
        {
            delete fb;
            vm->runtimeError("File '%s' does not exist", path);
            return 0;
        }
    }

    openFiles.push_back(fb);
    vm->push(vm->makeInt(nextFileId++));
    return 1;
}

// ============================================
// SAVE
// ============================================

int native_file_save(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isInt())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    int id = args[0].asInt();
    if (id <= 0 || id > (int)openFiles.size() || !openFiles[id - 1])
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    FileBuffer *fb = openFiles[id - 1];

    if (fb->mode == FileMode::READ)
    {
        vm->runtimeError("Cannot save file opened in read mode");
        vm->push(vm->makeBool(false));
        return 1;
    }

    if (!flush_file_buffer(fb))
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    fb->modified = false;
    vm->push(vm->makeBool(true));
    return 1;
}

// ============================================
// CLOSE
// ============================================

int native_file_close(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isInt())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    int id = args[0].asInt();
    if (id <= 0 || id > (int)openFiles.size() || !openFiles[id - 1])
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    FileBuffer *fb = openFiles[id - 1];

    if (fb->modified && fb->mode != FileMode::READ)
    {
        flush_file_buffer(fb);
    }

    delete fb;
    openFiles[id - 1] = nullptr;

    vm->push(vm->makeBool(true));
    return 1;
}

// ============================================
// WRITE BYTE
// ============================================

int native_file_write_byte(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 2 || !args[0].isInt() || !args[1].isNumber())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    int id = args[0].asInt();
    if (id <= 0 || id > (int)openFiles.size() || !openFiles[id - 1])
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    FileBuffer *fb = openFiles[id - 1];

    if (fb->mode == FileMode::READ)
    {
        vm->runtimeError("Cannot write to file opened in read mode");
        vm->push(vm->makeBool(false));
        return 1;
    }

    if (fb->cursor >= fb->data.size())
        fb->data.resize(fb->cursor + 1);

    fb->data[fb->cursor++] = (uint8_t)args[1].asNumber();
    fb->modified = true;

    vm->push(vm->makeBool(true));
    return 1;
}

// ============================================
// WRITE SHORT
// ============================================

int native_file_write_short(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 2 || !args[0].isInt() || !args[1].isNumber())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    int id = args[0].asInt();
    if (id <= 0 || id > (int)openFiles.size() || !openFiles[id - 1])
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    FileBuffer *fb = openFiles[id - 1];

    if (fb->mode == FileMode::READ)
    {
        vm->runtimeError("Cannot write to file opened in read mode");
        vm->push(vm->makeBool(false));
        return 1;
    }

    if (fb->cursor + sizeof(int16_t) > fb->data.size())
        fb->data.resize(fb->cursor + sizeof(int16_t));

    int16_t value = (int16_t)args[1].asNumber();
    memcpy(fb->data.data() + fb->cursor, &value, sizeof(int16_t));
    fb->cursor += sizeof(int16_t);
    fb->modified = true;

    vm->push(vm->makeBool(true));
    return 1;
}

// ============================================
// WRITE USHORT
// ============================================

int native_file_write_ushort(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 2 || !args[0].isInt() || !args[1].isNumber())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    int id = args[0].asInt();
    if (id <= 0 || id > (int)openFiles.size() || !openFiles[id - 1])
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    FileBuffer *fb = openFiles[id - 1];

    if (fb->mode == FileMode::READ)
    {
        vm->runtimeError("Cannot write to file opened in read mode");
        vm->push(vm->makeBool(false));
        return 1;
    }

    if (fb->cursor + sizeof(uint16_t) > fb->data.size())
        fb->data.resize(fb->cursor + sizeof(uint16_t));

    uint16_t value = (uint16_t)args[1].asNumber();
    memcpy(fb->data.data() + fb->cursor, &value, sizeof(uint16_t));
    fb->cursor += sizeof(uint16_t);
    fb->modified = true;

    vm->push(vm->makeBool(true));
    return 1;
}

// ============================================
// WRITE INT
// ============================================

int native_file_write_uint(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 2 || !args[0].isInt() || !args[1].isNumber())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    int id = args[0].asInt();
    if (id <= 0 || id > (int)openFiles.size() || !openFiles[id - 1])
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    FileBuffer *fb = openFiles[id - 1];

    if (fb->mode == FileMode::READ)
    {
        vm->runtimeError("Cannot write to file opened in read mode");
        vm->push(vm->makeBool(false));
        return 1;
    }

    if (fb->cursor + sizeof(uint32_t) > fb->data.size())
        fb->data.resize(fb->cursor + sizeof(uint32_t));

    uint32_t value = (uint32_t)args[1].asNumber();
    memcpy(fb->data.data() + fb->cursor, &value, sizeof(uint32_t));
    fb->cursor += sizeof(uint32_t);
    fb->modified = true;

    vm->push(vm->makeBool(true));
    return 1;
}

// ============================================
// WRITE INT
// ============================================

int native_file_write_int(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 2 || !args[0].isInt() || !args[1].isNumber())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    int id = args[0].asInt();
    if (id <= 0 || id > (int)openFiles.size() || !openFiles[id - 1])
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    FileBuffer *fb = openFiles[id - 1];

    if (fb->mode == FileMode::READ)
    {
        vm->runtimeError("Cannot write to file opened in read mode");
        vm->push(vm->makeBool(false));
        return 1;
    }

    if (fb->cursor + sizeof(int32_t) > fb->data.size())
        fb->data.resize(fb->cursor + sizeof(int32_t));

    int32_t value = (int32_t)args[1].asNumber();
    memcpy(fb->data.data() + fb->cursor, &value, sizeof(int32_t));
    fb->cursor += sizeof(int32_t);
    fb->modified = true;

    vm->push(vm->makeBool(true));
    return 1;
}

// ============================================
// WRITE FLOAT
// ============================================

int native_file_write_float(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 2 || !args[0].isInt() || !args[1].isNumber())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    int id = args[0].asInt();
    if (id <= 0 || id > (int)openFiles.size() || !openFiles[id - 1])
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    FileBuffer *fb = openFiles[id - 1];

    if (fb->mode == FileMode::READ)
    {
        vm->runtimeError("Cannot write to file opened in read mode");
        vm->push(vm->makeBool(false));
        return 1;
    }

    if (fb->cursor + sizeof(float) > fb->data.size())
        fb->data.resize(fb->cursor + sizeof(float));

    float value = (float)args[1].asNumber();
    memcpy(fb->data.data() + fb->cursor, &value, sizeof(float));
    fb->cursor += sizeof(float);
    fb->modified = true;

    vm->push(vm->makeBool(true));
    return 1;
}

// ============================================
// WRITE DOUBLE
// ============================================

int native_file_write_double(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 2 || !args[0].isInt() || !args[1].isNumber())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    int id = args[0].asInt();
    if (id <= 0 || id > (int)openFiles.size() || !openFiles[id - 1])
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    FileBuffer *fb = openFiles[id - 1];

    if (fb->mode == FileMode::READ)
    {
        vm->runtimeError("Cannot write to file opened in read mode");
        vm->push(vm->makeBool(false));
        return 1;
    }

    if (fb->cursor + sizeof(double) > fb->data.size())
        fb->data.resize(fb->cursor + sizeof(double));

    double value = args[1].asNumber();
    memcpy(fb->data.data() + fb->cursor, &value, sizeof(double));
    fb->cursor += sizeof(double);
    fb->modified = true;

    vm->push(vm->makeBool(true));
    return 1;
}

// ============================================
// WRITE BOOL
// ============================================

int native_file_write_bool(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 2 || !args[0].isInt() || !args[1].isBool())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    int id = args[0].asInt();
    if (id <= 0 || id > (int)openFiles.size() || !openFiles[id - 1])
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    FileBuffer *fb = openFiles[id - 1];

    if (fb->mode == FileMode::READ)
    {
        vm->runtimeError("Cannot write to file opened in read mode");
        vm->push(vm->makeBool(false));
        return 1;
    }

    if (fb->cursor >= fb->data.size())
        fb->data.resize(fb->cursor + 1);

    fb->data[fb->cursor++] = args[1].asBool() ? 1 : 0;
    fb->modified = true;

    vm->push(vm->makeBool(true));
    return 1;
}

// ============================================
// WRITE STRING
// ============================================

int native_file_write_string(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 2 || !args[0].isInt() || !args[1].isString())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    int id = args[0].asInt();
    if (id <= 0 || id > (int)openFiles.size() || !openFiles[id - 1])
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    FileBuffer *fb = openFiles[id - 1];

    if (fb->mode == FileMode::READ)
    {
        vm->runtimeError("Cannot write to file opened in read mode");
        vm->push(vm->makeBool(false));
        return 1;
    }

    const char *str = args[1].asStringChars();
    int len = args[1].asString()->length();

    size_t needed = sizeof(int32_t) + len;
    if (fb->cursor + needed > fb->data.size())
        fb->data.resize(fb->cursor + needed);

    int32_t size = len;
    memcpy(fb->data.data() + fb->cursor, &size, sizeof(int32_t));
    fb->cursor += sizeof(int32_t);

    memcpy(fb->data.data() + fb->cursor, str, len);
    fb->cursor += len;
    fb->modified = true;

    vm->push(vm->makeBool(true));
    return 1;
}

// ============================================
// READ BYTE
// ============================================

int native_file_read_byte(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isInt())
    {
        vm->push(vm->makeInt(0));
        return 1;
    }

    int id = args[0].asInt();
    if (id <= 0 || id > (int)openFiles.size() || !openFiles[id - 1])
    {
        vm->push(vm->makeInt(0));
        return 1;
    }

    FileBuffer *fb = openFiles[id - 1];

    if (fb->cursor >= fb->data.size())
    {
        vm->push(vm->makeInt(0));
        return 1;
    }

    uint8_t value = fb->data[fb->cursor++];
    vm->push(vm->makeInt(value));
    return 1;
}

// ============================================
// READ SHORT
// ============================================

int native_file_read_short(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isInt())
    {
        vm->push(vm->makeInt(0));
        return 1;
    }

    int id = args[0].asInt();
    if (id <= 0 || id > (int)openFiles.size() || !openFiles[id - 1])
    {
        vm->push(vm->makeInt(0));
        return 1;
    }

    FileBuffer *fb = openFiles[id - 1];

    if (fb->cursor + sizeof(int16_t) > fb->data.size())
    {
        vm->push(vm->makeInt(0));
        return 1;
    }

    int16_t value;
    memcpy(&value, fb->data.data() + fb->cursor, sizeof(int16_t));
    fb->cursor += sizeof(int16_t);

    vm->push(vm->makeInt(value));
    return 1;
}

// ============================================
// READ USHORT
// ============================================

int native_file_read_ushort(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isInt())
    {
        vm->push(vm->makeInt(0));
        return 1;
    }

    int id = args[0].asInt();
    if (id <= 0 || id > (int)openFiles.size() || !openFiles[id - 1])
    {
        vm->push(vm->makeInt(0));
        return 1;
    }

    FileBuffer *fb = openFiles[id - 1];

    if (fb->cursor + sizeof(uint16_t) > fb->data.size())
    {
        vm->push(vm->makeInt(0));
        return 1;
    }

    uint16_t value;
    memcpy(&value, fb->data.data() + fb->cursor, sizeof(uint16_t));
    fb->cursor += sizeof(uint16_t);

    vm->push(vm->makeInt((int)value));
    return 1;
}

// ============================================
// READ UINT
// ============================================

int native_file_read_uint(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isInt())
    {
        vm->push(vm->makeDouble(0));
        return 1;
    }

    int id = args[0].asInt();
    if (id <= 0 || id > (int)openFiles.size() || !openFiles[id - 1])
    {
        vm->push(vm->makeDouble(0));
        return 1;
    }

    FileBuffer *fb = openFiles[id - 1];

    if (fb->cursor + sizeof(uint32_t) > fb->data.size())
    {
        vm->push(vm->makeDouble(0));
        return 1;
    }

    uint32_t value;
    memcpy(&value, fb->data.data() + fb->cursor, sizeof(uint32_t));
    fb->cursor += sizeof(uint32_t);

    vm->push(vm->makeDouble((double)value));
    return 1;
}

// ============================================
// READ INT
// ============================================

int native_file_read_int(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isInt())
    {
        vm->push(vm->makeInt(0));
        return 1;
    }

    int id = args[0].asInt();
    if (id <= 0 || id > (int)openFiles.size() || !openFiles[id - 1])
    {
        vm->push(vm->makeInt(0));
        return 1;
    }

    FileBuffer *fb = openFiles[id - 1];

    if (fb->cursor + sizeof(int32_t) > fb->data.size())
    {
        vm->push(vm->makeInt(0));
        return 1;
    }

    int32_t value;
    memcpy(&value, fb->data.data() + fb->cursor, sizeof(int32_t));
    fb->cursor += sizeof(int32_t);

    vm->push(vm->makeInt(value));
    return 1;
}

// ============================================
// READ FLOAT
// ============================================

int native_file_read_float(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isInt())
    {
        vm->push(vm->makeDouble(0));
        return 1;
    }

    int id = args[0].asInt();
    if (id <= 0 || id > (int)openFiles.size() || !openFiles[id - 1])
    {
        vm->push(vm->makeDouble(0));
        return 1;
    }

    FileBuffer *fb = openFiles[id - 1];

    if (fb->cursor + sizeof(float) > fb->data.size())
    {
        vm->push(vm->makeDouble(0));
        return 1;
    }

    float value;
    memcpy(&value, fb->data.data() + fb->cursor, sizeof(float));
    fb->cursor += sizeof(float);

    vm->push(vm->makeDouble(value));
    return 1;
}

// ============================================
// READ DOUBLE
// ============================================

int native_file_read_double(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isInt())
    {
        vm->push(vm->makeDouble(0));
        return 1;
    }

    int id = args[0].asInt();
    if (id <= 0 || id > (int)openFiles.size() || !openFiles[id - 1])
    {
        vm->push(vm->makeDouble(0));
        return 1;
    }

    FileBuffer *fb = openFiles[id - 1];

    if (fb->cursor + sizeof(double) > fb->data.size())
    {
        vm->push(vm->makeDouble(0));
        return 1;
    }

    double value;
    memcpy(&value, fb->data.data() + fb->cursor, sizeof(double));
    fb->cursor += sizeof(double);

    vm->push(vm->makeDouble(value));
    return 1;
}

// ============================================
// READ BOOL
// ============================================

int native_file_read_bool(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isInt())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    int id = args[0].asInt();
    if (id <= 0 || id > (int)openFiles.size() || !openFiles[id - 1])
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    FileBuffer *fb = openFiles[id - 1];

    if (fb->cursor >= fb->data.size())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    uint8_t value = fb->data[fb->cursor++];
    vm->push(vm->makeBool(value != 0));
    return 1;
}

// ============================================
// READ STRING
// ============================================

int native_file_read_string(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isInt())
    {
        vm->push(vm->makeNil());
        return 1;
    }

    int id = args[0].asInt();
    if (id <= 0 || id > (int)openFiles.size() || !openFiles[id - 1])
    {
        vm->push(vm->makeNil());
        return 1;
    }

    FileBuffer *fb = openFiles[id - 1];

    if (fb->cursor + sizeof(int32_t) > fb->data.size())
    {
        vm->push(vm->makeNil());
        return 1;
    }

    int32_t len;
    memcpy(&len, fb->data.data() + fb->cursor, sizeof(int32_t));
    fb->cursor += sizeof(int32_t);

    if (len < 0 || fb->cursor + len > fb->data.size())
    {
        vm->push(vm->makeNil());
        return 1;
    }

    std::string str((char *)(fb->data.data() + fb->cursor), len);
    fb->cursor += len;

    vm->push(vm->makeString(str.c_str()));
    return 1;
}

// ============================================
// READ BYTES -> BUFFER(UINT8)
// ============================================

int native_file_read_bytes(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 2 || !args[0].isInt() || !args[1].isInt())
    {
        vm->push(vm->makeNil());
        return 1;
    }

    int id = args[0].asInt();
    int requested = args[1].asInt();
    FileBuffer *fb = get_open_file(id);
    if (!fb)
    {
        vm->push(vm->makeNil());
        return 1;
    }

    if (requested < 0)
    {
        vm->runtimeError("file.read_bytes count must be >= 0");
        return 0;
    }

    const int remaining = (int)fb->data.size() - (int)fb->cursor;
    const int count = (requested < remaining) ? requested : remaining;

    Value out = vm->makeBuffer(count, (int)BufferType::UINT8);
    BufferInstance *buf = out.asBuffer();
    if (!buf)
    {
        vm->push(vm->makeNil());
        return 1;
    }

    if (count > 0)
    {
        memcpy(buf->data, fb->data.data() + fb->cursor, (size_t)count);
        fb->cursor += (size_t)count;
    }

    vm->push(out);
    return 1;
}

// ============================================
// READ ALL REMAINING BYTES -> BUFFER(UINT8)
// ============================================

int native_file_read_all(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isInt())
    {
        vm->push(vm->makeNil());
        return 1;
    }

    int id = args[0].asInt();
    FileBuffer *fb = get_open_file(id);
    if (!fb)
    {
        vm->push(vm->makeNil());
        return 1;
    }

    const int remaining = (int)fb->data.size() - (int)fb->cursor;
    Value out = vm->makeBuffer(remaining, (int)BufferType::UINT8);
    BufferInstance *buf = out.asBuffer();
    if (!buf)
    {
        vm->push(vm->makeNil());
        return 1;
    }

    if (remaining > 0)
    {
        memcpy(buf->data, fb->data.data() + fb->cursor, (size_t)remaining);
        fb->cursor += (size_t)remaining;
    }

    vm->push(out);
    return 1;
}

// ============================================
// WRITE BUFFER
// ============================================

int native_file_write_buffer(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 2 || !args[0].isInt() || !args[1].isBuffer())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    int id = args[0].asInt();
    FileBuffer *fb = get_open_file(id);
    if (!fb)
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    if (fb->mode == FileMode::READ)
    {
        vm->runtimeError("Cannot write to file opened in read mode");
        vm->push(vm->makeBool(false));
        return 1;
    }

    BufferInstance *src = args[1].asBuffer();
    if (!src || src->count < 0)
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    const size_t byteCount = (size_t)src->count * (size_t)src->elementSize;
    const size_t needed = fb->cursor + byteCount;
    if (needed > fb->data.size())
        fb->data.resize(needed);

    if (byteCount > 0)
        memcpy(fb->data.data() + fb->cursor, src->data, byteCount);

    fb->cursor += byteCount;
    fb->modified = true;
    vm->push(vm->makeBool(true));
    return 1;
}

// ============================================
// SEEK
// ============================================

int native_file_seek(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 2 || !args[0].isInt() || !args[1].isInt())
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    int id = args[0].asInt();
    int pos = args[1].asInt();

    if (id <= 0 || id > (int)openFiles.size() || !openFiles[id - 1])
    {
        vm->push(vm->makeBool(false));
        return 1;
    }

    FileBuffer *fb = openFiles[id - 1];

    if (pos < 0 || pos > (int)fb->data.size())
    {
        vm->push(vm->makeBool(false));
    }
    else
    {
        fb->cursor = pos;
        vm->push(vm->makeBool(true));
    }
    return 1;
}

// ============================================
// TELL
// ============================================

int native_file_tell(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isInt())
    {
        vm->push(vm->makeInt(0));
        return 1;
    }    


    int id = args[0].asInt();
    if (id <= 0 || id > (int)openFiles.size() || !openFiles[id - 1])
    {
        vm->push(vm->makeInt(0));
        return 1;
    }

    FileBuffer *fb = openFiles[id - 1];
    vm->push(vm->makeInt((int)fb->cursor));
    return 1;
}

// ============================================
// SIZE
// ============================================

int native_file_size(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isInt())
    {
        vm->push(vm->makeInt(0));
        return 1;
    }    
    

    int id = args[0].asInt();
    if (id <= 0 || id > (int)openFiles.size() || !openFiles[id - 1])   
    {
        vm->push(vm->makeInt(0));
        return 1;
    }

    FileBuffer *fb = openFiles[id - 1];
    vm->push(vm->makeInt((int)fb->data.size()));
    return 1;
}

// ============================================
// REGISTO
// ============================================

void Interpreter::registerFile()
{
    static bool initialized = false;
    if (!initialized)
    {
        atexit(FileModuleCleanup);
        initialized = true;
    }

    addModule("file")
        .addFunction("exists", native_file_exists, 1)
        .addFunction("open", native_file_open, -1)
        .addFunction("save", native_file_save, 1)
        .addFunction("close", native_file_close, 1)

        .addFunction("write_byte", native_file_write_byte, 2)
        .addFunction("write_short", native_file_write_short, 2)
        .addFunction("write_ushort", native_file_write_ushort, 2)
        .addFunction("write_int", native_file_write_int, 2)
        .addFunction("write_uint", native_file_write_uint, 2)
        .addFunction("write_float", native_file_write_float, 2)
        .addFunction("write_double", native_file_write_double, 2)
        .addFunction("write_bool", native_file_write_bool, 2)
        .addFunction("write_string", native_file_write_string, 2)
        .addFunction("write_buffer", native_file_write_buffer, 2)

        .addFunction("read_byte", native_file_read_byte, 1)
        .addFunction("read_short", native_file_read_short, 1)
        .addFunction("read_ushort", native_file_read_ushort, 1)
        .addFunction("read_int", native_file_read_int, 1)
        .addFunction("read_uint", native_file_read_uint, 1)
        .addFunction("read_float", native_file_read_float, 1)
        .addFunction("read_double", native_file_read_double, 1)
        .addFunction("read_bool", native_file_read_bool, 1)
        .addFunction("read_string", native_file_read_string, 1)
        .addFunction("read_bytes", native_file_read_bytes, 2)
        .addFunction("read_all", native_file_read_all, 1)

        .addFunction("seek", native_file_seek, 2)
        .addFunction("tell", native_file_tell, 1)
        .addFunction("size", native_file_size, 1);
}

#endif
