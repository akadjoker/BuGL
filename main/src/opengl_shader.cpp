#include "bindings.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>
#include <SDL2/SDL.h>
#include <filesystem>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

namespace SDLBindings
{
    struct WatchedShaderProgram
    {
        std::string vertexPath;
        std::string fragmentPath;
        std::filesystem::file_time_type vertexWriteTime;
        std::filesystem::file_time_type fragmentWriteTime;
        bool watchEnabled = true;
    };

    static std::unordered_map<GLuint, WatchedShaderProgram> g_watchedShaderPrograms;

    static bool read_text_file_sdl(const char *path, std::string *outText)
    {
        if (!path || !outText)
            return false;

        SDL_RWops *rw = SDL_RWFromFile(path, "rb");
        if (!rw)
            return false;

        outText->clear();
        Sint64 rawSize = SDL_RWsize(rw);
        if (rawSize > 0)
        {
            outText->resize((size_t)rawSize);
            const size_t got = SDL_RWread(rw, outText->data(), 1, (size_t)rawSize);
            SDL_RWclose(rw);
            return got == (size_t)rawSize;
        }

        char chunk[4096];
        for (;;)
        {
            const size_t got = SDL_RWread(rw, chunk, 1, sizeof(chunk));
            if (got > 0)
                outText->append(chunk, got);
            if (got < sizeof(chunk))
                break;
        }
        SDL_RWclose(rw);
        return !outText->empty();
    }

    static bool get_write_time(const std::string &path, std::filesystem::file_time_type *outTime)
    {
        if (!outTime)
            return false;
        std::error_code ec;
        const auto t = std::filesystem::last_write_time(path, ec);
        if (ec)
            return false;
        *outTime = t;
        return true;
    }

    static bool compile_shader_from_source(GLenum type, const char *source, GLuint *outShader, std::string *outLog)
    {
        if (!source || !outShader)
            return false;

        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint ok = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
        if (!ok)
        {
            GLint len = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
            if (outLog)
            {
                outLog->clear();
                if (len > 1)
                {
                    outLog->resize((size_t)len);
                    GLsizei written = 0;
                    glGetShaderInfoLog(shader, len, &written, outLog->data());
                }
            }
            glDeleteShader(shader);
            return false;
        }

        *outShader = shader;
        return true;
    }

    static bool relink_program(GLuint program,
                               const std::string &vertexSource,
                               const std::string &fragmentSource,
                               std::string *outLog)
    {
        GLuint vs = 0;
        GLuint fs = 0;
        std::string localLog;

        if (!compile_shader_from_source(GL_VERTEX_SHADER, vertexSource.c_str(), &vs, &localLog))
        {
            if (outLog)
                *outLog = "Vertex shader compile failed:\n" + localLog;
            return false;
        }

        if (!compile_shader_from_source(GL_FRAGMENT_SHADER, fragmentSource.c_str(), &fs, &localLog))
        {
            glDeleteShader(vs);
            if (outLog)
                *outLog = "Fragment shader compile failed:\n" + localLog;
            return false;
        }

        GLint attachedCount = 0;
        glGetProgramiv(program, GL_ATTACHED_SHADERS, &attachedCount);
        std::vector<GLuint> oldShaders;
        if (attachedCount > 0)
        {
            oldShaders.resize((size_t)attachedCount);
            GLsizei written = 0;
            glGetAttachedShaders(program, attachedCount, &written, oldShaders.data());
            oldShaders.resize((size_t)written);
        }

        for (GLuint oldShader : oldShaders)
            glDetachShader(program, oldShader);

        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);

        GLint linked = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked)
        {
            GLint len = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
            if (outLog)
            {
                outLog->clear();
                if (len > 1)
                {
                    outLog->resize((size_t)len);
                    GLsizei written = 0;
                    glGetProgramInfoLog(program, len, &written, outLog->data());
                }
            }

            glDetachShader(program, vs);
            glDetachShader(program, fs);
            glDeleteShader(vs);
            glDeleteShader(fs);

            for (GLuint oldShader : oldShaders)
                glAttachShader(program, oldShader);
            glLinkProgram(program);
            return false;
        }

        glDetachShader(program, vs);
        glDetachShader(program, fs);
        glDeleteShader(vs);
        glDeleteShader(fs);

        for (GLuint oldShader : oldShaders)
            glDeleteShader(oldShader);

        return true;
    }

    static bool reload_program_if_needed(GLuint program, bool force, std::string *outError)
    {
        auto it = g_watchedShaderPrograms.find(program);
        if (it == g_watchedShaderPrograms.end())
            return true;

        WatchedShaderProgram &watch = it->second;
        if (!watch.watchEnabled && !force)
            return true;

        std::filesystem::file_time_type vsTime = {};
        std::filesystem::file_time_type fsTime = {};
        const bool hasVsTime = get_write_time(watch.vertexPath, &vsTime);
        const bool hasFsTime = get_write_time(watch.fragmentPath, &fsTime);

        if (!force && hasVsTime && hasFsTime &&
            vsTime == watch.vertexWriteTime &&
            fsTime == watch.fragmentWriteTime)
        {
            return true;
        }

        std::string vsText;
        std::string fsText;
        if (!read_text_file_sdl(watch.vertexPath.c_str(), &vsText))
        {
            if (outError)
                *outError = "Failed to read vertex shader: " + watch.vertexPath;
            return false;
        }
        if (!read_text_file_sdl(watch.fragmentPath.c_str(), &fsText))
        {
            if (outError)
                *outError = "Failed to read fragment shader: " + watch.fragmentPath;
            return false;
        }

        std::string log;
        if (!relink_program(program, vsText, fsText, &log))
        {
            if (outError)
                *outError = "Program link failed:\n" + log;
            return false;
        }

        if (hasVsTime)
            watch.vertexWriteTime = vsTime;
        if (hasFsTime)
            watch.fragmentWriteTime = fsTime;
        return true;
    }

    static GLuint resolve_watched_program(GLuint program, Interpreter *vm)
    {
        auto it = g_watchedShaderPrograms.find(program);
        if (it == g_watchedShaderPrograms.end())
            return program;

        std::string err;
        if (!reload_program_if_needed(program, false, &err) && get_gl_debug_enabled())
            vm->runtimeError("Hot reload failed for program %u: %s", (unsigned)program, err.c_str());

        return program;
    }

    static bool resolveModernPointerArg(const Value &value, const char *fnName, const GLvoid **outPtr)
    {
        if (value.isPointer())
        {
            *outPtr = value.asPointer();
            return true;
        }

        if (value.isBuffer())
        {
            BufferInstance *buf = value.asBuffer();
            *outPtr = buf ? (const GLvoid *)buf->data : nullptr;
            return true;
        }

        if (getBuiltinTypedArrayData(value, (const void **)outPtr))
        {
            return true;
        }

        if (value.isNumber())
        {
            *outPtr = (const GLvoid *)(uintptr_t)value.asNumber();
            return true;
        }

        Error("%s expects pointer/buffer/typedarray or offset number", fnName);
        return false;
    }

    static bool resolveModernUploadDataArg(const Value &value,
                                           GLsizeiptr size,
                                           const char *fnName,
                                           const GLvoid **outPtr,
                                           std::vector<unsigned char> &scratch)
    {
        if (value.isNil())
        {
            *outPtr = nullptr;
            return true;
        }

        if (value.isPointer())
        {
            *outPtr = value.asPointer();
            return true;
        }

        if (value.isBuffer())
        {
            BufferInstance *buf = value.asBuffer();
            if (buf && size > 0)
            {
                size_t have = (size_t)buf->count * (size_t)buf->elementSize;
                if (have < (size_t)size)
                {
                    Error("%s buffer too small: need %lld bytes", fnName, (long long)size);
                    return false;
                }
            }
            *outPtr = buf ? (const GLvoid *)buf->data : nullptr;
            return true;
        }

        if (getBuiltinTypedArrayData(value, (const void **)outPtr))
        {
            return true;
        }

        if (value.isArray())
        {
            ArrayInstance *arr = value.asArray();
            if (!arr)
            {
                Error("%s received invalid array", fnName);
                return false;
            }

            const int count = (int)arr->values.size();
            if (count <= 0 || size <= 0)
            {
                *outPtr = nullptr;
                return true;
            }

            if (size == (GLsizeiptr)(count * (int)sizeof(float)))
            {
                scratch.resize((size_t)count * sizeof(float));
                float *out = (float *)scratch.data();
                for (int i = 0; i < count; i++)
                {
                    if (!arr->values[i].isNumber())
                    {
                        Error("%s array must contain only numeric values", fnName);
                        return false;
                    }
                    out[i] = (float)arr->values[i].asNumber();
                }
                *outPtr = scratch.data();
                return true;
            }

            if (size == (GLsizeiptr)(count * (int)sizeof(int)))
            {
                scratch.resize((size_t)count * sizeof(int));
                int *out = (int *)scratch.data();
                for (int i = 0; i < count; i++)
                {
                    if (!arr->values[i].isNumber())
                    {
                        Error("%s array must contain only numeric values", fnName);
                        return false;
                    }
                    out[i] = arr->values[i].asInt();
                }
                *outPtr = scratch.data();
                return true;
            }

            Error("%s array size mismatch for requested upload size", fnName);
            return false;
        }

        Error("%s expects pointer, buffer, typedarray, array or nil", fnName);
        return false;
    }

    int native_glCreateShader(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("glCreateShader expects 1 argument: type");
            return 0;
        }
        GLuint shader = glCreateShader((GLenum)args[0].asNumber());
        vm->pushInt((int)shader);
        return 1;
    }

    int native_glShaderSource(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("glShaderSource expects 2 arguments: shader, source");
            return 0;
        }

        GLuint shader = (GLuint)args[0].asNumber();
        if (args[1].isString())
        {
            const char *src = args[1].asStringChars();
            glShaderSource(shader, 1, &src, nullptr);
            return 0;
        }

        if (args[1].isArray())
        {
            ArrayInstance *arr = args[1].asArray();
            if (!arr || arr->values.empty())
            {
                Error("glShaderSource array must contain at least one string");
                return 0;
            }
            std::vector<const char *> chunks;
            chunks.reserve(arr->values.size());
            for (int i = 0; i < (int)arr->values.size(); i++)
            {
                if (!arr->values[i].isString())
                {
                    Error("glShaderSource array must contain only strings");
                    return 0;
                }
                chunks.push_back(arr->values[i].asStringChars());
            }
            glShaderSource(shader, (GLsizei)chunks.size(), chunks.data(), nullptr);
            return 0;
        }

        Error("glShaderSource source must be string or array of strings");
        return 0;
    }

    int native_glCompileShader(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glCompileShader expects 1 argument");
            return 0;
        }
        glCompileShader((GLuint)args[0].asNumber());
        return 0;
    }

    int native_glGetShaderiv(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("glGetShaderiv expects 2 arguments: shader, pname");
            return 0;
        }
        GLint value = 0;
        glGetShaderiv((GLuint)args[0].asNumber(), (GLenum)args[1].asNumber(), &value);
        vm->pushInt((int)value);
        return 1;
    }

    int native_glGetShaderInfoLog(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("glGetShaderInfoLog expects 1 argument: shader");
            return 0;
        }
        GLuint shader = (GLuint)args[0].asNumber();
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
        if (len <= 1)
        {
            vm->pushString("");
            return 1;
        }
        std::vector<char> log((size_t)len);
        GLsizei written = 0;
        glGetShaderInfoLog(shader, len, &written, log.data());
        vm->pushString(log.data());
        return 1;
    }

    int native_glDeleteShader(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glDeleteShader expects 1 argument");
            return 0;
        }
        glDeleteShader((GLuint)args[0].asNumber());
        return 0;
    }

    int native_glCreateProgram(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("glCreateProgram expects 0 arguments");
            return 0;
        }
        GLuint program = glCreateProgram();
        vm->pushInt((int)program);
        return 1;
    }

    int native_LoadShaderProgram(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2 && argc != 3)
        {
            Error("LoadShaderProgram expects (vertexPath, fragmentPath[, watch])");
            return 0;
        }
        if (!args[0].isString() || !args[1].isString())
        {
            Error("LoadShaderProgram expects string paths");
            return 0;
        }

        bool watchEnabled = true;
        if (argc == 3)
            watchEnabled = args[2].asBool();

        const char *vertexPath = args[0].asStringChars();
        const char *fragmentPath = args[1].asStringChars();

        std::string vertexSource;
        std::string fragmentSource;
        if (!read_text_file_sdl(vertexPath, &vertexSource))
        {
            Error("LoadShaderProgram failed to read vertex shader: %s", vertexPath);
            return 0;
        }
        if (!read_text_file_sdl(fragmentPath, &fragmentSource))
        {
            Error("LoadShaderProgram failed to read fragment shader: %s", fragmentPath);
            return 0;
        }

        GLuint vs = 0;
        GLuint fs = 0;
        std::string log;
        if (!compile_shader_from_source(GL_VERTEX_SHADER, vertexSource.c_str(), &vs, &log))
        {
            Error("LoadShaderProgram vertex compile failed:\n%s", log.c_str());
            return 0;
        }
        if (!compile_shader_from_source(GL_FRAGMENT_SHADER, fragmentSource.c_str(), &fs, &log))
        {
            glDeleteShader(vs);
            Error("LoadShaderProgram fragment compile failed:\n%s", log.c_str());
            return 0;
        }

        GLuint program = glCreateProgram();
        glAttachShader(program, vs);
        glAttachShader(program, fs);
        glLinkProgram(program);

        GLint linked = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if (!linked)
        {
            GLint len = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
            std::string linkLog;
            if (len > 1)
            {
                linkLog.resize((size_t)len);
                GLsizei written = 0;
                glGetProgramInfoLog(program, len, &written, linkLog.data());
            }

            glDetachShader(program, vs);
            glDetachShader(program, fs);
            glDeleteShader(vs);
            glDeleteShader(fs);
            glDeleteProgram(program);

            Error("LoadShaderProgram link failed:\n%s", linkLog.c_str());
            return 0;
        }

        glDetachShader(program, vs);
        glDetachShader(program, fs);
        glDeleteShader(vs);
        glDeleteShader(fs);

        if (watchEnabled)
        {
            WatchedShaderProgram watched;
            watched.vertexPath = vertexPath;
            watched.fragmentPath = fragmentPath;
            watched.watchEnabled = true;
            (void)get_write_time(watched.vertexPath, &watched.vertexWriteTime);
            (void)get_write_time(watched.fragmentPath, &watched.fragmentWriteTime);
            g_watchedShaderPrograms[program] = watched;
        }
        else
        {
            g_watchedShaderPrograms.erase(program);
        }

        vm->pushInt((int)program);
        return 1;
    }

    int native_ReloadShaderProgram(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("ReloadShaderProgram expects (program)");
            return 0;
        }

        const GLuint program = (GLuint)args[0].asNumber();
        if (g_watchedShaderPrograms.find(program) == g_watchedShaderPrograms.end())
        {
            Error("ReloadShaderProgram expects a program loaded with LoadShaderProgram(..., watch=true)");
            return 0;
        }

        std::string err;
        if (!reload_program_if_needed(program, true, &err))
        {
            Error("ReloadShaderProgram failed: %s", err.c_str());
            return 0;
        }

        vm->pushBool(true);
        return 1;
    }

    int native_glAttachShader(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2)
        {
            Error("glAttachShader expects 2 arguments: program, shader");
            return 0;
        }
        glAttachShader((GLuint)args[0].asNumber(), (GLuint)args[1].asNumber());
        return 0;
    }

    int native_glDetachShader(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2)
        {
            Error("glDetachShader expects 2 arguments: program, shader");
            return 0;
        }
        glDetachShader((GLuint)args[0].asNumber(), (GLuint)args[1].asNumber());
        return 0;
    }

    int native_glLinkProgram(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glLinkProgram expects 1 argument");
            return 0;
        }
        glLinkProgram((GLuint)args[0].asNumber());
        return 0;
    }

    int native_glValidateProgram(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glValidateProgram expects 1 argument");
            return 0;
        }
        glValidateProgram((GLuint)args[0].asNumber());
        return 0;
    }

    int native_glGetProgramiv(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2)
        {
            Error("glGetProgramiv expects 2 arguments: program, pname");
            return 0;
        }
        GLint value = 0;
        glGetProgramiv((GLuint)args[0].asNumber(), (GLenum)args[1].asNumber(), &value);
        vm->pushInt((int)value);
        return 1;
    }

    int native_glGetProgramInfoLog(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("glGetProgramInfoLog expects 1 argument: program");
            return 0;
        }
        GLuint program = (GLuint)args[0].asNumber();
        GLint len = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
        if (len <= 1)
        {
            vm->pushString("");
            return 1;
        }
        std::vector<char> log((size_t)len);
        GLsizei written = 0;
        glGetProgramInfoLog(program, len, &written, log.data());
        vm->pushString(log.data());
        return 1;
    }

    int native_glDeleteProgram(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glDeleteProgram expects 1 argument");
            return 0;
        }
        const GLuint program = (GLuint)args[0].asNumber();
        g_watchedShaderPrograms.erase(program);
        glDeleteProgram(program);
        return 0;
    }

    int native_glUseProgram(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("glUseProgram expects 1 argument");
            return 0;
        }
        GLuint program = (GLuint)args[0].asNumber();
        program = resolve_watched_program(program, vm);
        glUseProgram(program);
        return 0;
    }

    int native_glBindAttribLocation(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3 || !args[2].isString())
        {
            Error("glBindAttribLocation expects (program, index, name)");
            return 0;
        }
        glBindAttribLocation((GLuint)args[0].asNumber(), (GLuint)args[1].asNumber(), args[2].asStringChars());
        return 0;
    }

    int native_glGetAttribLocation(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2 || !args[1].isString())
        {
            Error("glGetAttribLocation expects (program, name)");
            return 0;
        }
        GLuint program = (GLuint)args[0].asNumber();
        program = resolve_watched_program(program, vm);
        GLint location = glGetAttribLocation(program, args[1].asStringChars());
        vm->pushInt((int)location);
        return 1;
    }

    int native_glEnableVertexAttribArray(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glEnableVertexAttribArray expects 1 argument: index");
            return 0;
        }
        glEnableVertexAttribArray((GLuint)args[0].asNumber());
        return 0;
    }

    int native_glDisableVertexAttribArray(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glDisableVertexAttribArray expects 1 argument: index");
            return 0;
        }
        glDisableVertexAttribArray((GLuint)args[0].asNumber());
        return 0;
    }

    int native_glVertexAttribPointer(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 6 && argc != 7)
        {
            Error("glVertexAttribPointer expects 6 or 7 args: index, size, type, normalized, stride, pointerOrOffset [, byteOffset]");
            return 0;
        }

        GLuint index = (GLuint)args[0].asNumber();
        GLint size = (GLint)args[1].asNumber();
        GLenum type = (GLenum)args[2].asNumber();
        GLboolean normalized = args[3].asBool() ? GL_TRUE : GL_FALSE;
        GLsizei stride = (GLsizei)args[4].asNumber();
        const GLvoid *pointer = nullptr;
        if (!resolveModernPointerArg(args[5], "glVertexAttribPointer", &pointer))
            return 0;

        if (argc == 7)
        {
            if (!args[6].isNumber())
            {
                Error("glVertexAttribPointer byteOffset must be numeric");
                return 0;
            }
            uintptr_t base = (uintptr_t)pointer;
            uintptr_t add = (uintptr_t)args[6].asNumber();
            pointer = (const GLvoid *)(base + add);
        }

        glVertexAttribPointer(index, size, type, normalized, stride, pointer);
        return 0;
    }

    int native_glGetUniformLocation(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 2 || !args[1].isString())
        {
            Error("glGetUniformLocation expects (program, name)");
            return 0;
        }
        GLuint program = (GLuint)args[0].asNumber();
        program = resolve_watched_program(program, vm);
        GLint location = glGetUniformLocation(program, args[1].asStringChars());
        vm->pushInt((int)location);
        return 1;
    }

    int native_glUniform1i(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2) { Error("glUniform1i expects (location, x)"); return 0; }
        glUniform1i((GLint)args[0].asNumber(), (GLint)args[1].asNumber());
        return 0;
    }

    int native_glUniform1f(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 2) { Error("glUniform1f expects (location, x)"); return 0; }
        glUniform1f((GLint)args[0].asNumber(), (GLfloat)args[1].asNumber());
        return 0;
    }

    int native_glUniform2f(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3) { Error("glUniform2f expects (location, x, y)"); return 0; }
        glUniform2f((GLint)args[0].asNumber(), (GLfloat)args[1].asNumber(), (GLfloat)args[2].asNumber());
        return 0;
    }

    int native_glUniform3f(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 4) { Error("glUniform3f expects (location, x, y, z)"); return 0; }
        glUniform3f((GLint)args[0].asNumber(), (GLfloat)args[1].asNumber(), (GLfloat)args[2].asNumber(), (GLfloat)args[3].asNumber());
        return 0;
    }

    int native_glUniform4f(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 5) { Error("glUniform4f expects (location, x, y, z, w)"); return 0; }
        glUniform4f((GLint)args[0].asNumber(), (GLfloat)args[1].asNumber(), (GLfloat)args[2].asNumber(), (GLfloat)args[3].asNumber(), (GLfloat)args[4].asNumber());
        return 0;
    }

    // =====================================================
    // INTEGER UNIFORMS
    // =====================================================

    int native_glUniform2i(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 3) { Error("glUniform2i expects (location, x, y)"); return 0; }
        glUniform2i((GLint)args[0].asNumber(), (GLint)args[1].asNumber(), (GLint)args[2].asNumber());
        return 0;
    }

    int native_glUniform3i(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 4) { Error("glUniform3i expects (location, x, y, z)"); return 0; }
        glUniform3i((GLint)args[0].asNumber(), (GLint)args[1].asNumber(), (GLint)args[2].asNumber(), (GLint)args[3].asNumber());
        return 0;
    }

    int native_glUniform4i(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 5) { Error("glUniform4i expects (location, x, y, z, w)"); return 0; }
        glUniform4i((GLint)args[0].asNumber(), (GLint)args[1].asNumber(), (GLint)args[2].asNumber(), (GLint)args[3].asNumber(), (GLint)args[4].asNumber());
        return 0;
    }

    // =====================================================
    // FLOAT VECTOR UNIFORMS (1fv, 2fv, 3fv, 4fv)
    // =====================================================

    int native_glUniform1fv(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("glUniform1fv expects (location, count, data)");
            return 0;
        }
        GLint location = (GLint)args[0].asNumber();
        GLsizei count = (GLsizei)args[1].asNumber();
        if (count < 0) { Error("glUniform1fv count must be >= 0"); return 0; }

        const GLvoid *ptr = nullptr;
        std::vector<unsigned char> scratch;
        size_t bytes = (size_t)count * sizeof(float);
        if (!resolveModernUploadDataArg(args[2], (GLsizeiptr)bytes, "glUniform1fv", &ptr, scratch))
            return 0;
        glUniform1fv(location, count, (const GLfloat *)ptr);
        return 0;
    }

    int native_glUniform2fv(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("glUniform2fv expects (location, count, data)");
            return 0;
        }
        GLint location = (GLint)args[0].asNumber();
        GLsizei count = (GLsizei)args[1].asNumber();
        if (count < 0) { Error("glUniform2fv count must be >= 0"); return 0; }

        const GLvoid *ptr = nullptr;
        std::vector<unsigned char> scratch;
        size_t bytes = (size_t)count * 2 * sizeof(float);
        if (!resolveModernUploadDataArg(args[2], (GLsizeiptr)bytes, "glUniform2fv", &ptr, scratch))
            return 0;
        glUniform2fv(location, count, (const GLfloat *)ptr);
        return 0;
    }

    int native_glUniform3fv(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("glUniform3fv expects (location, count, data)");
            return 0;
        }
        GLint location = (GLint)args[0].asNumber();
        GLsizei count = (GLsizei)args[1].asNumber();
        if (count < 0) { Error("glUniform3fv count must be >= 0"); return 0; }

        const GLvoid *ptr = nullptr;
        std::vector<unsigned char> scratch;
        size_t bytes = (size_t)count * 3 * sizeof(float);
        if (!resolveModernUploadDataArg(args[2], (GLsizeiptr)bytes, "glUniform3fv", &ptr, scratch))
            return 0;
        glUniform3fv(location, count, (const GLfloat *)ptr);
        return 0;
    }

    int native_glUniform4fv(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 3)
        {
            Error("glUniform4fv expects (location, count, data)");
            return 0;
        }
        GLint location = (GLint)args[0].asNumber();
        GLsizei count = (GLsizei)args[1].asNumber();
        if (count < 0) { Error("glUniform4fv count must be >= 0"); return 0; }

        const GLvoid *ptr = nullptr;
        std::vector<unsigned char> scratch;
        size_t bytes = (size_t)count * 4 * sizeof(float);
        if (!resolveModernUploadDataArg(args[2], (GLsizeiptr)bytes, "glUniform4fv", &ptr, scratch))
            return 0;
        glUniform4fv(location, count, (const GLfloat *)ptr);
        return 0;
    }

    // =====================================================
    // MATRIX UNIFORMS (2fv, 3fv)
    // =====================================================

    int native_glUniformMatrix2fv(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("glUniformMatrix2fv expects (location, count, transpose, data)");
            return 0;
        }
        GLint location = (GLint)args[0].asNumber();
        GLsizei count = (GLsizei)args[1].asNumber();
        GLboolean transpose = args[2].asBool() ? GL_TRUE : GL_FALSE;
        if (count < 0) { Error("glUniformMatrix2fv count must be >= 0"); return 0; }

        const GLvoid *ptr = nullptr;
        std::vector<unsigned char> scratch;
        size_t bytes = (size_t)count * 4 * sizeof(float);
        if (!resolveModernUploadDataArg(args[3], (GLsizeiptr)bytes, "glUniformMatrix2fv", &ptr, scratch))
            return 0;
        glUniformMatrix2fv(location, count, transpose, (const GLfloat *)ptr);
        return 0;
    }

    int native_glUniformMatrix3fv(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("glUniformMatrix3fv expects (location, count, transpose, data)");
            return 0;
        }
        GLint location = (GLint)args[0].asNumber();
        GLsizei count = (GLsizei)args[1].asNumber();
        GLboolean transpose = args[2].asBool() ? GL_TRUE : GL_FALSE;
        if (count < 0) { Error("glUniformMatrix3fv count must be >= 0"); return 0; }

        const GLvoid *ptr = nullptr;
        std::vector<unsigned char> scratch;
        size_t bytes = (size_t)count * 9 * sizeof(float);
        if (!resolveModernUploadDataArg(args[3], (GLsizeiptr)bytes, "glUniformMatrix3fv", &ptr, scratch))
            return 0;
        glUniformMatrix3fv(location, count, transpose, (const GLfloat *)ptr);
        return 0;
    }

    int native_glUniformMatrix4fv(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("glUniformMatrix4fv expects (location, count, transpose, data)");
            return 0;
        }

        GLint location = (GLint)args[0].asNumber();
        GLsizei count = (GLsizei)args[1].asNumber();
        GLboolean transpose = args[2].asBool() ? GL_TRUE : GL_FALSE;

        if (count < 0)
        {
            Error("glUniformMatrix4fv count must be >= 0");
            return 0;
        }

        const GLvoid *ptr = nullptr;
        std::vector<unsigned char> scratch;
        size_t bytes = 0;
        if ((size_t)count > (std::numeric_limits<size_t>::max() / (16 * sizeof(float))))
        {
            Error("glUniformMatrix4fv size overflow");
            return 0;
        }
        bytes = (size_t)count * 16 * sizeof(float);
        if (!resolveModernUploadDataArg(args[3], (GLsizeiptr)bytes, "glUniformMatrix4fv", &ptr, scratch))
            return 0;

        glUniformMatrix4fv(location, count, transpose, (const GLfloat *)ptr);
        return 0;
    }

    int native_glDrawElements(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 4)
        {
            Error("glDrawElements expects (mode, count, type, indicesOrOffset)");
            return 0;
        }

        GLenum mode = (GLenum)args[0].asNumber();
        GLsizei count = (GLsizei)args[1].asNumber();
        GLenum type = (GLenum)args[2].asNumber();
        const GLvoid *indices = nullptr;
        if (!resolveModernPointerArg(args[3], "glDrawElements", &indices))
            return 0;

        glDrawElements(mode, count, type, indices);
        return 0;
    }

    int native_glActiveTexture(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glActiveTexture expects 1 argument");
            return 0;
        }
        glActiveTexture((GLenum)args[0].asNumber());
        return 0;
    }

    int native_glGenVertexArrays(Interpreter *vm, int argc, Value *args)
    {
        (void)args;
        if (argc != 0)
        {
            Error("glGenVertexArrays expects 0 arguments");
            return 0;
        }
        GLuint vao = 0;
        glGenVertexArrays(1, &vao);
        vm->pushInt((int)vao);
        return 1;
    }

    int native_glBindVertexArray(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glBindVertexArray expects 1 argument: vao");
            return 0;
        }
        glBindVertexArray((GLuint)args[0].asNumber());
        return 0;
    }

    int native_glDeleteVertexArrays(Interpreter *vm, int argc, Value *args)
    {
        (void)vm;
        if (argc != 1)
        {
            Error("glDeleteVertexArrays expects 1 argument: vao");
            return 0;
        }
        GLuint vao = (GLuint)args[0].asNumber();
        glDeleteVertexArrays(1, &vao);
        return 0;
    }

    int native_glIsVertexArray(Interpreter *vm, int argc, Value *args)
    {
        if (argc != 1)
        {
            Error("glIsVertexArray expects 1 argument: vao");
            return 0;
        }
        GLboolean ok = glIsVertexArray((GLuint)args[0].asNumber());
        vm->pushBool(ok);
        return 1;
    }

    void register_opengl_shader(ModuleBuilder &module)
    {
        module.addFunction("LoadShaderProgram", native_LoadShaderProgram, -1)
            .addFunction("ReloadShaderProgram", native_ReloadShaderProgram, 1)
            .addFunction("glCreateShader", native_glCreateShader, 1)
            .addFunction("glShaderSource", native_glShaderSource, 2)
            .addFunction("glCompileShader", native_glCompileShader, 1)
            .addFunction("glGetShaderiv", native_glGetShaderiv, 2)
            .addFunction("glGetShaderInfoLog", native_glGetShaderInfoLog, 1)
            .addFunction("glDeleteShader", native_glDeleteShader, 1)
            .addFunction("glCreateProgram", native_glCreateProgram, 0)
            .addFunction("glAttachShader", native_glAttachShader, 2)
            .addFunction("glDetachShader", native_glDetachShader, 2)
            .addFunction("glLinkProgram", native_glLinkProgram, 1)
            .addFunction("glValidateProgram", native_glValidateProgram, 1)
            .addFunction("glGetProgramiv", native_glGetProgramiv, 2)
            .addFunction("glGetProgramInfoLog", native_glGetProgramInfoLog, 1)
            .addFunction("glDeleteProgram", native_glDeleteProgram, 1)
            .addFunction("glUseProgram", native_glUseProgram, 1)
            .addFunction("glBindAttribLocation", native_glBindAttribLocation, 3)
            .addFunction("glGetAttribLocation", native_glGetAttribLocation, 2)
            .addFunction("glEnableVertexAttribArray", native_glEnableVertexAttribArray, 1)
            .addFunction("glDisableVertexAttribArray", native_glDisableVertexAttribArray, 1)
            .addFunction("glVertexAttribPointer", native_glVertexAttribPointer, -1)
            .addFunction("glGetUniformLocation", native_glGetUniformLocation, 2)
            .addFunction("glUniform1i", native_glUniform1i, 2)
            .addFunction("glUniform1f", native_glUniform1f, 2)
            .addFunction("glUniform2f", native_glUniform2f, 3)
            .addFunction("glUniform3f", native_glUniform3f, 4)
            .addFunction("glUniform4f", native_glUniform4f, 5)
            .addFunction("glUniform2i", native_glUniform2i, 3)
            .addFunction("glUniform3i", native_glUniform3i, 4)
            .addFunction("glUniform4i", native_glUniform4i, 5)
            .addFunction("glUniform1fv", native_glUniform1fv, 3)
            .addFunction("glUniform2fv", native_glUniform2fv, 3)
            .addFunction("glUniform3fv", native_glUniform3fv, 3)
            .addFunction("glUniform4fv", native_glUniform4fv, 3)
            .addFunction("glUniformMatrix2fv", native_glUniformMatrix2fv, 4)
            .addFunction("glUniformMatrix3fv", native_glUniformMatrix3fv, 4)
            .addFunction("glUniformMatrix4fv", native_glUniformMatrix4fv, 4)
            .addFunction("glDrawElements", native_glDrawElements, 4)
            .addFunction("glActiveTexture", native_glActiveTexture, 1)
            .addFunction("glGenVertexArrays", native_glGenVertexArrays, 0)
            .addFunction("glBindVertexArray", native_glBindVertexArray, 1)
            .addFunction("glDeleteVertexArrays", native_glDeleteVertexArrays, 1)
            .addFunction("glIsVertexArray", native_glIsVertexArray, 1)

            .addInt("GL_VERTEX_SHADER", GL_VERTEX_SHADER)
            .addInt("GL_FRAGMENT_SHADER", GL_FRAGMENT_SHADER)
            .addInt("GL_COMPILE_STATUS", GL_COMPILE_STATUS)
            .addInt("GL_LINK_STATUS", GL_LINK_STATUS)
            .addInt("GL_VALIDATE_STATUS", GL_VALIDATE_STATUS)
            .addInt("GL_INFO_LOG_LENGTH", GL_INFO_LOG_LENGTH)
            .addInt("GL_ACTIVE_ATTRIBUTES", GL_ACTIVE_ATTRIBUTES)
            .addInt("GL_ACTIVE_UNIFORMS", GL_ACTIVE_UNIFORMS)
            .addInt("GL_CURRENT_PROGRAM", GL_CURRENT_PROGRAM)
            .addInt("GL_TEXTURE0", GL_TEXTURE0)
            .addInt("GL_TEXTURE1", GL_TEXTURE1)
            .addInt("GL_TEXTURE2", GL_TEXTURE2)
            .addInt("GL_TEXTURE3", GL_TEXTURE3);

#ifdef GL_GEOMETRY_SHADER
        module.addInt("GL_GEOMETRY_SHADER", GL_GEOMETRY_SHADER);
#endif

#ifdef GL_VERTEX_ARRAY_BINDING
        module.addInt("GL_VERTEX_ARRAY_BINDING", GL_VERTEX_ARRAY_BINDING);
#endif
#ifdef GL_MAX_VERTEX_ATTRIBS
        module.addInt("GL_MAX_VERTEX_ATTRIBS", GL_MAX_VERTEX_ATTRIBS);
#endif
    }
}
