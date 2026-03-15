#include "pch.h"
#include "batch.hpp"
#include "raymath.h"
#include "utils.hpp"

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL_opengl.h>
#include <SDL2/SDL_opengl_glext.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <limits>

namespace
{
    static constexpr int LINES = 0x0001;
    static constexpr int TRIANGLES = 0x0004;
    static constexpr int QUAD = 0x0008;

    static unsigned char floatToUnsignedChar(float value)
    {
        float normalizedValue = (value < 0.0f) ? 0.0f : ((value > 1.0f) ? 1.0f : value);
        float scaledValue = normalizedValue * 255.0f;
        if (scaledValue < 0.0f)
            scaledValue = 0.0f;
        if (scaledValue > 255.0f)
            scaledValue = 255.0f;
        return (unsigned char)scaledValue;
    }

    static GLenum to_gl_mode(int mode)
    {
        if (mode == LINES)
            return GL_LINES;
        if (mode == TRIANGLES)
            return GL_TRIANGLES;
        return GL_TRIANGLES;
    }

    static GLuint compile_shader(GLenum type, const char *source)
    {
        GLuint shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);

        GLint ok = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
        if (!ok)
        {
            char log[2048];
            GLsizei written = 0;
            glGetShaderInfoLog(shader, (GLsizei)sizeof(log), &written, log);
            LogError("[Batch] shader compile failed: %.*s", (int)written, log);
            glDeleteShader(shader);
            return 0;
        }

        return shader;
    }

    static Vector2 make_vec2(float x, float y)
    {
        Vector2 v = {x, y};
        return v;
    }

    static Vector3 make_vec3(float x, float y, float z)
    {
        Vector3 v = {x, y, z};
        return v;
    }
}

Color::Color() : r(255), g(255), b(255), a(255) {}
Color::Color(u8 pr, u8 pg, u8 pb, u8 pa) : r(pr), g(pg), b(pb), a(pa) {}
Color::Color(float pr, float pg, float pb, float pa)
    : r(floatToUnsignedChar(pr)), g(floatToUnsignedChar(pg)), b(floatToUnsignedChar(pb)), a(floatToUnsignedChar(pa)) {}

RenderBatch::RenderBatch()
    : maxVertices(0),
      currentMode(QUAD),
      currentTexture(0),
      currentDepth(0.0f),
      texcoordx(0.0f),
      texcoordy(0.0f),
      colorr(255),
      colorg(255),
      colorb(255),
      colora(255),
      use_matrix(false),
      programId(0),
      vboId(0),
      eboId(0),
      whiteTextureId(0),
      uMvpLocation(-1),
      uTextureLocation(-1),
      aPosLocation(-1),
      aUvLocation(-1),
      aColorLocation(-1),
      gpuReady(false)
{
}

RenderBatch::~RenderBatch()
{
    Release();
}

void RenderBatch::Init(int numBuffers, int bufferElements)
{
    (void)numBuffers;
    maxVertices = std::max(bufferElements * 4, 256);
    vertices.clear();
    draws.clear();
    vertices.reserve((std::size_t)maxVertices);
    draws.reserve(1024);
    currentMode = QUAD;
    currentTexture = 0;
    currentDepth = 0.0f;
    texcoordx = 0.0f;
    texcoordy = 0.0f;
    colorr = 255;
    colorg = 255;
    colorb = 255;
    colora = 255;
    use_matrix = false;
    modelMatrix = MatrixIdentity();
    viewMatrix = MatrixIdentity();
    createDeviceObjects();
}

void RenderBatch::Release()
{
    destroyDeviceObjects();
    vertices.clear();
    draws.clear();
    quadIndices.clear();
    maxVertices = 0;
    currentTexture = 0;
    currentMode = QUAD;
    use_matrix = false;
    modelMatrix = MatrixIdentity();
    viewMatrix = MatrixIdentity();
}

bool RenderBatch::compileShaderProgram()
{
    static const char *kVertexShader =
        "#version 120\n"
        "attribute vec3 aPos;\n"
        "attribute vec2 aUV;\n"
        "attribute vec4 aColor;\n"
        "uniform mat4 uMVP;\n"
        "varying vec2 vUV;\n"
        "varying vec4 vColor;\n"
        "void main() {\n"
        "    vUV = aUV;\n"
        "    vColor = aColor;\n"
        "    gl_Position = uMVP * vec4(aPos, 1.0);\n"
        "}\n";

    static const char *kFragmentShader =
        "#version 120\n"
        "uniform sampler2D uTexture;\n"
        "varying vec2 vUV;\n"
        "varying vec4 vColor;\n"
        "void main() {\n"
        "    gl_FragColor = texture2D(uTexture, vUV) * vColor;\n"
        "}\n";

    GLuint vs = compile_shader(GL_VERTEX_SHADER, kVertexShader);
    if (!vs)
        return false;

    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, kFragmentShader);
    if (!fs)
    {
        glDeleteShader(vs);
        return false;
    }

    programId = glCreateProgram();
    glAttachShader(programId, vs);
    glAttachShader(programId, fs);
    glBindAttribLocation(programId, 0, "aPos");
    glBindAttribLocation(programId, 1, "aUV");
    glBindAttribLocation(programId, 2, "aColor");
    glLinkProgram(programId);

    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint linked = 0;
    glGetProgramiv(programId, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        char log[2048];
        GLsizei written = 0;
        glGetProgramInfoLog(programId, (GLsizei)sizeof(log), &written, log);
        LogError("[Batch] program link failed: %.*s", (int)written, log);
        glDeleteProgram(programId);
        programId = 0;
        return false;
    }

    aPosLocation = 0;
    aUvLocation = 1;
    aColorLocation = 2;
    uMvpLocation = glGetUniformLocation(programId, "uMVP");
    uTextureLocation = glGetUniformLocation(programId, "uTexture");
    return true;
}

bool RenderBatch::createDeviceObjects()
{
    destroyDeviceObjects();

    if (!compileShaderProgram())
        return false;

    glGenBuffers(1, &vboId);
    glGenBuffers(1, &eboId);
    if (vboId == 0 || eboId == 0)
    {
        LogError("[Batch] failed to create VBO/EBO");
        destroyDeviceObjects();
        return false;
    }

    glGenTextures(1, &whiteTextureId);
    if (whiteTextureId == 0)
    {
        LogError("[Batch] failed to create default texture");
        destroyDeviceObjects();
        return false;
    }

    const unsigned char white[4] = {255, 255, 255, 255};
    glBindTexture(GL_TEXTURE_2D, whiteTextureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
    glBindTexture(GL_TEXTURE_2D, 0);

    gpuReady = ensureIndexCapacity((std::size_t)maxVertices);
    // normaliza currentTexture agora que whiteTextureId está definido
    if (currentTexture == 0)
        currentTexture = whiteTextureId;
    return gpuReady;
}

void RenderBatch::destroyDeviceObjects()
{
    if (whiteTextureId != 0)
    {
        glDeleteTextures(1, &whiteTextureId);
        whiteTextureId = 0;
    }

    if (eboId != 0)
    {
        glDeleteBuffers(1, &eboId);
        eboId = 0;
    }

    if (vboId != 0)
    {
        glDeleteBuffers(1, &vboId);
        vboId = 0;
    }

    if (programId != 0)
    {
        glDeleteProgram(programId);
        programId = 0;
    }

    uMvpLocation = -1;
    uTextureLocation = -1;
    aPosLocation = -1;
    aUvLocation = -1;
    aColorLocation = -1;
    gpuReady = false;
}


bool RenderBatch::CheckRenderBatchLimit(int vCount)
{
    if (maxVertices > 0 && (int)vertices.size() + vCount > maxVertices)
    {
        int      savedMode = currentMode;
        unsigned savedTex  = currentTexture;

        Render();

        currentMode    = savedMode;
        currentTexture = savedTex;
        return true;
    }
    return false;
}


void RenderBatch::ensureDrawCall()
{
    if (!draws.empty())
    {
        DrawCall &last = draws.back();
        if (last.mode == currentMode && last.textureId == currentTexture)
            return;
    }

    DrawCall call;
    call.mode      = currentMode;
    call.textureId = currentTexture;
    call.first     = (int)vertices.size();
    call.count     = 0;
    draws.push_back(call);
}


void RenderBatch::SetMode(int mode)
{
    if (currentMode == mode)
        return;

    // flush só se há vértices QUAD desalinhados
    if (!vertices.empty() && !draws.empty())
    {
        const bool leavingQuad  = (currentMode == QUAD);
        const bool enteringQuad = (mode == QUAD);

        if ((leavingQuad || enteringQuad) && draws.back().count % 4 != 0)
            Render();
    }

    currentMode = mode;
}


void RenderBatch::SetTexture(unsigned int id)
{
    currentTexture = (id != 0) ? id : whiteTextureId;
}


void RenderBatch::Vertex3f(float x, float y, float z)
{
    CheckRenderBatchLimit(1);
    ensureDrawCall();

    const Vector3 p = transformPoint({x, y, z});

    Vertex v;
    v.x = p.x;
    v.y = p.y;
    v.z = p.z;
    v.u = texcoordx;
    v.v = texcoordy;
    v.r = colorr;
    v.g = colorg;
    v.b = colorb;
    v.a = colora;

    vertices.push_back(v);
    draws.back().count++;
}
void RenderBatch::Render()
{
    if (vertices.empty() || draws.empty())
        return;

    if (!gpuReady && !createDeviceObjects())
        return;

    if (!ensureIndexCapacity(vertices.size()))
        return;

    glUseProgram(programId);
    if (uMvpLocation >= 0)
    {
        const float16 mvp = MatrixToFloatV(viewMatrix);
        glUniformMatrix4fv(uMvpLocation, 1, GL_FALSE, mvp.v);
    }
    if (uTextureLocation >= 0)
        glUniform1i(uTextureLocation, 0);

    glBindBuffer(GL_ARRAY_BUFFER, vboId);
    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)(vertices.size() * sizeof(Vertex)),
                 vertices.data(),
                 GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray((GLuint)aPosLocation);
    glEnableVertexAttribArray((GLuint)aUvLocation);
    glEnableVertexAttribArray((GLuint)aColorLocation);
    glVertexAttribPointer((GLuint)aPosLocation,   3, GL_FLOAT,         GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, x));
    glVertexAttribPointer((GLuint)aUvLocation,    2, GL_FLOAT,         GL_FALSE, sizeof(Vertex), (const void*)offsetof(Vertex, u));
    glVertexAttribPointer((GLuint)aColorLocation, 4, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(Vertex), (const void*)offsetof(Vertex, r));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboId);
    glActiveTexture(GL_TEXTURE0);

    unsigned int activeTex = 0xFFFFFFFFu;

    for (std::size_t i = 0; i < draws.size(); ++i)
    {
        const DrawCall &draw = draws[i];
        if (draw.count == 0)
            continue;

        const unsigned int tex = (draw.textureId != 0) ? draw.textureId : whiteTextureId;
        if (tex != activeTex)
        {
            glBindTexture(GL_TEXTURE_2D, tex);
            activeTex = tex;
        }

        if (draw.mode == QUAD)
        {
            const GLint   firstIndex = (GLint)(draw.first / 4) * 6;
            const GLsizei count      = (GLsizei)(draw.count / 4) * 6;
            if (count > 0)
                glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT,
                               (const void*)(firstIndex * sizeof(unsigned int)));
        }
        else
        {
            const GLenum glMode = (draw.mode == LINES) ? GL_LINES : GL_TRIANGLES;
            glDrawArrays(glMode, (GLint)draw.first, (GLsizei)draw.count);
        }
    }

    glDisableVertexAttribArray((GLuint)aPosLocation);
    glDisableVertexAttribArray((GLuint)aUvLocation);
    glDisableVertexAttribArray((GLuint)aColorLocation);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);

    vertices.clear();
    draws.clear();
}


bool RenderBatch::ensureIndexCapacity(std::size_t vertexCount)
{
    if (eboId == 0)
        return false;

    const std::size_t quadCount  = (vertexCount + 3) / 4;
    const std::size_t indexCount = quadCount * 6;
    if (quadIndices.size() >= indexCount)
        return true;

    quadIndices.resize(indexCount);
    for (std::size_t i = 0; i < quadCount; ++i)
    {
        const unsigned int base = (unsigned int)(i * 4);
        const std::size_t  bi   = i * 6;
        quadIndices[bi + 0] = base + 0;
        quadIndices[bi + 1] = base + 1;
        quadIndices[bi + 2] = base + 2;
        quadIndices[bi + 3] = base + 0;
        quadIndices[bi + 4] = base + 2;
        quadIndices[bi + 5] = base + 3;
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboId);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 (GLsizeiptr)(quadIndices.size() * sizeof(unsigned int)),
                 quadIndices.data(),
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    return true;
}

  
Vector3 RenderBatch::transformPoint(const Vector3 &point) const
{
    if (!use_matrix)
        return point;

    Vector3 result;
    result.x = modelMatrix.m0 * point.x + modelMatrix.m4 * point.y + modelMatrix.m8  * point.z + modelMatrix.m12;
    result.y = modelMatrix.m1 * point.x + modelMatrix.m5 * point.y + modelMatrix.m9  * point.z + modelMatrix.m13;
    result.z = modelMatrix.m2 * point.x + modelMatrix.m6 * point.y + modelMatrix.m10 * point.z + modelMatrix.m14;
    return result;
}

 

void RenderBatch::SetMatrix(const Matrix &matrix)
{
    viewMatrix = matrix;
}

void RenderBatch::BeginTransform(const Matrix &transform)
{
    modelMatrix = transform;
    use_matrix = true;
}

void RenderBatch::EndTransform()
{
    use_matrix = false;
}

void RenderBatch::TexCoord2f(float x, float y)
{
    texcoordx = x;
    texcoordy = y;
}
 

void RenderBatch::Vertex2f(float x, float y)
{
    Vertex3f(x, y, currentDepth);
}

void RenderBatch::SetColor(const Color &color)
{
    colorr = color.r;
    colorg = color.g;
    colorb = color.b;
    colora = color.a;
}

void RenderBatch::SetColor(float r, float g, float b)
{
    colorr = floatToUnsignedChar(r);
    colorg = floatToUnsignedChar(g);
    colorb = floatToUnsignedChar(b);
}

void RenderBatch::SetColor(u8 r, u8 g, u8 b, u8 a)
{
    colorr = r;
    colorg = g;
    colorb = b;
    colora = a;
}

void RenderBatch::SetAlpha(float a)
{
    colora = floatToUnsignedChar(a);
} 

void RenderBatch::Line2D(int startPosX, int startPosY, int endPosX, int endPosY)
{
    SetTexture(0);
    SetMode(LINES);
    Vertex2f((float)startPosX, (float)startPosY);
    Vertex2f((float)endPosX, (float)endPosY);
}

void RenderBatch::Line2D(const Vector2 &start, const Vector2 &end)
{
    SetTexture(0);
    SetMode(LINES);
    Vertex2f(start.x, start.y);
    Vertex2f(end.x, end.y);
}

void RenderBatch::Line3D(const Vector3 &start, const Vector3 &end)
{
    SetTexture(0);
    SetMode(LINES);
    Vertex3f(start.x, start.y, start.z);
    Vertex3f(end.x, end.y, end.z);
}

void RenderBatch::Line3D(float startX, float startY, float startZ, float endX, float endY, float endZ)
{
    SetTexture(0);
    SetMode(LINES);
    Vertex3f(startX, startY, startZ);
    Vertex3f(endX, endY, endZ);
}

void RenderBatch::Circle(int centerX, int centerY, float radius, bool fill)
{
    SetTexture(0);
    const int segments = std::max(18, std::min(96, (int)(radius * 0.8f)));
    const float step = (2.0f * PI) / (float)segments;
    const float cx = (float)centerX;
    const float cy = (float)centerY;

    if (fill)
    {
        SetMode(TRIANGLES);
        for (int i = 0; i < segments; ++i)
        {
            const float a0 = step * i;
            const float a1 = step * (i + 1);
            Vertex2f(cx, cy);
            Vertex2f(cx + cosf(a0) * radius, cy + sinf(a0) * radius);
            Vertex2f(cx + cosf(a1) * radius, cy + sinf(a1) * radius);
        }
    }
    else
    {
        SetMode(LINES);
        for (int i = 0; i < segments; ++i)
        {
            const float a0 = step * i;
            const float a1 = step * (i + 1);
            Vertex2f(cx + cosf(a0) * radius, cy + sinf(a0) * radius);
            Vertex2f(cx + cosf(a1) * radius, cy + sinf(a1) * radius);
        }
    }
}

void RenderBatch::Rectangle(int posX, int posY, int width, int height, bool fill)
{
    SetTexture(0);
    if (fill)
    {
        SetMode(QUAD);
        Vertex2f((float)posX, (float)posY);
        Vertex2f((float)(posX + width), (float)posY);
        Vertex2f((float)(posX + width), (float)(posY + height));
        Vertex2f((float)posX, (float)(posY + height));
    }
    else
    {
        SetMode(LINES);
        Vertex2f((float)posX, (float)posY);
        Vertex2f((float)(posX + width), (float)posY);

        Vertex2f((float)(posX + width), (float)posY);
        Vertex2f((float)(posX + width), (float)(posY + height));

        Vertex2f((float)(posX + width), (float)(posY + height));
        Vertex2f((float)posX, (float)(posY + height));

        Vertex2f((float)posX, (float)(posY + height));
        Vertex2f((float)posX, (float)posY);
    }
}

void RenderBatch::Box(const BoundingBox &box)
{
    SetTexture(0);
    SetMode(LINES);
    Line3D(box.min.x, box.min.y, box.min.z, box.max.x, box.min.y, box.min.z);
    Line3D(box.max.x, box.min.y, box.min.z, box.max.x, box.max.y, box.min.z);
    Line3D(box.max.x, box.max.y, box.min.z, box.min.x, box.max.y, box.min.z);
    Line3D(box.min.x, box.max.y, box.min.z, box.min.x, box.min.y, box.min.z);

    Line3D(box.min.x, box.min.y, box.max.z, box.max.x, box.min.y, box.max.z);
    Line3D(box.max.x, box.min.y, box.max.z, box.max.x, box.max.y, box.max.z);
    Line3D(box.max.x, box.max.y, box.max.z, box.min.x, box.max.y, box.max.z);
    Line3D(box.min.x, box.max.y, box.max.z, box.min.x, box.min.y, box.max.z);

    Line3D(box.min.x, box.min.y, box.min.z, box.min.x, box.min.y, box.max.z);
    Line3D(box.max.x, box.min.y, box.min.z, box.max.x, box.min.y, box.max.z);
    Line3D(box.max.x, box.max.y, box.min.z, box.max.x, box.max.y, box.max.z);
    Line3D(box.min.x, box.max.y, box.min.z, box.min.x, box.max.y, box.max.z);
}

void RenderBatch::Box(const BoundingBox &box, const Matrix &transform)
{
    BeginTransform(transform);
    Box(box);
    EndTransform();
}

void RenderBatch::Cube(const Vector3 &position, float width, float height, float depth, bool wire)
{
    SetTexture(0);
    const float hx = width * 0.5f;
    const float hy = height * 0.5f;
    const float hz = depth * 0.5f;
    BoundingBox box(make_vec3(position.x - hx, position.y - hy, position.z - hz),
                    make_vec3(position.x + hx, position.y + hy, position.z + hz));

    if (wire)
    {
        Box(box);
        return;
    }

    SetMode(TRIANGLES);
    const Vector3 p000 = make_vec3(box.min.x, box.min.y, box.min.z);
    const Vector3 p001 = make_vec3(box.min.x, box.min.y, box.max.z);
    const Vector3 p010 = make_vec3(box.min.x, box.max.y, box.min.z);
    const Vector3 p011 = make_vec3(box.min.x, box.max.y, box.max.z);
    const Vector3 p100 = make_vec3(box.max.x, box.min.y, box.min.z);
    const Vector3 p101 = make_vec3(box.max.x, box.min.y, box.max.z);
    const Vector3 p110 = make_vec3(box.max.x, box.max.y, box.min.z);
    const Vector3 p111 = make_vec3(box.max.x, box.max.y, box.max.z);

    Triangle(p001, p101, p111); Triangle(p001, p111, p011);
    Triangle(p100, p000, p010); Triangle(p100, p010, p110);
    Triangle(p000, p001, p011); Triangle(p000, p011, p010);
    Triangle(p101, p100, p110); Triangle(p101, p110, p111);
    Triangle(p010, p011, p111); Triangle(p010, p111, p110);
    Triangle(p000, p100, p101); Triangle(p000, p101, p001);
}

void RenderBatch::Sphere(const Vector3 &position, float radius, int rings, int slices, bool wire)
{
    SetTexture(0);
    rings = std::max(rings, 3);
    slices = std::max(slices, 4);

    if (wire)
        SetMode(LINES);
    else
        SetMode(TRIANGLES);

    for (int i = 0; i < rings; ++i)
    {
        const float theta1 = (float)i * PI / (float)rings;
        const float theta2 = (float)(i + 1) * PI / (float)rings;

        for (int j = 0; j < slices; ++j)
        {
            const float phi1 = (float)j * 2.0f * PI / (float)slices;
            const float phi2 = (float)(j + 1) * 2.0f * PI / (float)slices;

            const Vector3 v1 = make_vec3(position.x + radius * sinf(theta1) * cosf(phi1), position.y + radius * cosf(theta1), position.z + radius * sinf(theta1) * sinf(phi1));
            const Vector3 v2 = make_vec3(position.x + radius * sinf(theta1) * cosf(phi2), position.y + radius * cosf(theta1), position.z + radius * sinf(theta1) * sinf(phi2));
            const Vector3 v3 = make_vec3(position.x + radius * sinf(theta2) * cosf(phi1), position.y + radius * cosf(theta2), position.z + radius * sinf(theta2) * sinf(phi1));
            const Vector3 v4 = make_vec3(position.x + radius * sinf(theta2) * cosf(phi2), position.y + radius * cosf(theta2), position.z + radius * sinf(theta2) * sinf(phi2));

            if (wire)
            {
                Line3D(v1, v2);
                Line3D(v1, v3);
            }
            else
            {
                Triangle(v1, v2, v3);
                Triangle(v2, v4, v3);
            }
        }
    }
}

void RenderBatch::Cone(const Vector3 &position, float radius, float height, int segments, bool wire)
{
    SetTexture(0);
    segments = std::max(segments, 3);
    const Vector3 apex = make_vec3(position.x, position.y + height * 0.5f, position.z);
    const float baseY = position.y - height * 0.5f;

    if (wire)
        SetMode(LINES);
    else
        SetMode(TRIANGLES);

    for (int i = 0; i < segments; ++i)
    {
        const float a0 = (float)i * 2.0f * PI / (float)segments;
        const float a1 = (float)(i + 1) * 2.0f * PI / (float)segments;
        const Vector3 b0 = make_vec3(position.x + cosf(a0) * radius, baseY, position.z + sinf(a0) * radius);
        const Vector3 b1 = make_vec3(position.x + cosf(a1) * radius, baseY, position.z + sinf(a1) * radius);

        if (wire)
        {
            Line3D(b0, b1);
            Line3D(apex, b0);
        }
        else
        {
            Triangle(apex, b0, b1);
            Triangle(make_vec3(position.x, baseY, position.z), b1, b0);
        }
    }
}

void RenderBatch::Cylinder(const Vector3 &position, float radius, float height, int segments, bool wire)
{
    SetTexture(0);
    segments = std::max(segments, 3);
    const float y0 = position.y - height * 0.5f;
    const float y1 = position.y + height * 0.5f;

    if (wire)
        SetMode(LINES);
    else
        SetMode(TRIANGLES);

    for (int i = 0; i < segments; ++i)
    {
        const float a0 = (float)i * 2.0f * PI / (float)segments;
        const float a1 = (float)(i + 1) * 2.0f * PI / (float)segments;
        const Vector3 b0 = make_vec3(position.x + cosf(a0) * radius, y0, position.z + sinf(a0) * radius);
        const Vector3 b1 = make_vec3(position.x + cosf(a1) * radius, y0, position.z + sinf(a1) * radius);
        const Vector3 t0 = make_vec3(position.x + cosf(a0) * radius, y1, position.z + sinf(a0) * radius);
        const Vector3 t1 = make_vec3(position.x + cosf(a1) * radius, y1, position.z + sinf(a1) * radius);

        if (wire)
        {
            Line3D(b0, b1);
            Line3D(t0, t1);
            Line3D(b0, t0);
        }
        else
        {
            Triangle(b0, b1, t1);
            Triangle(b0, t1, t0);
            Triangle(make_vec3(position.x, y0, position.z), b1, b0);
            Triangle(make_vec3(position.x, y1, position.z), t0, t1);
        }
    }
}

void RenderBatch::Capsule(const Vector3 &position, float radius, float height, int segments, bool wire)
{
    SetTexture(0);
    const float cylinderHeight = std::max(0.0f, height - radius * 2.0f);
    Cylinder(position, radius, cylinderHeight, segments, wire);
    Sphere(make_vec3(position.x, position.y + cylinderHeight * 0.5f, position.z), radius, std::max(segments / 2, 4), segments, wire);
    Sphere(make_vec3(position.x, position.y - cylinderHeight * 0.5f, position.z), radius, std::max(segments / 2, 4), segments, wire);
}

void RenderBatch::TriangleLines(const Vector3 &p1, const Vector3 &p2, const Vector3 &p3)
{
    SetTexture(0);
    SetMode(LINES);
    Line3D(p1, p2);
    Line3D(p2, p3);
    Line3D(p3, p1);
}

void RenderBatch::Triangle(const Vector3 &p1, const Vector3 &p2, const Vector3 &p3)
{
    SetTexture(0);
    SetMode(TRIANGLES);
    Vertex3f(p1.x, p1.y, p1.z);
    Vertex3f(p2.x, p2.y, p2.z);
    Vertex3f(p3.x, p3.y, p3.z);
}

void RenderBatch::Grid(int slices, float spacing, bool axes)
{
    SetTexture(0);
    slices = std::max(slices, 1);
    SetMode(LINES);

    const float half = (float)slices * spacing * 0.5f;
    for (int i = 0; i <= slices; ++i)
    {
        const float offset = -half + (float)i * spacing;
        if (axes && fabsf(offset) < 0.0001f)
            SetColor(0.8f, 0.2f, 0.2f);
        else
            SetColor(0.45f, 0.45f, 0.45f);
        Line3D(make_vec3(offset, 0.0f, -half), make_vec3(offset, 0.0f, half));

        if (axes && fabsf(offset) < 0.0001f)
            SetColor(0.2f, 0.8f, 0.2f);
        else
            SetColor(0.45f, 0.45f, 0.45f);
        Line3D(make_vec3(-half, 0.0f, offset), make_vec3(half, 0.0f, offset));
    }

    if (axes)
        SetColor(1.0f, 1.0f, 1.0f);
}

void RenderBatch::Quad(u32 texture, float x, float y, float width, float height)
{
    SetMode(QUAD);
    SetTexture(texture);

    TexCoord2f(0.0f, 0.0f); Vertex2f(x, y);
    TexCoord2f(1.0f, 0.0f); Vertex2f(x + width, y);
    TexCoord2f(1.0f, 1.0f); Vertex2f(x + width, y + height);
    TexCoord2f(0.0f, 1.0f); Vertex2f(x, y + height);
}

void RenderBatch::Quad(const Vector2 *coords, const Vector2 *texcoords)
{
    if (!coords || !texcoords)
        return;

    SetMode(QUAD);

    TexCoord2f(texcoords[0].x, texcoords[0].y); Vertex2f(coords[0].x, coords[0].y);
    TexCoord2f(texcoords[1].x, texcoords[1].y); Vertex2f(coords[1].x, coords[1].y);
    TexCoord2f(texcoords[2].x, texcoords[2].y); Vertex2f(coords[2].x, coords[2].y);
    TexCoord2f(texcoords[3].x, texcoords[3].y); Vertex2f(coords[3].x, coords[3].y);
}

void RenderBatch::Quad(u32 texture, const Vector2 *coords, const Vector2 *texcoords)
{
    SetTexture(texture);
    Quad(coords, texcoords);
}

void RenderBatch::DrawQuad(float x1, float y1, float x2, float y2, float u0, float v0, float u1, float v1)
{
    Vector2 coords[4] = {
        make_vec2(x1, y1),
        make_vec2(x1, y2),
        make_vec2(x2, y2),
        make_vec2(x2, y1)
    };

    Vector2 texcoords[4] = {
        make_vec2(u0, v0),
        make_vec2(u0, v1),
        make_vec2(u1, v1),
        make_vec2(u1, v0)
    };

    Quad(coords, texcoords);
}

void RenderBatch::DrawQuad(u32 texture, float x1, float y1, float x2, float y2, float u0, float v0, float u1, float v1)
{
    SetTexture(texture);
    DrawQuad(x1, y1, x2, y2, u0, v0, u1, v1);
}

void RenderBatch::DrawQuad(float x1, float y1, float x2, float y2, float u0, float v0, float u1, float v1, const Color &color)
{
    SetColor(color);
    DrawQuad(x1, y1, x2, y2, u0, v0, u1, v1);
}

void RenderBatch::Quad(u32 texture, float x1, float y1, float x2, float y2, const FloatRect &src, float textureWidth, float textureHeight)
{
    const float safeWidth = (textureWidth > 0.0f) ? textureWidth : 1.0f;
    const float safeHeight = (textureHeight > 0.0f) ? textureHeight : 1.0f;
    const float left = (2.0f * src.x + 1.0f) / (2.0f * safeWidth);
    const float right = left + (src.width * 2.0f - 2.0f) / (2.0f * safeWidth);
    const float top = (2.0f * src.y + 1.0f) / (2.0f * safeHeight);
    const float bottom = top + (src.height * 2.0f - 2.0f) / (2.0f * safeHeight);

    Vector2 coords[4] = {
        make_vec2(x1, y1),
        make_vec2(x1, y2),
        make_vec2(x2, y2),
        make_vec2(x2, y1)
    };

    Vector2 texcoords[4] = {
        make_vec2(left, top),
        make_vec2(left, bottom),
        make_vec2(right, bottom),
        make_vec2(right, top)
    };

    Quad(texture, coords, texcoords);
}

void RenderBatch::Quad(u32 texture, const FloatRect &src, float x, float y, float width, float height, float textureWidth, float textureHeight)
{
    Quad(texture, x, y, x + width, y + height, src, textureWidth, textureHeight);
}

void RenderBatch::QuadCentered(u32 texture, float textureWidth, float textureHeight, float x, float y, float size, const FloatRect &clip)
{
    const float quadSize = size * 80.0f;
    Quad(texture, x - quadSize, y - quadSize, x + quadSize, y + quadSize, clip, textureWidth, textureHeight);
}

void RenderBatch::Triangle(float x1, float y1, float x2, float y2, float x3, float y3, bool fill)
{
    if (fill)
    {
        SetMode(TRIANGLES);
        Vertex2f(x1, y1);
        Vertex2f(x2, y2);
        Vertex2f(x3, y3);
    }
    else
    {
        SetMode(LINES);
        Vertex2f(x1, y1); Vertex2f(x2, y2);
        Vertex2f(x2, y2); Vertex2f(x3, y3);
        Vertex2f(x3, y3); Vertex2f(x1, y1);
    }
}

void RenderBatch::Ellipse(int centerX, int centerY, float radiusX, float radiusY, bool fill)
{
    const int segments = 64;
    const float angleInc = 2.0f * PI / (float)segments;
    const float cx = (float)centerX;
    const float cy = (float)centerY;

    if (fill)
    {
        SetMode(TRIANGLES);
        float angle = 0.0f;
        for (int i = 0; i < segments; ++i)
        {
            Vertex2f(cx, cy);
            Vertex2f(cx + cosf(angle) * radiusX, cy + sinf(angle) * radiusY);
            angle += angleInc;
            Vertex2f(cx + cosf(angle) * radiusX, cy + sinf(angle) * radiusY);
        }
    }
    else
    {
        SetMode(LINES);
        float angle = 0.0f;
        for (int i = 0; i < segments; ++i)
        {
            Vertex2f(cx + cosf(angle) * radiusX, cy + sinf(angle) * radiusY);
            angle += angleInc;
            Vertex2f(cx + cosf(angle) * radiusX, cy + sinf(angle) * radiusY);
        }
    }
}

void RenderBatch::Polygon(int centerX, int centerY, int sides, float radius, float rotation, bool fill)
{
    if (sides < 3)
        return;

    const float angleInc = 2.0f * PI / (float)sides;
    float angle = rotation * (PI / 180.0f);

    if (fill)
    {
        SetMode(TRIANGLES);
        for (int i = 0; i < sides; ++i)
        {
            Vertex2f((float)centerX, (float)centerY);
            Vertex2f(centerX + cosf(angle) * radius, centerY + sinf(angle) * radius);
            angle += angleInc;
            Vertex2f(centerX + cosf(angle) * radius, centerY + sinf(angle) * radius);
        }
    }
    else
    {
        SetMode(LINES);
        for (int i = 0; i < sides; ++i)
        {
            Vertex2f(centerX + cosf(angle) * radius, centerY + sinf(angle) * radius);
            angle += angleInc;
            Vertex2f(centerX + cosf(angle) * radius, centerY + sinf(angle) * radius);
        }
    }
}

void RenderBatch::Polyline(const Vector2 *points, int pointCount)
{
    if (!points || pointCount < 2)
        return;

    SetMode(LINES);
    for (int i = 0; i < pointCount - 1; ++i)
    {
        Vertex2f(points[i].x, points[i].y);
        Vertex2f(points[i + 1].x, points[i + 1].y);
    }
}

void RenderBatch::RoundedRectangle(int posX, int posY, int width, int height, float roundness, int segments, bool fill)
{
    if (width <= 0 || height <= 0)
        return;

    if (roundness <= 0.0f)
    {
        Rectangle(posX, posY, width, height, fill);
        return;
    }

    float radius = roundness;
    radius = std::min(radius, width * 0.5f);
    radius = std::min(radius, height * 0.5f);
    segments = std::max(2, std::min(segments, 32));

    const float cxTL = posX + radius;
    const float cyTL = posY + radius;
    const float cxTR = posX + width - radius;
    const float cyTR = posY + radius;
    const float cxBR = posX + width - radius;
    const float cyBR = posY + height - radius;
    const float cxBL = posX + radius;
    const float cyBL = posY + height - radius;

    const int totalPoints = segments * 4;
    std::vector<Vector2> pts((size_t)totalPoints);

    auto fillCorner = [&](int baseIndex, float cx, float cy, float startDeg, float endDeg)
    {
        const float startRad = startDeg * (PI / 180.0f);
        const float endRad = endDeg * (PI / 180.0f);
        const float step = (endRad - startRad) / (float)segments;
        for (int i = 0; i < segments; ++i)
        {
            const float a = startRad + step * (float)i;
            pts[(size_t)(baseIndex + i)] = make_vec2(cx + cosf(a) * radius, cy + sinf(a) * radius);
        }
    };

    fillCorner(0 * segments, cxTL, cyTL, 180.0f, 270.0f);
    fillCorner(1 * segments, cxTR, cyTR, 270.0f, 360.0f);
    fillCorner(2 * segments, cxBR, cyBR, 0.0f, 90.0f);
    fillCorner(3 * segments, cxBL, cyBL, 90.0f, 180.0f);

    if (fill)
    {
        SetMode(TRIANGLES);
        const float cx = posX + width * 0.5f;
        const float cy = posY + height * 0.5f;
        for (int i = 0; i < totalPoints; ++i)
        {
            const Vector2 &a = pts[(size_t)i];
            const Vector2 &b = pts[(size_t)((i + 1) % totalPoints)];
            Vertex2f(cx, cy);
            Vertex2f(b.x, b.y);
            Vertex2f(a.x, a.y);
        }
    }
    else
    {
        SetMode(LINES);
        for (int i = 0; i < totalPoints; ++i)
        {
            const Vector2 &a = pts[(size_t)i];
            const Vector2 &b = pts[(size_t)((i + 1) % totalPoints)];
            Vertex2f(a.x, a.y);
            Vertex2f(b.x, b.y);
        }
    }
}

void RenderBatch::CircleSector(int centerX, int centerY, float radius, float startAngle, float endAngle, int segments, bool fill)
{
    segments = std::max(1, segments);
    const float angleStep = (endAngle - startAngle) / (float)segments;
    float angle = startAngle * (PI / 180.0f);

    if (fill)
    {
        SetMode(TRIANGLES);
        for (int i = 0; i < segments; ++i)
        {
            Vertex2f((float)centerX, (float)centerY);
            Vertex2f(centerX + cosf(angle) * radius, centerY + sinf(angle) * radius);
            angle += angleStep * (PI / 180.0f);
            Vertex2f(centerX + cosf(angle) * radius, centerY + sinf(angle) * radius);
        }
    }
    else
    {
        SetMode(LINES);
        for (int i = 0; i < segments; ++i)
        {
            Vertex2f(centerX + cosf(angle) * radius, centerY + sinf(angle) * radius);
            angle += angleStep * (PI / 180.0f);
            Vertex2f(centerX + cosf(angle) * radius, centerY + sinf(angle) * radius);
        }

        const float startRad = startAngle * (PI / 180.0f);
        const float endRad = endAngle * (PI / 180.0f);
        Vertex2f((float)centerX, (float)centerY);
        Vertex2f(centerX + cosf(startRad) * radius, centerY + sinf(startRad) * radius);
        Vertex2f((float)centerX, (float)centerY);
        Vertex2f(centerX + cosf(endRad) * radius, centerY + sinf(endRad) * radius);
    }
}

void RenderBatch::Grid(int posX, int posY, int width, int height, int cellWidth, int cellHeight)
{
    if (cellWidth <= 0 || cellHeight <= 0)
        return;

    SetMode(LINES);
    for (int x = posX; x <= posX + width; x += cellWidth)
    {
        Vertex2f((float)x, (float)posY);
        Vertex2f((float)x, (float)(posY + height));
    }

    for (int y = posY; y <= posY + height; y += cellHeight)
    {
        Vertex2f((float)posX, (float)y);
        Vertex2f((float)(posX + width), (float)y);
    }
}

void RenderBatch::TexturedPolygon(const Vector2 *points, int pointCount, unsigned int textureId)
{
    if (!points || pointCount < 3)
        return;

    float minX = points[0].x;
    float maxX = points[0].x;
    float minY = points[0].y;
    float maxY = points[0].y;
    float centerX = 0.0f;
    float centerY = 0.0f;

    for (int i = 0; i < pointCount; ++i)
    {
        minX = std::min(minX, points[i].x);
        maxX = std::max(maxX, points[i].x);
        minY = std::min(minY, points[i].y);
        maxY = std::max(maxY, points[i].y);
        centerX += points[i].x;
        centerY += points[i].y;
    }

    const float width = std::max(0.0001f, maxX - minX);
    const float height = std::max(0.0001f, maxY - minY);
    centerX /= (float)pointCount;
    centerY /= (float)pointCount;

    SetTexture(textureId);
    SetMode(TRIANGLES);

    const float centerU = (centerX - minX) / width;
    const float centerV = (centerY - minY) / height;
    for (int i = 0; i < pointCount; ++i)
    {
        const int next = (i + 1) % pointCount;
        TexCoord2f(centerU, centerV);
        Vertex2f(centerX, centerY);

        TexCoord2f((points[i].x - minX) / width, (points[i].y - minY) / height);
        Vertex2f(points[i].x, points[i].y);

        TexCoord2f((points[next].x - minX) / width, (points[next].y - minY) / height);
        Vertex2f(points[next].x, points[next].y);
    }
}

void RenderBatch::TexturedPolygonCustomUV(const TexVertex *vertices, int vertexCount, unsigned int textureId)
{
    if (!vertices || vertexCount < 3)
        return;

    float centerX = 0.0f;
    float centerY = 0.0f;
    float centerU = 0.0f;
    float centerV = 0.0f;
    for (int i = 0; i < vertexCount; ++i)
    {
        centerX += vertices[i].position.x;
        centerY += vertices[i].position.y;
        centerU += vertices[i].texCoord.x;
        centerV += vertices[i].texCoord.y;
    }
    centerX /= (float)vertexCount;
    centerY /= (float)vertexCount;
    centerU /= (float)vertexCount;
    centerV /= (float)vertexCount;

    SetTexture(textureId);
    SetMode(TRIANGLES);
    for (int i = 0; i < vertexCount; ++i)
    {
        const int next = (i + 1) % vertexCount;
        TexCoord2f(centerU, centerV);
        Vertex2f(centerX, centerY);

        TexCoord2f(vertices[i].texCoord.x, vertices[i].texCoord.y);
        Vertex2f(vertices[i].position.x, vertices[i].position.y);

        TexCoord2f(vertices[next].texCoord.x, vertices[next].texCoord.y);
        Vertex2f(vertices[next].position.x, vertices[next].position.y);
    }
}

void RenderBatch::TexturedQuad(const Vector2 &p1, const Vector2 &p2, const Vector2 &p3, const Vector2 &p4, unsigned int textureId)
{
    TexturedQuad(p1, p2, p3, p4,
                 make_vec2(0.0f, 0.0f), make_vec2(1.0f, 0.0f),
                 make_vec2(1.0f, 1.0f), make_vec2(0.0f, 1.0f),
                 textureId);
}

void RenderBatch::TexturedQuad(const Vector2 &p1, const Vector2 &p2, const Vector2 &p3, const Vector2 &p4,
                               const Vector2 &uv1, const Vector2 &uv2, const Vector2 &uv3, const Vector2 &uv4,
                               unsigned int textureId)
{
    SetTexture(textureId);
    SetMode(TRIANGLES);

    TexCoord2f(uv1.x, uv1.y); Vertex2f(p1.x, p1.y);
    TexCoord2f(uv2.x, uv2.y); Vertex2f(p2.x, p2.y);
    TexCoord2f(uv3.x, uv3.y); Vertex2f(p3.x, p3.y);

    TexCoord2f(uv1.x, uv1.y); Vertex2f(p1.x, p1.y);
    TexCoord2f(uv3.x, uv3.y); Vertex2f(p3.x, p3.y);
    TexCoord2f(uv4.x, uv4.y); Vertex2f(p4.x, p4.y);
}

void RenderBatch::TexturedTriangle(const Vector2 &p1, const Vector2 &p2, const Vector2 &p3, unsigned int textureId)
{
    TexturedTriangle(p1, p2, p3, make_vec2(0.0f, 0.0f), make_vec2(1.0f, 0.0f), make_vec2(0.5f, 1.0f), textureId);
}

void RenderBatch::TexturedTriangle(const Vector2 &p1, const Vector2 &p2, const Vector2 &p3,
                                   const Vector2 &uv1, const Vector2 &uv2, const Vector2 &uv3,
                                   unsigned int textureId)
{
    SetTexture(textureId);
    SetMode(TRIANGLES);

    TexCoord2f(uv1.x, uv1.y); Vertex2f(p1.x, p1.y);
    TexCoord2f(uv2.x, uv2.y); Vertex2f(p2.x, p2.y);
    TexCoord2f(uv3.x, uv3.y); Vertex2f(p3.x, p3.y);
}

void RenderBatch::BezierQuadratic(const Vector2 &p0, const Vector2 &p1, const Vector2 &p2, int segments)
{
    segments = std::max(1, segments);
    SetMode(LINES);
    for (int i = 0; i < segments; ++i)
    {
        const float t1 = (float)i / (float)segments;
        const float t2 = (float)(i + 1) / (float)segments;
        const float mt1 = 1.0f - t1;
        const float mt2 = 1.0f - t2;
        const float x1 = mt1 * mt1 * p0.x + 2.0f * mt1 * t1 * p1.x + t1 * t1 * p2.x;
        const float y1 = mt1 * mt1 * p0.y + 2.0f * mt1 * t1 * p1.y + t1 * t1 * p2.y;
        const float x2 = mt2 * mt2 * p0.x + 2.0f * mt2 * t2 * p1.x + t2 * t2 * p2.x;
        const float y2 = mt2 * mt2 * p0.y + 2.0f * mt2 * t2 * p1.y + t2 * t2 * p2.y;
        Vertex2f(x1, y1);
        Vertex2f(x2, y2);
    }
}

void RenderBatch::BezierCubic(const Vector2 &p0, const Vector2 &p1, const Vector2 &p2, const Vector2 &p3, int segments)
{
    segments = std::max(1, segments);
    SetMode(LINES);
    for (int i = 0; i < segments; ++i)
    {
        const float t1 = (float)i / (float)segments;
        const float t2 = (float)(i + 1) / (float)segments;
        const float mt1 = 1.0f - t1;
        const float mt2 = 1.0f - t2;

        const float x1 = mt1 * mt1 * mt1 * p0.x + 3.0f * mt1 * mt1 * t1 * p1.x + 3.0f * mt1 * t1 * t1 * p2.x + t1 * t1 * t1 * p3.x;
        const float y1 = mt1 * mt1 * mt1 * p0.y + 3.0f * mt1 * mt1 * t1 * p1.y + 3.0f * mt1 * t1 * t1 * p2.y + t1 * t1 * t1 * p3.y;
        const float x2 = mt2 * mt2 * mt2 * p0.x + 3.0f * mt2 * mt2 * t2 * p1.x + 3.0f * mt2 * t2 * t2 * p2.x + t2 * t2 * t2 * p3.x;
        const float y2 = mt2 * mt2 * mt2 * p0.y + 3.0f * mt2 * mt2 * t2 * p1.y + 3.0f * mt2 * t2 * t2 * p2.y + t2 * t2 * t2 * p3.y;
        Vertex2f(x1, y1);
        Vertex2f(x2, y2);
    }
}

void RenderBatch::CatmullRomSpline(const Vector2 *points, int pointCount, int segments)
{
    if (!points || pointCount < 4)
        return;

    segments = std::max(1, segments);
    SetMode(LINES);
    for (int i = 0; i < pointCount - 3; ++i)
    {
        const Vector2 &p0 = points[i];
        const Vector2 &p1 = points[i + 1];
        const Vector2 &p2 = points[i + 2];
        const Vector2 &p3 = points[i + 3];
        for (int s = 0; s < segments; ++s)
        {
            const float t1 = (float)s / (float)segments;
            const float t2 = (float)(s + 1) / (float)segments;
            const float t1_2 = t1 * t1;
            const float t1_3 = t1_2 * t1;
            const float t2_2 = t2 * t2;
            const float t2_3 = t2_2 * t2;

            const float x1 = 0.5f * ((2.0f * p1.x) + (-p0.x + p2.x) * t1 + (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t1_2 + (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t1_3);
            const float y1 = 0.5f * ((2.0f * p1.y) + (-p0.y + p2.y) * t1 + (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t1_2 + (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t1_3);
            const float x2 = 0.5f * ((2.0f * p1.x) + (-p0.x + p2.x) * t2 + (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2_2 + (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t2_3);
            const float y2 = 0.5f * ((2.0f * p1.y) + (-p0.y + p2.y) * t2 + (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2_2 + (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t2_3);
            Vertex2f(x1, y1);
            Vertex2f(x2, y2);
        }
    }
}

void RenderBatch::BSpline(const Vector2 *points, int pointCount, int segments, int degree)
{
    (void)degree;
    if (!points || pointCount < 4)
        return;

    segments = std::max(1, segments);
    SetMode(LINES);
    for (int i = 0; i < pointCount - 3; ++i)
    {
        const Vector2 &p0 = points[i];
        const Vector2 &p1 = points[i + 1];
        const Vector2 &p2 = points[i + 2];
        const Vector2 &p3 = points[i + 3];
        for (int s = 0; s < segments; ++s)
        {
            const float t1 = (float)s / (float)segments;
            const float t2 = (float)(s + 1) / (float)segments;
            const float t1_2 = t1 * t1;
            const float t1_3 = t1_2 * t1;
            const float t2_2 = t2 * t2;
            const float t2_3 = t2_2 * t2;

            const float b0_1 = (1.0f - 3.0f * t1 + 3.0f * t1_2 - t1_3) / 6.0f;
            const float b1_1 = (4.0f - 6.0f * t1_2 + 3.0f * t1_3) / 6.0f;
            const float b2_1 = (1.0f + 3.0f * t1 + 3.0f * t1_2 - 3.0f * t1_3) / 6.0f;
            const float b3_1 = t1_3 / 6.0f;
            const float b0_2 = (1.0f - 3.0f * t2 + 3.0f * t2_2 - t2_3) / 6.0f;
            const float b1_2 = (4.0f - 6.0f * t2_2 + 3.0f * t2_3) / 6.0f;
            const float b2_2 = (1.0f + 3.0f * t2 + 3.0f * t2_2 - 3.0f * t2_3) / 6.0f;
            const float b3_2 = t2_3 / 6.0f;

            const float x1 = b0_1 * p0.x + b1_1 * p1.x + b2_1 * p2.x + b3_1 * p3.x;
            const float y1 = b0_1 * p0.y + b1_1 * p1.y + b2_1 * p2.y + b3_1 * p3.y;
            const float x2 = b0_2 * p0.x + b1_2 * p1.x + b2_2 * p2.x + b3_2 * p3.x;
            const float y2 = b0_2 * p0.y + b1_2 * p1.y + b2_2 * p2.y + b3_2 * p3.y;
            Vertex2f(x1, y1);
            Vertex2f(x2, y2);
        }
    }
}

void RenderBatch::HermiteSpline(const HermitePoint *points, int pointCount, int segments)
{
    if (!points || pointCount < 2)
        return;

    segments = std::max(1, segments);
    SetMode(LINES);
    for (int i = 0; i < pointCount - 1; ++i)
    {
        const Vector2 &p0 = points[i].position;
        const Vector2 &m0 = points[i].tangent;
        const Vector2 &p1 = points[i + 1].position;
        const Vector2 &m1 = points[i + 1].tangent;
        for (int s = 0; s < segments; ++s)
        {
            const float t1 = (float)s / (float)segments;
            const float t2 = (float)(s + 1) / (float)segments;
            const float t1_2 = t1 * t1;
            const float t1_3 = t1_2 * t1;
            const float t2_2 = t2 * t2;
            const float t2_3 = t2_2 * t2;

            const float h00_1 = 2.0f * t1_3 - 3.0f * t1_2 + 1.0f;
            const float h10_1 = t1_3 - 2.0f * t1_2 + t1;
            const float h01_1 = -2.0f * t1_3 + 3.0f * t1_2;
            const float h11_1 = t1_3 - t1_2;
            const float h00_2 = 2.0f * t2_3 - 3.0f * t2_2 + 1.0f;
            const float h10_2 = t2_3 - 2.0f * t2_2 + t2;
            const float h01_2 = -2.0f * t2_3 + 3.0f * t2_2;
            const float h11_2 = t2_3 - t2_2;

            const float x1 = h00_1 * p0.x + h10_1 * m0.x + h01_1 * p1.x + h11_1 * m1.x;
            const float y1 = h00_1 * p0.y + h10_1 * m0.y + h01_1 * p1.y + h11_1 * m1.y;
            const float x2 = h00_2 * p0.x + h10_2 * m0.x + h01_2 * p1.x + h11_2 * m1.x;
            const float y2 = h00_2 * p0.y + h10_2 * m0.y + h01_2 * p1.y + h11_2 * m1.y;
            Vertex2f(x1, y1);
            Vertex2f(x2, y2);
        }
    }
}

void RenderBatch::ThickSpline(const Vector2 *points, int pointCount, float thickness, int segments)
{
    (void)segments;
    if (!points || pointCount < 2)
        return;

    SetMode(TRIANGLES);
    const float halfThickness = thickness * 0.5f;
    for (int i = 0; i < pointCount - 1; ++i)
    {
        const Vector2 &p0 = points[i];
        const Vector2 &p1 = points[i + 1];
        float dx = p1.x - p0.x;
        float dy = p1.y - p0.y;
        const float len = sqrtf(dx * dx + dy * dy);
        if (len <= 0.0001f)
            continue;

        dx /= len;
        dy /= len;
        const float px = -dy * halfThickness;
        const float py = dx * halfThickness;

        const Vector2 v0 = make_vec2(p0.x + px, p0.y + py);
        const Vector2 v1 = make_vec2(p0.x - px, p0.y - py);
        const Vector2 v2 = make_vec2(p1.x + px, p1.y + py);
        const Vector2 v3 = make_vec2(p1.x - px, p1.y - py);

        Vertex2f(v0.x, v0.y);
        Vertex2f(v1.x, v1.y);
        Vertex2f(v2.x, v2.y);

        Vertex2f(v1.x, v1.y);
        Vertex2f(v3.x, v3.y);
        Vertex2f(v2.x, v2.y);
    }
}
