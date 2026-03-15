#pragma once

#include "batch.hpp"
#include <string>
#include <unordered_map>
#include <vector>

// Windows GDI headers may define GetCharWidth as a macro alias.
// Keep SpriteFont::GetCharWidth as a real method name.
#ifdef GetCharWidth
#undef GetCharWidth
#endif

struct Texture
{
    unsigned int id;
    int width;
    int height;

    Texture() : id(0), width(0), height(0) {}
};
class Pixmap;

struct IntRect
{
    int x;
    int y;
    int width;
    int height;

    IntRect() : x(0), y(0), width(0), height(0) {}
    IntRect(int px, int py, int pw, int ph) : x(px), y(py), width(pw), height(ph) {}
};

enum class TextAlign
{
    LEFT,
    CENTER,
    RIGHT,
    JUSTIFY
};

enum class TextVAlign
{
    TOP,
    MIDDLE,
    BOTTOM
};

struct GlyphInfo
{
    int value;
    int offsetX;
    int offsetY;
    int advanceX;
};

struct TextMetrics
{
    Vector2 size;
    float lineHeight;
    int lineCount;
    float maxLineWidth;
    std::vector<float> lineWidths;

    TextMetrics()
        : size{0.0f, 0.0f}, lineHeight(0.0f), lineCount(0), maxLineWidth(0.0f)
    {
    }
};

struct TextStyle
{
    Color color = Color((u8)255, (u8)255, (u8)255, (u8)255);
    float fontSize = 16.0f;
    float spacing = 1.0f;
    float lineSpacing = 0.0f;
    bool enableShadow = false;
    Color shadowColor = Color((u8)0, (u8)0, (u8)0, (u8)255);
    Vector2 shadowOffset = {1.0f, 1.0f};
    bool enableOutline = false;
    Color outlineColor = Color((u8)0, (u8)0, (u8)0, (u8)255);
    float outlineThickness = 1.0f;
    TextAlign align = TextAlign::LEFT;
    TextVAlign valign = TextVAlign::TOP;
};

class SpriteFont
{
public:
    SpriteFont();
    ~SpriteFont();

    bool LoadDefaultFont();
    bool LoadBMFont(const char *fntPath, const char *texturePath = nullptr);
    bool LoadFromPixmap(const Pixmap &pixmap, int charWidth, int charHeight, const char *charset = nullptr);
    bool LoadTTF(const char *ttfPath, float fontSize, int atlasSize = 512);
    bool LoadTTFFromMemory(const unsigned char *data, int dataSize, float fontSize, int atlasSize = 512);

    void Release();

    void Print(const char *text, float x, float y);
    void Print(float x, float y, const char *fmt, ...);
    void Print(const char *text, float x, float y, const TextStyle &style);
    void PrintAligned(const char *text, const FloatRect &bounds,
                      TextAlign align = TextAlign::LEFT,
                      TextVAlign valign = TextVAlign::TOP);
    void PrintWrapped(const char *text, float x, float y, float maxWidth);
    void PrintWrapped(const char *text, const FloatRect &bounds, const TextStyle &style);
    void PrintWithShadow(const char *text, float x, float y,
                         const Color &textColor, const Color &shadowColor,
                         const Vector2 &shadowOffset = {1.0f, 1.0f});
    void PrintWithOutline(const char *text, float x, float y,
                          const Color &textColor, const Color &outlineColor,
                          float thickness = 1.0f);

    Vector2 GetTextSize(const char *text);
    float GetTextWidth(const char *text);
    float GetTextHeight(const char *text);
    TextMetrics MeasureText(const char *text, float maxWidth = 0.0f);
    float GetCharWidth(int codepoint);
    std::vector<std::string> WrapText(const char *text, float maxWidth);

    void SetBatch(RenderBatch *renderBatch) { batch = renderBatch; }
    void SetColor(const Color &newColor) { color = newColor; }
    void SetColor(u8 r, u8 g, u8 b, u8 a = 255);
    void SetFontSize(float size) { fontSize = size; }
    void SetSpacing(float value) { spacing = value; }
    void SetLineSpacing(float value) { textLineSpacing = value; }
    void SetClip(int x, int y, int w, int h);
    void EnableClip(bool enable);

    float GetFontSize() const { return fontSize; }
    float GetSpacing() const { return spacing; }
    float GetLineSpacing() const { return textLineSpacing; }
    int GetBaseSize() const { return m_baseSize; }
    int GetGlyphCount() const { return m_glyphCount; }
    Texture *GetTexture() const { return texture; }
    bool IsValid() const { return m_loaded && m_glyphCount > 0; }

private:
    int getGlyphIndex(int codepoint);
    std::vector<std::string> SplitIntoLines(const char *text);
    float GetLineWidth(const char *line);
    void drawTextCodepoint(int codepoint, float x, float y);
    void warnRenderPath() const;
    void warnUnsupportedLoad(const char *api) const;

    Texture *texture;
    RenderBatch *batch;
    int m_baseSize;
    int m_glyphCount;
    int m_glyphPadding;
    std::vector<FloatRect> m_recs;
    std::vector<GlyphInfo> m_glyphs;
    std::unordered_map<int, int> m_glyphCache;
    Color color;
    float fontSize;
    float spacing;
    float textLineSpacing;
    bool enableClip;
    IntRect clip;
    bool m_loaded;
    mutable bool m_renderWarningEmitted;
    mutable bool m_loadWarningEmitted;
};
