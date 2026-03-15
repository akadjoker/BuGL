#pragma once

#include <cstdint>
#include <vector>
#include "raymath.h"

using u8 = std::uint8_t;
using u32 = std::uint32_t;

struct Color
{
    u8 r;
    u8 g;
    u8 b;
    u8 a;

    Color();
    Color(u8 pr, u8 pg, u8 pb, u8 pa = 255);
    Color(float pr, float pg, float pb, float pa = 1.0f);
};

struct BoundingBox
{
    Vector3 min;
    Vector3 max;

    BoundingBox() = default;
    BoundingBox(const Vector3 &pmin, const Vector3 &pmax) : min(pmin), max(pmax) {}
};

struct FloatRect
{
    float x;
    float y;
    float width;
    float height;

    FloatRect() : x(0.0f), y(0.0f), width(0.0f), height(0.0f) {}
    FloatRect(float px, float py, float pw, float ph) : x(px), y(py), width(pw), height(ph) {}
};

struct TexVertex
{
    Vector2 position;
    Vector2 texCoord;
};

struct HermitePoint
{
    Vector2 position;
    Vector2 tangent;
};

class RenderBatch
{
public:
    RenderBatch();
    virtual ~RenderBatch();

    void Release();
    void Init(int numBuffers = 1, int bufferElements = 10000);

    void SetColor(const Color &color);
    void SetColor(float r, float g, float b);
    void SetColor(u8 r, u8 g, u8 b, u8 a);
    void SetAlpha(float a);

    void Line2D(int startPosX, int startPosY, int endPosX, int endPosY);
    void Line2D(const Vector2 &start, const Vector2 &end);

    void Line3D(const Vector3 &start, const Vector3 &end);
    void Line3D(float startX, float startY, float startZ, float endX, float endY, float endZ);

    void Circle(int centerX, int centerY, float radius, bool fill = false);
    void Rectangle(int posX, int posY, int width, int height, bool fill = false);
    void Triangle(float x1, float y1, float x2, float y2, float x3, float y3, bool fill = true);
    void Ellipse(int centerX, int centerY, float radiusX, float radiusY, bool fill = true);
    void Polygon(int centerX, int centerY, int sides, float radius, float rotation = 0.0f, bool fill = true);
    void Polyline(const Vector2 *points, int pointCount);
    void RoundedRectangle(int posX, int posY, int width, int height, float roundness, int segments = 8, bool fill = true);
    void CircleSector(int centerX, int centerY, float radius, float startAngle, float endAngle, int segments = 16, bool fill = true);
    void Grid(int posX, int posY, int width, int height, int cellWidth, int cellHeight);

    void Box(const BoundingBox &box);
    void Box(const BoundingBox &box, const Matrix &transform);

    void Cube(const Vector3 &position, float width, float height, float depth, bool wire = true);
    void Sphere(const Vector3 &position, float radius, int rings, int slices, bool wire = true);
    void Cone(const Vector3 &position, float radius, float height, int segments, bool wire = true);
    void Cylinder(const Vector3 &position, float radius, float height, int segments, bool wire = true);
    void Capsule(const Vector3 &position, float radius, float height, int segments, bool wire = true);

    void TriangleLines(const Vector3 &p1, const Vector3 &p2, const Vector3 &p3);
    void Triangle(const Vector3 &p1, const Vector3 &p2, const Vector3 &p3);

    void Grid(int slices, float spacing, bool axes = true);

    void TexturedPolygon(const Vector2 *points, int pointCount, unsigned int textureId);
    void TexturedPolygonCustomUV(const TexVertex *vertices, int vertexCount, unsigned int textureId);
    void TexturedQuad(const Vector2 &p1, const Vector2 &p2, const Vector2 &p3, const Vector2 &p4, unsigned int textureId);
    void TexturedQuad(const Vector2 &p1, const Vector2 &p2, const Vector2 &p3, const Vector2 &p4,
                      const Vector2 &uv1, const Vector2 &uv2, const Vector2 &uv3, const Vector2 &uv4,
                      unsigned int textureId);
    void TexturedTriangle(const Vector2 &p1, const Vector2 &p2, const Vector2 &p3, unsigned int textureId);
    void TexturedTriangle(const Vector2 &p1, const Vector2 &p2, const Vector2 &p3,
                          const Vector2 &uv1, const Vector2 &uv2, const Vector2 &uv3,
                          unsigned int textureId);
    void BezierQuadratic(const Vector2 &p0, const Vector2 &p1, const Vector2 &p2, int segments = 20);
    void BezierCubic(const Vector2 &p0, const Vector2 &p1, const Vector2 &p2, const Vector2 &p3, int segments = 30);
    void CatmullRomSpline(const Vector2 *points, int pointCount, int segments = 20);
    void BSpline(const Vector2 *points, int pointCount, int segments = 20, int degree = 3);
    void HermiteSpline(const HermitePoint *points, int pointCount, int segments = 20);
    void ThickSpline(const Vector2 *points, int pointCount, float thickness, int segments = 20);

    void Quad(const Vector2 *coords, const Vector2 *texcoords);
    void Quad(u32 texture, const Vector2 *coords, const Vector2 *texcoords);
    void Quad(u32 texture, const FloatRect &src, float x, float y, float width, float height, float textureWidth, float textureHeight);
    void Quad(u32 texture, float x1, float y1, float x2, float y2, const FloatRect &src, float textureWidth, float textureHeight);
    void Quad(u32 texture, float x, float y, float width, float height);
    void QuadCentered(u32 texture, float textureWidth, float textureHeight, float x, float y, float size, const FloatRect &clip);
    void DrawQuad(float x1, float y1, float x2, float y2, float u0, float v0, float u1, float v1);
    void DrawQuad(u32 texture, float x1, float y1, float x2, float y2, float u0, float v0, float u1, float v1);
    void DrawQuad(float x1, float y1, float x2, float y2, float u0, float v0, float u1, float v1, const Color &color);

    void BeginTransform(const Matrix &transform);
    void EndTransform();

    void Render();
    void SetMode(int mode);

    void Vertex2f(float x, float y);
    void Vertex3f(float x, float y, float z);
    void TexCoord2f(float x, float y);

    void SetTexture(unsigned int id);
    void SetMatrix(const Matrix &matrix);

private:
    struct Vertex
    {
        float x;
        float y;
        float z;
        float u;
        float v;
        u8 r;
        u8 g;
        u8 b;
        u8 a;
    };

    struct DrawCall
    {
        int mode;
        unsigned textureId;
        std::size_t first;
        std::size_t count;
    };

    bool CheckRenderBatchLimit(int vCount);
    void ensureDrawCall();
    Vector3 transformPoint(const Vector3 &point) const;
    bool createDeviceObjects();
    void destroyDeviceObjects();
    bool ensureIndexCapacity(std::size_t vertexCount);
    bool compileShaderProgram();

    int maxVertices;
    int currentMode;
    unsigned int currentTexture;
    float currentDepth;
    float texcoordx;
    float texcoordy;
    u8 colorr;
    u8 colorg;
    u8 colorb;
    u8 colora;
    bool use_matrix;
    Matrix modelMatrix;
    Matrix viewMatrix;
    
    std::vector<Vertex> vertices;
    std::vector<DrawCall> draws;
    std::vector<unsigned int> quadIndices;
    unsigned int programId;
    unsigned int vboId;
    unsigned int eboId;
    unsigned int whiteTextureId;
    int uMvpLocation;
    int uTextureLocation;
    int aPosLocation;
    int aUvLocation;
    int aColorLocation;
    bool gpuReady;
};
