#ifdef _WIN32
#include <windows.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>

namespace
{
    template <typename T>
    static T load_gl_proc(const char *name)
    {
        static HMODULE opengl32 = LoadLibraryA("opengl32.dll");
        void *proc = (void *)SDL_GL_GetProcAddress(name);
        if (!proc && opengl32)
            proc = (void *)GetProcAddress(opengl32, name);
        return reinterpret_cast<T>(proc);
    }
}

extern "C" void APIENTRY glActiveTexture(GLenum texture)
{
    using Proc = PFNGLACTIVETEXTUREPROC;
    static Proc proc = load_gl_proc<Proc>("glActiveTexture");
    if (!proc)
    {
        return;
    }
    proc(texture);
}
extern "C" PFNGLACTIVETEXTUREPROC __imp_glActiveTexture = glActiveTexture;

extern "C" void APIENTRY glAttachShader(GLuint program, GLuint shader)
{
    using Proc = PFNGLATTACHSHADERPROC;
    static Proc proc = load_gl_proc<Proc>("glAttachShader");
    if (!proc)
    {
        return;
    }
    proc(program, shader);
}
extern "C" PFNGLATTACHSHADERPROC __imp_glAttachShader = glAttachShader;

extern "C" void APIENTRY glBeginQuery(GLenum target, GLuint id)
{
    using Proc = PFNGLBEGINQUERYPROC;
    static Proc proc = load_gl_proc<Proc>("glBeginQuery");
    if (!proc)
    {
        return;
    }
    proc(target, id);
}
extern "C" PFNGLBEGINQUERYPROC __imp_glBeginQuery = glBeginQuery;

extern "C" void APIENTRY glBindAttribLocation(GLuint program, GLuint index, const GLchar *name)
{
    using Proc = PFNGLBINDATTRIBLOCATIONPROC;
    static Proc proc = load_gl_proc<Proc>("glBindAttribLocation");
    if (!proc)
    {
        return;
    }
    proc(program, index, name);
}
extern "C" PFNGLBINDATTRIBLOCATIONPROC __imp_glBindAttribLocation = glBindAttribLocation;

extern "C" void APIENTRY glBindBuffer(GLenum target, GLuint buffer)
{
    using Proc = PFNGLBINDBUFFERPROC;
    static Proc proc = load_gl_proc<Proc>("glBindBuffer");
    if (!proc)
    {
        return;
    }
    proc(target, buffer);
}
extern "C" PFNGLBINDBUFFERPROC __imp_glBindBuffer = glBindBuffer;

extern "C" void APIENTRY glBindBufferBase(GLenum target, GLuint index, GLuint buffer)
{
    using Proc = PFNGLBINDBUFFERBASEPROC;
    static Proc proc = load_gl_proc<Proc>("glBindBufferBase");
    if (!proc)
    {
        return;
    }
    proc(target, index, buffer);
}
extern "C" PFNGLBINDBUFFERBASEPROC __imp_glBindBufferBase = glBindBufferBase;

extern "C" void APIENTRY glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size)
{
    using Proc = PFNGLBINDBUFFERRANGEPROC;
    static Proc proc = load_gl_proc<Proc>("glBindBufferRange");
    if (!proc)
    {
        return;
    }
    proc(target, index, buffer, offset, size);
}
extern "C" PFNGLBINDBUFFERRANGEPROC __imp_glBindBufferRange = glBindBufferRange;

extern "C" void APIENTRY glBindFramebuffer(GLenum target, GLuint framebuffer)
{
    using Proc = PFNGLBINDFRAMEBUFFERPROC;
    static Proc proc = load_gl_proc<Proc>("glBindFramebuffer");
    if (!proc)
    {
        return;
    }
    proc(target, framebuffer);
}
extern "C" PFNGLBINDFRAMEBUFFERPROC __imp_glBindFramebuffer = glBindFramebuffer;

extern "C" void APIENTRY glBindRenderbuffer(GLenum target, GLuint renderbuffer)
{
    using Proc = PFNGLBINDRENDERBUFFERPROC;
    static Proc proc = load_gl_proc<Proc>("glBindRenderbuffer");
    if (!proc)
    {
        return;
    }
    proc(target, renderbuffer);
}
extern "C" PFNGLBINDRENDERBUFFERPROC __imp_glBindRenderbuffer = glBindRenderbuffer;

extern "C" void APIENTRY glBindVertexArray(GLuint array)
{
    using Proc = PFNGLBINDVERTEXARRAYPROC;
    static Proc proc = load_gl_proc<Proc>("glBindVertexArray");
    if (!proc)
    {
        return;
    }
    proc(array);
}
extern "C" PFNGLBINDVERTEXARRAYPROC __imp_glBindVertexArray = glBindVertexArray;

extern "C" void APIENTRY glBlitFramebuffer(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter)
{
    using Proc = PFNGLBLITFRAMEBUFFERPROC;
    static Proc proc = load_gl_proc<Proc>("glBlitFramebuffer");
    if (!proc)
    {
        return;
    }
    proc(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}
extern "C" PFNGLBLITFRAMEBUFFERPROC __imp_glBlitFramebuffer = glBlitFramebuffer;

extern "C" void APIENTRY glBufferData(GLenum target, GLsizeiptr size, const void *data, GLenum usage)
{
    using Proc = PFNGLBUFFERDATAPROC;
    static Proc proc = load_gl_proc<Proc>("glBufferData");
    if (!proc)
    {
        return;
    }
    proc(target, size, data, usage);
}
extern "C" PFNGLBUFFERDATAPROC __imp_glBufferData = glBufferData;

extern "C" void APIENTRY glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data)
{
    using Proc = PFNGLBUFFERSUBDATAPROC;
    static Proc proc = load_gl_proc<Proc>("glBufferSubData");
    if (!proc)
    {
        return;
    }
    proc(target, offset, size, data);
}
extern "C" PFNGLBUFFERSUBDATAPROC __imp_glBufferSubData = glBufferSubData;

extern "C" GLenum APIENTRY glCheckFramebufferStatus(GLenum target)
{
    using Proc = PFNGLCHECKFRAMEBUFFERSTATUSPROC;
    static Proc proc = load_gl_proc<Proc>("glCheckFramebufferStatus");
    if (!proc)
    {
        return (GLenum)0;
    }
    return proc(target);
}
extern "C" PFNGLCHECKFRAMEBUFFERSTATUSPROC __imp_glCheckFramebufferStatus = glCheckFramebufferStatus;

extern "C" void APIENTRY glCompileShader(GLuint shader)
{
    using Proc = PFNGLCOMPILESHADERPROC;
    static Proc proc = load_gl_proc<Proc>("glCompileShader");
    if (!proc)
    {
        return;
    }
    proc(shader);
}
extern "C" PFNGLCOMPILESHADERPROC __imp_glCompileShader = glCompileShader;

extern "C" GLuint APIENTRY glCreateProgram(void)
{
    using Proc = PFNGLCREATEPROGRAMPROC;
    static Proc proc = load_gl_proc<Proc>("glCreateProgram");
    if (!proc)
    {
        return (GLuint)0;
    }
    return proc();
}
extern "C" PFNGLCREATEPROGRAMPROC __imp_glCreateProgram = glCreateProgram;

extern "C" GLuint APIENTRY glCreateShader(GLenum type)
{
    using Proc = PFNGLCREATESHADERPROC;
    static Proc proc = load_gl_proc<Proc>("glCreateShader");
    if (!proc)
    {
        return (GLuint)0;
    }
    return proc(type);
}
extern "C" PFNGLCREATESHADERPROC __imp_glCreateShader = glCreateShader;

extern "C" void APIENTRY glDeleteBuffers(GLsizei n, const GLuint *buffers)
{
    using Proc = PFNGLDELETEBUFFERSPROC;
    static Proc proc = load_gl_proc<Proc>("glDeleteBuffers");
    if (!proc)
    {
        return;
    }
    proc(n, buffers);
}
extern "C" PFNGLDELETEBUFFERSPROC __imp_glDeleteBuffers = glDeleteBuffers;

extern "C" void APIENTRY glDeleteFramebuffers(GLsizei n, const GLuint *framebuffers)
{
    using Proc = PFNGLDELETEFRAMEBUFFERSPROC;
    static Proc proc = load_gl_proc<Proc>("glDeleteFramebuffers");
    if (!proc)
    {
        return;
    }
    proc(n, framebuffers);
}
extern "C" PFNGLDELETEFRAMEBUFFERSPROC __imp_glDeleteFramebuffers = glDeleteFramebuffers;

extern "C" void APIENTRY glDeleteProgram(GLuint program)
{
    using Proc = PFNGLDELETEPROGRAMPROC;
    static Proc proc = load_gl_proc<Proc>("glDeleteProgram");
    if (!proc)
    {
        return;
    }
    proc(program);
}
extern "C" PFNGLDELETEPROGRAMPROC __imp_glDeleteProgram = glDeleteProgram;

extern "C" void APIENTRY glDeleteQueries(GLsizei n, const GLuint *ids)
{
    using Proc = PFNGLDELETEQUERIESPROC;
    static Proc proc = load_gl_proc<Proc>("glDeleteQueries");
    if (!proc)
    {
        return;
    }
    proc(n, ids);
}
extern "C" PFNGLDELETEQUERIESPROC __imp_glDeleteQueries = glDeleteQueries;

extern "C" void APIENTRY glDeleteRenderbuffers(GLsizei n, const GLuint *renderbuffers)
{
    using Proc = PFNGLDELETERENDERBUFFERSPROC;
    static Proc proc = load_gl_proc<Proc>("glDeleteRenderbuffers");
    if (!proc)
    {
        return;
    }
    proc(n, renderbuffers);
}
extern "C" PFNGLDELETERENDERBUFFERSPROC __imp_glDeleteRenderbuffers = glDeleteRenderbuffers;

extern "C" void APIENTRY glDeleteShader(GLuint shader)
{
    using Proc = PFNGLDELETESHADERPROC;
    static Proc proc = load_gl_proc<Proc>("glDeleteShader");
    if (!proc)
    {
        return;
    }
    proc(shader);
}
extern "C" PFNGLDELETESHADERPROC __imp_glDeleteShader = glDeleteShader;

extern "C" void APIENTRY glDeleteVertexArrays(GLsizei n, const GLuint *arrays)
{
    using Proc = PFNGLDELETEVERTEXARRAYSPROC;
    static Proc proc = load_gl_proc<Proc>("glDeleteVertexArrays");
    if (!proc)
    {
        return;
    }
    proc(n, arrays);
}
extern "C" PFNGLDELETEVERTEXARRAYSPROC __imp_glDeleteVertexArrays = glDeleteVertexArrays;

extern "C" void APIENTRY glDetachShader(GLuint program, GLuint shader)
{
    using Proc = PFNGLDETACHSHADERPROC;
    static Proc proc = load_gl_proc<Proc>("glDetachShader");
    if (!proc)
    {
        return;
    }
    proc(program, shader);
}
extern "C" PFNGLDETACHSHADERPROC __imp_glDetachShader = glDetachShader;

extern "C" void APIENTRY glDisableVertexAttribArray(GLuint index)
{
    using Proc = PFNGLDISABLEVERTEXATTRIBARRAYPROC;
    static Proc proc = load_gl_proc<Proc>("glDisableVertexAttribArray");
    if (!proc)
    {
        return;
    }
    proc(index);
}
extern "C" PFNGLDISABLEVERTEXATTRIBARRAYPROC __imp_glDisableVertexAttribArray = glDisableVertexAttribArray;

extern "C" void APIENTRY glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instancecount)
{
    using Proc = PFNGLDRAWARRAYSINSTANCEDPROC;
    static Proc proc = load_gl_proc<Proc>("glDrawArraysInstanced");
    if (!proc)
    {
        return;
    }
    proc(mode, first, count, instancecount);
}
extern "C" PFNGLDRAWARRAYSINSTANCEDPROC __imp_glDrawArraysInstanced = glDrawArraysInstanced;

extern "C" void APIENTRY glDrawBuffers(GLsizei n, const GLenum *bufs)
{
    using Proc = PFNGLDRAWBUFFERSPROC;
    static Proc proc = load_gl_proc<Proc>("glDrawBuffers");
    if (!proc)
    {
        return;
    }
    proc(n, bufs);
}
extern "C" PFNGLDRAWBUFFERSPROC __imp_glDrawBuffers = glDrawBuffers;

extern "C" void APIENTRY glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount)
{
    using Proc = PFNGLDRAWELEMENTSINSTANCEDPROC;
    static Proc proc = load_gl_proc<Proc>("glDrawElementsInstanced");
    if (!proc)
    {
        return;
    }
    proc(mode, count, type, indices, instancecount);
}
extern "C" PFNGLDRAWELEMENTSINSTANCEDPROC __imp_glDrawElementsInstanced = glDrawElementsInstanced;

extern "C" void APIENTRY glEnableVertexAttribArray(GLuint index)
{
    using Proc = PFNGLENABLEVERTEXATTRIBARRAYPROC;
    static Proc proc = load_gl_proc<Proc>("glEnableVertexAttribArray");
    if (!proc)
    {
        return;
    }
    proc(index);
}
extern "C" PFNGLENABLEVERTEXATTRIBARRAYPROC __imp_glEnableVertexAttribArray = glEnableVertexAttribArray;

extern "C" void APIENTRY glEndQuery(GLenum target)
{
    using Proc = PFNGLENDQUERYPROC;
    static Proc proc = load_gl_proc<Proc>("glEndQuery");
    if (!proc)
    {
        return;
    }
    proc(target);
}
extern "C" PFNGLENDQUERYPROC __imp_glEndQuery = glEndQuery;

extern "C" void APIENTRY glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
    using Proc = PFNGLFRAMEBUFFERRENDERBUFFERPROC;
    static Proc proc = load_gl_proc<Proc>("glFramebufferRenderbuffer");
    if (!proc)
    {
        return;
    }
    proc(target, attachment, renderbuffertarget, renderbuffer);
}
extern "C" PFNGLFRAMEBUFFERRENDERBUFFERPROC __imp_glFramebufferRenderbuffer = glFramebufferRenderbuffer;

extern "C" void APIENTRY glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    using Proc = PFNGLFRAMEBUFFERTEXTURE2DPROC;
    static Proc proc = load_gl_proc<Proc>("glFramebufferTexture2D");
    if (!proc)
    {
        return;
    }
    proc(target, attachment, textarget, texture, level);
}
extern "C" PFNGLFRAMEBUFFERTEXTURE2DPROC __imp_glFramebufferTexture2D = glFramebufferTexture2D;

extern "C" void APIENTRY glGenBuffers(GLsizei n, GLuint *buffers)
{
    using Proc = PFNGLGENBUFFERSPROC;
    static Proc proc = load_gl_proc<Proc>("glGenBuffers");
    if (!proc)
    {
        return;
    }
    proc(n, buffers);
}
extern "C" PFNGLGENBUFFERSPROC __imp_glGenBuffers = glGenBuffers;

extern "C" void APIENTRY glGenFramebuffers(GLsizei n, GLuint *framebuffers)
{
    using Proc = PFNGLGENFRAMEBUFFERSPROC;
    static Proc proc = load_gl_proc<Proc>("glGenFramebuffers");
    if (!proc)
    {
        return;
    }
    proc(n, framebuffers);
}
extern "C" PFNGLGENFRAMEBUFFERSPROC __imp_glGenFramebuffers = glGenFramebuffers;

extern "C" void APIENTRY glGenQueries(GLsizei n, GLuint *ids)
{
    using Proc = PFNGLGENQUERIESPROC;
    static Proc proc = load_gl_proc<Proc>("glGenQueries");
    if (!proc)
    {
        return;
    }
    proc(n, ids);
}
extern "C" PFNGLGENQUERIESPROC __imp_glGenQueries = glGenQueries;

extern "C" void APIENTRY glGenRenderbuffers(GLsizei n, GLuint *renderbuffers)
{
    using Proc = PFNGLGENRENDERBUFFERSPROC;
    static Proc proc = load_gl_proc<Proc>("glGenRenderbuffers");
    if (!proc)
    {
        return;
    }
    proc(n, renderbuffers);
}
extern "C" PFNGLGENRENDERBUFFERSPROC __imp_glGenRenderbuffers = glGenRenderbuffers;

extern "C" void APIENTRY glGenVertexArrays(GLsizei n, GLuint *arrays)
{
    using Proc = PFNGLGENVERTEXARRAYSPROC;
    static Proc proc = load_gl_proc<Proc>("glGenVertexArrays");
    if (!proc)
    {
        return;
    }
    proc(n, arrays);
}
extern "C" PFNGLGENVERTEXARRAYSPROC __imp_glGenVertexArrays = glGenVertexArrays;

extern "C" void APIENTRY glGenerateMipmap(GLenum target)
{
    using Proc = PFNGLGENERATEMIPMAPPROC;
    static Proc proc = load_gl_proc<Proc>("glGenerateMipmap");
    if (!proc)
    {
        return;
    }
    proc(target);
}
extern "C" PFNGLGENERATEMIPMAPPROC __imp_glGenerateMipmap = glGenerateMipmap;

extern "C" void APIENTRY glGetAttachedShaders(GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders)
{
    using Proc = PFNGLGETATTACHEDSHADERSPROC;
    static Proc proc = load_gl_proc<Proc>("glGetAttachedShaders");
    if (!proc)
    {
        return;
    }
    proc(program, maxCount, count, shaders);
}
extern "C" PFNGLGETATTACHEDSHADERSPROC __imp_glGetAttachedShaders = glGetAttachedShaders;

extern "C" GLint APIENTRY glGetAttribLocation(GLuint program, const GLchar *name)
{
    using Proc = PFNGLGETATTRIBLOCATIONPROC;
    static Proc proc = load_gl_proc<Proc>("glGetAttribLocation");
    if (!proc)
    {
        return (GLint)0;
    }
    return proc(program, name);
}
extern "C" PFNGLGETATTRIBLOCATIONPROC __imp_glGetAttribLocation = glGetAttribLocation;

extern "C" void APIENTRY glGetProgramInfoLog(GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog)
{
    using Proc = PFNGLGETPROGRAMINFOLOGPROC;
    static Proc proc = load_gl_proc<Proc>("glGetProgramInfoLog");
    if (!proc)
    {
        return;
    }
    proc(program, bufSize, length, infoLog);
}
extern "C" PFNGLGETPROGRAMINFOLOGPROC __imp_glGetProgramInfoLog = glGetProgramInfoLog;

extern "C" void APIENTRY glGetProgramiv(GLuint program, GLenum pname, GLint *params)
{
    using Proc = PFNGLGETPROGRAMIVPROC;
    static Proc proc = load_gl_proc<Proc>("glGetProgramiv");
    if (!proc)
    {
        return;
    }
    proc(program, pname, params);
}
extern "C" PFNGLGETPROGRAMIVPROC __imp_glGetProgramiv = glGetProgramiv;

extern "C" void APIENTRY glGetQueryObjectiv(GLuint id, GLenum pname, GLint *params)
{
    using Proc = PFNGLGETQUERYOBJECTIVPROC;
    static Proc proc = load_gl_proc<Proc>("glGetQueryObjectiv");
    if (!proc)
    {
        return;
    }
    proc(id, pname, params);
}
extern "C" PFNGLGETQUERYOBJECTIVPROC __imp_glGetQueryObjectiv = glGetQueryObjectiv;

extern "C" void APIENTRY glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint *params)
{
    using Proc = PFNGLGETQUERYOBJECTUIVPROC;
    static Proc proc = load_gl_proc<Proc>("glGetQueryObjectuiv");
    if (!proc)
    {
        return;
    }
    proc(id, pname, params);
}
extern "C" PFNGLGETQUERYOBJECTUIVPROC __imp_glGetQueryObjectuiv = glGetQueryObjectuiv;

extern "C" void APIENTRY glGetShaderInfoLog(GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog)
{
    using Proc = PFNGLGETSHADERINFOLOGPROC;
    static Proc proc = load_gl_proc<Proc>("glGetShaderInfoLog");
    if (!proc)
    {
        return;
    }
    proc(shader, bufSize, length, infoLog);
}
extern "C" PFNGLGETSHADERINFOLOGPROC __imp_glGetShaderInfoLog = glGetShaderInfoLog;

extern "C" void APIENTRY glGetShaderiv(GLuint shader, GLenum pname, GLint *params)
{
    using Proc = PFNGLGETSHADERIVPROC;
    static Proc proc = load_gl_proc<Proc>("glGetShaderiv");
    if (!proc)
    {
        return;
    }
    proc(shader, pname, params);
}
extern "C" PFNGLGETSHADERIVPROC __imp_glGetShaderiv = glGetShaderiv;

extern "C" GLuint APIENTRY glGetUniformBlockIndex(GLuint program, const GLchar *uniformBlockName)
{
    using Proc = PFNGLGETUNIFORMBLOCKINDEXPROC;
    static Proc proc = load_gl_proc<Proc>("glGetUniformBlockIndex");
    if (!proc)
    {
        return (GLuint)0;
    }
    return proc(program, uniformBlockName);
}
extern "C" PFNGLGETUNIFORMBLOCKINDEXPROC __imp_glGetUniformBlockIndex = glGetUniformBlockIndex;

extern "C" GLint APIENTRY glGetUniformLocation(GLuint program, const GLchar *name)
{
    using Proc = PFNGLGETUNIFORMLOCATIONPROC;
    static Proc proc = load_gl_proc<Proc>("glGetUniformLocation");
    if (!proc)
    {
        return (GLint)0;
    }
    return proc(program, name);
}
extern "C" PFNGLGETUNIFORMLOCATIONPROC __imp_glGetUniformLocation = glGetUniformLocation;

extern "C" GLboolean APIENTRY glIsQuery(GLuint id)
{
    using Proc = PFNGLISQUERYPROC;
    static Proc proc = load_gl_proc<Proc>("glIsQuery");
    if (!proc)
    {
        return (GLboolean)0;
    }
    return proc(id);
}
extern "C" PFNGLISQUERYPROC __imp_glIsQuery = glIsQuery;

extern "C" GLboolean APIENTRY glIsVertexArray(GLuint array)
{
    using Proc = PFNGLISVERTEXARRAYPROC;
    static Proc proc = load_gl_proc<Proc>("glIsVertexArray");
    if (!proc)
    {
        return (GLboolean)0;
    }
    return proc(array);
}
extern "C" PFNGLISVERTEXARRAYPROC __imp_glIsVertexArray = glIsVertexArray;

extern "C" void APIENTRY glLinkProgram(GLuint program)
{
    using Proc = PFNGLLINKPROGRAMPROC;
    static Proc proc = load_gl_proc<Proc>("glLinkProgram");
    if (!proc)
    {
        return;
    }
    proc(program);
}
extern "C" PFNGLLINKPROGRAMPROC __imp_glLinkProgram = glLinkProgram;

extern "C" void * APIENTRY glMapBuffer(GLenum target, GLenum access)
{
    using Proc = PFNGLMAPBUFFERPROC;
    static Proc proc = load_gl_proc<Proc>("glMapBuffer");
    if (!proc)
    {
        return (void *)0;
    }
    return proc(target, access);
}
extern "C" PFNGLMAPBUFFERPROC __imp_glMapBuffer = glMapBuffer;

extern "C" void APIENTRY glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
    using Proc = PFNGLRENDERBUFFERSTORAGEPROC;
    static Proc proc = load_gl_proc<Proc>("glRenderbufferStorage");
    if (!proc)
    {
        return;
    }
    proc(target, internalformat, width, height);
}
extern "C" PFNGLRENDERBUFFERSTORAGEPROC __imp_glRenderbufferStorage = glRenderbufferStorage;

extern "C" void APIENTRY glRenderbufferStorageMultisample(GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height)
{
    using Proc = PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC;
    static Proc proc = load_gl_proc<Proc>("glRenderbufferStorageMultisample");
    if (!proc)
    {
        return;
    }
    proc(target, samples, internalformat, width, height);
}
extern "C" PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC __imp_glRenderbufferStorageMultisample = glRenderbufferStorageMultisample;

extern "C" void APIENTRY glShaderSource(GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length)
{
    using Proc = PFNGLSHADERSOURCEPROC;
    static Proc proc = load_gl_proc<Proc>("glShaderSource");
    if (!proc)
    {
        return;
    }
    proc(shader, count, string, length);
}
extern "C" PFNGLSHADERSOURCEPROC __imp_glShaderSource = glShaderSource;

extern "C" void APIENTRY glUniform1f(GLint location, GLfloat v0)
{
    using Proc = PFNGLUNIFORM1FPROC;
    static Proc proc = load_gl_proc<Proc>("glUniform1f");
    if (!proc)
    {
        return;
    }
    proc(location, v0);
}
extern "C" PFNGLUNIFORM1FPROC __imp_glUniform1f = glUniform1f;

extern "C" void APIENTRY glUniform1i(GLint location, GLint v0)
{
    using Proc = PFNGLUNIFORM1IPROC;
    static Proc proc = load_gl_proc<Proc>("glUniform1i");
    if (!proc)
    {
        return;
    }
    proc(location, v0);
}
extern "C" PFNGLUNIFORM1IPROC __imp_glUniform1i = glUniform1i;

extern "C" void APIENTRY glUniform2f(GLint location, GLfloat v0, GLfloat v1)
{
    using Proc = PFNGLUNIFORM2FPROC;
    static Proc proc = load_gl_proc<Proc>("glUniform2f");
    if (!proc)
    {
        return;
    }
    proc(location, v0, v1);
}
extern "C" PFNGLUNIFORM2FPROC __imp_glUniform2f = glUniform2f;

extern "C" void APIENTRY glUniform3f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2)
{
    using Proc = PFNGLUNIFORM3FPROC;
    static Proc proc = load_gl_proc<Proc>("glUniform3f");
    if (!proc)
    {
        return;
    }
    proc(location, v0, v1, v2);
}
extern "C" PFNGLUNIFORM3FPROC __imp_glUniform3f = glUniform3f;

extern "C" void APIENTRY glUniform4f(GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3)
{
    using Proc = PFNGLUNIFORM4FPROC;
    static Proc proc = load_gl_proc<Proc>("glUniform4f");
    if (!proc)
    {
        return;
    }
    proc(location, v0, v1, v2, v3);
}
extern "C" PFNGLUNIFORM4FPROC __imp_glUniform4f = glUniform4f;

extern "C" void APIENTRY glUniformBlockBinding(GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding)
{
    using Proc = PFNGLUNIFORMBLOCKBINDINGPROC;
    static Proc proc = load_gl_proc<Proc>("glUniformBlockBinding");
    if (!proc)
    {
        return;
    }
    proc(program, uniformBlockIndex, uniformBlockBinding);
}
extern "C" PFNGLUNIFORMBLOCKBINDINGPROC __imp_glUniformBlockBinding = glUniformBlockBinding;

extern "C" void APIENTRY glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value)
{
    using Proc = PFNGLUNIFORMMATRIX4FVPROC;
    static Proc proc = load_gl_proc<Proc>("glUniformMatrix4fv");
    if (!proc)
    {
        return;
    }
    proc(location, count, transpose, value);
}
extern "C" PFNGLUNIFORMMATRIX4FVPROC __imp_glUniformMatrix4fv = glUniformMatrix4fv;

extern "C" GLboolean APIENTRY glUnmapBuffer(GLenum target)
{
    using Proc = PFNGLUNMAPBUFFERPROC;
    static Proc proc = load_gl_proc<Proc>("glUnmapBuffer");
    if (!proc)
    {
        return (GLboolean)0;
    }
    return proc(target);
}
extern "C" PFNGLUNMAPBUFFERPROC __imp_glUnmapBuffer = glUnmapBuffer;

extern "C" void APIENTRY glUseProgram(GLuint program)
{
    using Proc = PFNGLUSEPROGRAMPROC;
    static Proc proc = load_gl_proc<Proc>("glUseProgram");
    if (!proc)
    {
        return;
    }
    proc(program);
}
extern "C" PFNGLUSEPROGRAMPROC __imp_glUseProgram = glUseProgram;

extern "C" void APIENTRY glValidateProgram(GLuint program)
{
    using Proc = PFNGLVALIDATEPROGRAMPROC;
    static Proc proc = load_gl_proc<Proc>("glValidateProgram");
    if (!proc)
    {
        return;
    }
    proc(program);
}
extern "C" PFNGLVALIDATEPROGRAMPROC __imp_glValidateProgram = glValidateProgram;

extern "C" void APIENTRY glVertexAttribDivisor(GLuint index, GLuint divisor)
{
    using Proc = PFNGLVERTEXATTRIBDIVISORPROC;
    static Proc proc = load_gl_proc<Proc>("glVertexAttribDivisor");
    if (!proc)
    {
        return;
    }
    proc(index, divisor);
}
extern "C" PFNGLVERTEXATTRIBDIVISORPROC __imp_glVertexAttribDivisor = glVertexAttribDivisor;

extern "C" void APIENTRY glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer)
{
    using Proc = PFNGLVERTEXATTRIBPOINTERPROC;
    static Proc proc = load_gl_proc<Proc>("glVertexAttribPointer");
    if (!proc)
    {
        return;
    }
    proc(index, size, type, normalized, stride, pointer);
}
extern "C" PFNGLVERTEXATTRIBPOINTERPROC __imp_glVertexAttribPointer = glVertexAttribPointer;

#endif
