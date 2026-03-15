#include "pch.h"
#include "spriteFont.hpp"
#include "platform.hpp"
#include "utils.hpp"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <fstream>
#include <sstream>
#include <sstream>
#include <SDL2/SDL_opengl.h>

#include "data.cc"
#include "stb_image.h"
#include "stb_truetype.h"

namespace
{
    static int get_codepoint_next(const char *text, int *codepointSize)
    {
        const unsigned char *ptr = (const unsigned char *)text;
        int codepoint = 0x3f;
        *codepointSize = 1;

        if ((ptr[0] & 0x80u) == 0)
        {
            codepoint = ptr[0];
            return codepoint;
        }

        if ((ptr[0] & 0xe0u) == 0xc0u && (ptr[1] & 0xc0u) == 0x80u)
        {
            *codepointSize = 2;
            return ((ptr[0] & 0x1fu) << 6) | (ptr[1] & 0x3fu);
        }

        if ((ptr[0] & 0xf0u) == 0xe0u && (ptr[1] & 0xc0u) == 0x80u && (ptr[2] & 0xc0u) == 0x80u)
        {
            *codepointSize = 3;
            return ((ptr[0] & 0x0fu) << 12) | ((ptr[1] & 0x3fu) << 6) | (ptr[2] & 0x3fu);
        }

        if ((ptr[0] & 0xf8u) == 0xf0u && (ptr[1] & 0xc0u) == 0x80u && (ptr[2] & 0xc0u) == 0x80u && (ptr[3] & 0xc0u) == 0x80u)
        {
            *codepointSize = 4;
            return ((ptr[0] & 0x07u) << 18) | ((ptr[1] & 0x3fu) << 12) | ((ptr[2] & 0x3fu) << 6) | (ptr[3] & 0x3fu);
        }

        return codepoint;
    }

    static bool read_file_bytes(const char *path, std::vector<unsigned char> *outBytes)
    {
        if (!path || !outBytes)
            return false;

        const int size = OsFileSize(path);
        if (size <= 0)
            return false;

        outBytes->resize((size_t)size);
        const int read = OsFileRead(path, outBytes->data(), (size_t)size);
        if (read != size)
        {
            outBytes->clear();
            return false;
        }
        return true;
    }

    static Texture *upload_font_texture_rgba(const unsigned char *alphaPixels, int width, int height)
    {
        if (!alphaPixels || width <= 0 || height <= 0)
            return nullptr;

        std::vector<unsigned char> rgba((size_t)width * (size_t)height * 4u, 255);
        for (int i = 0; i < width * height; ++i)
            rgba[(size_t)i * 4u + 3u] = alphaPixels[i];

        Texture *texture = new Texture();
        texture->width = width;
        texture->height = height;
        glGenTextures(1, &texture->id);
        if (texture->id == 0)
        {
            delete texture;
            return nullptr;
        }

        glBindTexture(GL_TEXTURE_2D, texture->id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
        glBindTexture(GL_TEXTURE_2D, 0);
        return texture;
    }

    static Texture *upload_texture_rgba(const unsigned char *pixels, int width, int height)
    {
        if (!pixels || width <= 0 || height <= 0)
            return nullptr;

        Texture *texture = new Texture();
        texture->width = width;
        texture->height = height;
        glGenTextures(1, &texture->id);
        if (texture->id == 0)
        {
            delete texture;
            return nullptr;
        }

        glBindTexture(GL_TEXTURE_2D, texture->id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glBindTexture(GL_TEXTURE_2D, 0);
        return texture;
    }

    static void join_relative_path(const char *baseFile, const char *relativePath, char *outPath, size_t outSize)
    {
        if (!outPath || outSize == 0)
            return;

        outPath[0] = '\0';
        if (!relativePath || !relativePath[0])
            return;

        if (!baseFile || !baseFile[0] ||
            relativePath[0] == '/' || relativePath[0] == '\\' ||
            (std::strlen(relativePath) > 1 && relativePath[1] == ':'))
        {
            std::snprintf(outPath, outSize, "%s", relativePath);
            return;
        }

        char baseCopy[512];
        std::snprintf(baseCopy, sizeof(baseCopy), "%s", baseFile);
        char *slash = std::strrchr(baseCopy, '/');
#ifdef _WIN32
        char *backslash = std::strrchr(baseCopy, '\\');
        if (!slash || (backslash && backslash > slash))
            slash = backslash;
#endif
        if (!slash)
        {
            std::snprintf(outPath, outSize, "%s", relativePath);
            return;
        }

        *slash = '\0';
        std::snprintf(outPath, outSize, "%s/%s", baseCopy, relativePath);
    }

    static bool parse_bmfont_value(const std::string &token, const char *key, int *out)
    {
        if (!out)
            return false;
        const std::string prefix = std::string(key) + "=";
        if (token.rfind(prefix, 0) != 0)
            return false;
        *out = std::atoi(token.c_str() + (int)prefix.size());
        return true;
    }

    static bool parse_bmfont_quoted_value(const std::string &line, const char *key, std::string *out)
    {
        if (!out)
            return false;

        const std::string needle = std::string(key) + "=\"";
        const size_t start = line.find(needle);
        if (start == std::string::npos)
            return false;

        const size_t valueStart = start + needle.size();
        const size_t valueEnd = line.find('"', valueStart);
        if (valueEnd == std::string::npos)
            return false;

        *out = line.substr(valueStart, valueEnd - valueStart);
        return true;
    }
}

SpriteFont::SpriteFont()
    : texture(nullptr),
      batch(nullptr),
      m_baseSize(16),
      m_glyphCount(0),
      m_glyphPadding(0),
      color((u8)255, (u8)255, (u8)255, (u8)255),
      fontSize(16.0f),
      spacing(1.0f),
      textLineSpacing(0.0f),
      enableClip(false),
      clip(),
      m_loaded(false),
      m_renderWarningEmitted(false),
      m_loadWarningEmitted(false)
{
}

SpriteFont::~SpriteFont()
{
    Release();
}

void SpriteFont::warnRenderPath() const
{
    if (!m_renderWarningEmitted)
    {
        LogWarning("[SpriteFont] render path is not wired to the current renderer yet");
        m_renderWarningEmitted = true;
    }
}

void SpriteFont::warnUnsupportedLoad(const char *api) const
{
    if (!m_loadWarningEmitted)
    {
        LogWarning("[SpriteFont] %s is not wired in this refactor yet", api ? api : "load");
        m_loadWarningEmitted = true;
    }
}

void SpriteFont::Release()
{
    if (texture)
    {
        if (texture->id != 0)
            glDeleteTextures(1, &texture->id);
        delete texture;
        texture = nullptr;
    }
    m_glyphCount = 0;
    m_baseSize = 16;
    m_glyphPadding = 0;
    m_recs.clear();
    m_glyphs.clear();
    m_glyphCache.clear();
    m_loaded = false;
}

bool SpriteFont::LoadDefaultFont()
{
    Release();

    static const int charsHeight = 10;
    static const int charsDivisor = 1;
    static const int atlasWidth = 128;
    static const int atlasHeight = 128;
    static const int glyphCount = 224;

    unsigned char pixels[atlasWidth * atlasHeight * 4];
    std::memset(pixels, 0, sizeof(pixels));

    for (int i = 0, counter = 0; i < atlasWidth * atlasHeight; i += 32)
    {
        for (int j = 31; j >= 0; --j)
        {
            const bool bitSet = (defaultFontData[counter] & (1u << j)) != 0;
            const int pixelIndex = (i + j) * 4;
            pixels[pixelIndex + 0] = 255;
            pixels[pixelIndex + 1] = 255;
            pixels[pixelIndex + 2] = 255;
            pixels[pixelIndex + 3] = bitSet ? 255 : 0;
        }
        counter++;
    }

    texture = new Texture();
    texture->width = atlasWidth;
    texture->height = atlasHeight;
    glGenTextures(1, &texture->id);
    if (texture->id == 0)
    {
        delete texture;
        texture = nullptr;
        LogError("[SpriteFont] failed to create default font texture");
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, texture->id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, atlasWidth, atlasHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_glyphCount = glyphCount;
    m_glyphPadding = 0;
    m_baseSize = charsHeight;
    m_glyphs.resize((size_t)m_glyphCount);
    m_recs.resize((size_t)m_glyphCount);

    int currentLine = 0;
    int currentPosX = charsDivisor;
    int testPosX = charsDivisor;
    for (int i = 0; i < m_glyphCount; ++i)
    {
        m_glyphs[(size_t)i].value = 32 + i;
        m_recs[(size_t)i].x = (float)currentPosX;
        m_recs[(size_t)i].y = (float)(charsDivisor + currentLine * (charsHeight + charsDivisor));
        m_recs[(size_t)i].width = (float)charsWidth[i];
        m_recs[(size_t)i].height = (float)charsHeight;

        testPosX += charsWidth[i] + charsDivisor;
        if (testPosX >= atlasWidth)
        {
            currentLine++;
            currentPosX = 2 * charsDivisor + charsWidth[i];
            testPosX = currentPosX;
            m_recs[(size_t)i].x = (float)charsDivisor;
            m_recs[(size_t)i].y = (float)(charsDivisor + currentLine * (charsHeight + charsDivisor));
        }
        else
        {
            currentPosX = testPosX;
        }

        m_glyphs[(size_t)i].offsetX = 0;
        m_glyphs[(size_t)i].offsetY = 0;
        m_glyphs[(size_t)i].advanceX = 0;
        m_glyphCache[m_glyphs[(size_t)i].value] = i;
    }

    m_loaded = true;
    return true;
}

bool SpriteFont::LoadBMFont(const char *fntPath, const char *texturePath)
{
    Release();

    if (!fntPath || !fntPath[0])
    {
        LogError("[SpriteFont] invalid BMFont path");
        return false;
    }

    std::ifstream file(fntPath);
    if (!file.is_open())
    {
        LogError("[SpriteFont] failed to open BMFont file: %s", fntPath);
        return false;
    }

    std::vector<GlyphInfo> glyphs;
    std::vector<FloatRect> recs;
    std::string textureFile;
    std::string line;

    while (std::getline(file, line))
    {
        if (line.rfind("common ", 0) == 0)
        {
            std::istringstream iss(line);
            std::string token;
            iss >> token;
            while (iss >> token)
            {
                int value = 0;
                if (parse_bmfont_value(token, "lineHeight", &value))
                    m_baseSize = value;
            }
        }
        else if (line.rfind("page ", 0) == 0)
        {
            parse_bmfont_quoted_value(line, "file", &textureFile);
        }
        else if (line.rfind("char ", 0) == 0)
        {
            GlyphInfo glyph{};
            FloatRect rec;

            std::istringstream iss(line);
            std::string token;
            iss >> token;
            while (iss >> token)
            {
                int value = 0;
                if (parse_bmfont_value(token, "id", &value)) glyph.value = value;
                else if (parse_bmfont_value(token, "x", &value)) rec.x = (float)value;
                else if (parse_bmfont_value(token, "y", &value)) rec.y = (float)value;
                else if (parse_bmfont_value(token, "width", &value)) rec.width = (float)value;
                else if (parse_bmfont_value(token, "height", &value)) rec.height = (float)value;
                else if (parse_bmfont_value(token, "xoffset", &value)) glyph.offsetX = value;
                else if (parse_bmfont_value(token, "yoffset", &value)) glyph.offsetY = value;
                else if (parse_bmfont_value(token, "xadvance", &value)) glyph.advanceX = value;
            }

            glyphs.push_back(glyph);
            recs.push_back(rec);
        }
    }

    file.close();

    if (glyphs.empty())
    {
        LogError("[SpriteFont] no glyphs found in BMFont: %s", fntPath);
        return false;
    }

    char resolvedTexturePath[512];
    if (texturePath && texturePath[0])
        std::snprintf(resolvedTexturePath, sizeof(resolvedTexturePath), "%s", texturePath);
    else
        join_relative_path(fntPath, textureFile.c_str(), resolvedTexturePath, sizeof(resolvedTexturePath));

    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char *pixels = stbi_load(resolvedTexturePath, &width, &height, &channels, 4);
    if (!pixels)
    {
        LogError("[SpriteFont] failed to load BMFont texture: %s", resolvedTexturePath);
        return false;
    }

    texture = upload_texture_rgba(pixels, width, height);
    stbi_image_free(pixels);
    if (!texture)
    {
        LogError("[SpriteFont] failed to upload BMFont texture: %s", resolvedTexturePath);
        return false;
    }

    m_glyphCount = (int)glyphs.size();
    m_glyphs = glyphs;
    m_recs = recs;
    m_glyphPadding = 0;
    m_glyphCache.clear();
    for (int i = 0; i < m_glyphCount; ++i)
        m_glyphCache[m_glyphs[(size_t)i].value] = i;

    if (m_baseSize <= 0)
        m_baseSize = 16;
    m_loaded = true;
    return true;
}

bool SpriteFont::LoadFromPixmap(const Pixmap &pixmap, int charWidth, int charHeight, const char *charset)
{
    (void)pixmap;
    (void)charWidth;
    (void)charHeight;
    (void)charset;
    warnUnsupportedLoad("LoadFromPixmap");
    return false;
}

bool SpriteFont::LoadTTF(const char *ttfPath, float pixelSize, int atlasSize)
{
    std::vector<unsigned char> bytes;
    if (!read_file_bytes(ttfPath, &bytes))
    {
        LogError("[SpriteFont] failed to read TTF: %s", ttfPath ? ttfPath : "(null)");
        return false;
    }

    return LoadTTFFromMemory(bytes.data(), (int)bytes.size(), pixelSize, atlasSize);
}

bool SpriteFont::LoadTTFFromMemory(const unsigned char *data, int dataSize, float pixelSize, int atlasSize)
{
    if (!data || dataSize <= 0 || pixelSize <= 0.0f || atlasSize <= 0)
    {
        LogError("[SpriteFont] invalid TTF memory input");
        return false;
    }

    Release();

    const int firstChar = 32;
    const int numChars = 224;
    const int fontOffset = stbtt_GetFontOffsetForIndex(data, 0);
    if (fontOffset < 0)
    {
        LogError("[SpriteFont] invalid TTF font offset");
        return false;
    }

    std::vector<unsigned char> bitmap((size_t)atlasSize * (size_t)atlasSize, 0);
    std::vector<stbtt_bakedchar> baked((size_t)numChars);
    const int row = stbtt_BakeFontBitmap(data,
                                         fontOffset,
                                         pixelSize,
                                         bitmap.data(),
                                         atlasSize,
                                         atlasSize,
                                         firstChar,
                                         numChars,
                                         baked.data());
    if (row <= 0)
    {
        LogError("[SpriteFont] stbtt_BakeFontBitmap failed for atlas %dx%d", atlasSize, atlasSize);
        return false;
    }

    texture = upload_font_texture_rgba(bitmap.data(), atlasSize, atlasSize);
    if (!texture)
    {
        LogError("[SpriteFont] failed to upload TTF atlas texture");
        return false;
    }

    stbtt_fontinfo fontInfo;
    if (!stbtt_InitFont(&fontInfo, data, fontOffset))
    {
        Release();
        LogError("[SpriteFont] stbtt_InitFont failed");
        return false;
    }

    m_baseSize = (int)pixelSize;
    if (m_baseSize <= 0)
        m_baseSize = 16;
    m_glyphCount = numChars;
    m_glyphPadding = 0;
    m_glyphs.resize((size_t)m_glyphCount);
    m_recs.resize((size_t)m_glyphCount);
    m_glyphCache.clear();

    for (int i = 0; i < numChars; ++i)
    {
        const stbtt_bakedchar &g = baked[(size_t)i];
        GlyphInfo &glyph = m_glyphs[(size_t)i];
        FloatRect &rec = m_recs[(size_t)i];
        glyph.value = firstChar + i;
        glyph.offsetX = (int)std::floor(g.xoff);
        glyph.offsetY = (int)std::floor(g.yoff);
        glyph.advanceX = (int)std::round(g.xadvance);
        rec.x = (float)g.x0;
        rec.y = (float)g.y0;
        rec.width = (float)(g.x1 - g.x0);
        rec.height = (float)(g.y1 - g.y0);
        m_glyphCache[glyph.value] = i;
    }

    m_loaded = true;
    return true;
}

int SpriteFont::getGlyphIndex(int codepoint)
{
    std::unordered_map<int, int>::const_iterator it = m_glyphCache.find(codepoint);
    if (it != m_glyphCache.end())
        return it->second;

    int fallbackIndex = 0;
    for (int i = 0; i < m_glyphCount; ++i)
    {
        if (m_glyphs[(size_t)i].value == '?')
        {
            fallbackIndex = i;
            break;
        }
    }
    return fallbackIndex;
}

float SpriteFont::GetCharWidth(int codepoint)
{
    if (!m_loaded || m_glyphCount <= 0)
        return 0.0f;

    int index = getGlyphIndex(codepoint);
    float scaleFactor = fontSize / (float)m_baseSize;
    if (m_glyphs[(size_t)index].advanceX != 0)
        return m_glyphs[(size_t)index].advanceX * scaleFactor;
    return (m_recs[(size_t)index].width + m_glyphs[(size_t)index].offsetX) * scaleFactor;
}

TextMetrics SpriteFont::MeasureText(const char *text, float maxWidth)
{
    TextMetrics metrics;
    metrics.lineCount = 0;
    metrics.maxLineWidth = 0.0f;
    if (!text || !IsValid())
        return metrics;

    float scaleFactor = fontSize / (float)m_baseSize;
    float lineHeight = m_baseSize * scaleFactor;
    float extraLineSpacing = (textLineSpacing > 0.0f) ? textLineSpacing : (lineHeight * 0.15f);
    metrics.lineHeight = lineHeight + extraLineSpacing;

    float currentWidth = 0.0f;
    const char *ptr = text;
    while (*ptr)
    {
        int codepointSize = 1;
        const int codepoint = get_codepoint_next(ptr, &codepointSize);
        ptr += codepointSize;

        if (codepoint == '\n')
        {
            metrics.lineWidths.push_back(currentWidth);
            metrics.maxLineWidth = std::max(metrics.maxLineWidth, currentWidth);
            metrics.lineCount++;
            currentWidth = 0.0f;
            continue;
        }

        int index = getGlyphIndex(codepoint);
        float charWidth = 0.0f;
        if (m_glyphs[(size_t)index].advanceX != 0)
            charWidth = m_glyphs[(size_t)index].advanceX * scaleFactor;
        else
            charWidth = (m_recs[(size_t)index].width + m_glyphs[(size_t)index].offsetX) * scaleFactor;

        currentWidth += charWidth + spacing;
        if (maxWidth > 0.0f && currentWidth > maxWidth)
            metrics.maxLineWidth = std::max(metrics.maxLineWidth, currentWidth);
    }

    if (currentWidth > 0.0f || metrics.lineCount == 0)
    {
        metrics.lineWidths.push_back(currentWidth);
        metrics.maxLineWidth = std::max(metrics.maxLineWidth, currentWidth);
        metrics.lineCount++;
    }

    metrics.size.x = metrics.maxLineWidth;
    metrics.size.y = metrics.lineHeight * (float)metrics.lineCount;
    return metrics;
}

Vector2 SpriteFont::GetTextSize(const char *text)
{
    return MeasureText(text, 0.0f).size;
}

float SpriteFont::GetTextWidth(const char *text)
{
    return GetTextSize(text).x;
}

float SpriteFont::GetTextHeight(const char *text)
{
    return GetTextSize(text).y;
}

std::vector<std::string> SpriteFont::WrapText(const char *text, float maxWidth)
{
    std::vector<std::string> lines;
    if (!text)
        return lines;
    if (maxWidth <= 0.0f)
    {
        lines.push_back(text);
        return lines;
    }

    std::istringstream stream(text);
    std::string current;
    std::string word;
    while (stream >> word)
    {
        std::string candidate = current.empty() ? word : current + " " + word;
        if (!current.empty() && GetTextWidth(candidate.c_str()) > maxWidth)
        {
            lines.push_back(current);
            current = word;
        }
        else
        {
            current = candidate;
        }
    }
    if (!current.empty())
        lines.push_back(current);
    if (lines.empty())
        lines.push_back("");
    return lines;
}

std::vector<std::string> SpriteFont::SplitIntoLines(const char *text)
{
    std::vector<std::string> lines;
    if (!text)
        return lines;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line))
        lines.push_back(line);
    if (lines.empty())
        lines.push_back("");
    return lines;
}

float SpriteFont::GetLineWidth(const char *line)
{
    return GetTextWidth(line);
}

void SpriteFont::SetColor(u8 r, u8 g, u8 b, u8 a)
{
    color = Color(r, g, b, a);
}

void SpriteFont::SetClip(int x, int y, int w, int h)
{
    clip = IntRect(x, y, w, h);
}

void SpriteFont::EnableClip(bool enable)
{
    enableClip = enable;
}

void SpriteFont::drawTextCodepoint(int codepoint, float x, float y)
{
    if (!batch || !texture || texture->id == 0 || !IsValid())
        return;

    int index = getGlyphIndex(codepoint);
    float scaleFactor = fontSize / (float)m_baseSize;

    FloatRect srcRec(
        m_recs[(size_t)index].x - (float)m_glyphPadding,
        m_recs[(size_t)index].y - (float)m_glyphPadding,
        m_recs[(size_t)index].width + 2.0f * m_glyphPadding,
        m_recs[(size_t)index].height + 2.0f * m_glyphPadding);

    FloatRect dstRec(
        x + m_glyphs[(size_t)index].offsetX * scaleFactor - (float)m_glyphPadding * scaleFactor,
        y + m_glyphs[(size_t)index].offsetY * scaleFactor - (float)m_glyphPadding * scaleFactor,
        (m_recs[(size_t)index].width + 2.0f * m_glyphPadding) * scaleFactor,
        (m_recs[(size_t)index].height + 2.0f * m_glyphPadding) * scaleFactor);

    if (enableClip)
    {
        if (dstRec.x < clip.x || dstRec.y < clip.y ||
            dstRec.x + dstRec.width > clip.x + clip.width ||
            dstRec.y + dstRec.height > clip.y + clip.height)
            return;
    }

    Vector2 coords[4];
    Vector2 texcoords[4];
    coords[0] = {dstRec.x, dstRec.y};
    coords[1] = {dstRec.x, dstRec.y + dstRec.height};
    coords[2] = {dstRec.x + dstRec.width, dstRec.y + dstRec.height};
    coords[3] = {dstRec.x + dstRec.width, dstRec.y};

    float texWidth = (float)texture->width;
    float texHeight = (float)texture->height;
    texcoords[0] = {srcRec.x / texWidth, srcRec.y / texHeight};
    texcoords[1] = {srcRec.x / texWidth, (srcRec.y + srcRec.height) / texHeight};
    texcoords[2] = {(srcRec.x + srcRec.width) / texWidth, (srcRec.y + srcRec.height) / texHeight};
    texcoords[3] = {(srcRec.x + srcRec.width) / texWidth, srcRec.y / texHeight};

    batch->SetColor(color);
    batch->Quad(texture->id, coords, texcoords);
}

void SpriteFont::Print(const char *text, float x, float y)
{
    if (!text || !batch || !IsValid())
        return;

    float textOffsetX = 0.0f;
    float textOffsetY = 0.0f;
    float scaleFactor = fontSize / (float)m_baseSize;
    float lineHeight = m_baseSize * scaleFactor;
    float extraLineSpacing = (textLineSpacing > 0.0f) ? textLineSpacing : 0.0f;

    const char *ptr = text;
    while (*ptr)
    {
        int codepointByteCount = 1;
        int codepoint = get_codepoint_next(ptr, &codepointByteCount);
        int index = getGlyphIndex(codepoint);

        if (codepoint == '\n')
        {
            textOffsetY += lineHeight + extraLineSpacing;
            textOffsetX = 0.0f;
        }
        else
        {
            if (codepoint != ' ' && codepoint != '\t')
                drawTextCodepoint(codepoint, x + textOffsetX, y + textOffsetY);

            if (m_glyphs[(size_t)index].advanceX == 0)
                textOffsetX += (m_recs[(size_t)index].width * scaleFactor + spacing);
            else
                textOffsetX += (m_glyphs[(size_t)index].advanceX * scaleFactor + spacing);
        }

        ptr += codepointByteCount;
    }
}

void SpriteFont::Print(float x, float y, const char *fmt, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    Print(buffer, x, y);
}

void SpriteFont::Print(const char *text, float x, float y, const TextStyle &style)
{
    const Color oldColor = color;
    const float oldFontSize = fontSize;
    const float oldSpacing = spacing;
    const float oldLineSpacing = textLineSpacing;
    color = style.color;
    fontSize = style.fontSize;
    spacing = style.spacing;
    textLineSpacing = style.lineSpacing;
    Print(text, x, y);
    color = oldColor;
    fontSize = oldFontSize;
    spacing = oldSpacing;
    textLineSpacing = oldLineSpacing;
}

void SpriteFont::PrintAligned(const char *text, const FloatRect &bounds, TextAlign align, TextVAlign valign)
{
    Vector2 textSize = GetTextSize(text);
    float x = bounds.x;
    float y = bounds.y;
    if (align == TextAlign::CENTER)
        x = bounds.x + (bounds.width - textSize.x) * 0.5f;
    else if (align == TextAlign::RIGHT)
        x = bounds.x + bounds.width - textSize.x;
    if (valign == TextVAlign::MIDDLE)
        y = bounds.y + (bounds.height - textSize.y) * 0.5f;
    else if (valign == TextVAlign::BOTTOM)
        y = bounds.y + bounds.height - textSize.y;
    Print(text, x, y);
}

void SpriteFont::PrintWrapped(const char *text, float x, float y, float maxWidth)
{
    const std::vector<std::string> lines = WrapText(text, maxWidth);
    float currentY = y;
    for (size_t i = 0; i < lines.size(); ++i)
    {
        Print(lines[i].c_str(), x, currentY);
        currentY += fontSize + textLineSpacing;
    }
}

void SpriteFont::PrintWrapped(const char *text, const FloatRect &bounds, const TextStyle &style)
{
    const Color oldColor = color;
    const float oldFontSize = fontSize;
    const float oldSpacing = spacing;
    const float oldLineSpacing = textLineSpacing;
    color = style.color;
    fontSize = style.fontSize;
    spacing = style.spacing;
    textLineSpacing = style.lineSpacing;
    PrintWrapped(text, bounds.x, bounds.y, bounds.width);
    color = oldColor;
    fontSize = oldFontSize;
    spacing = oldSpacing;
    textLineSpacing = oldLineSpacing;
}

void SpriteFont::PrintWithShadow(const char *text, float x, float y, const Color &textColor, const Color &shadowColor, const Vector2 &shadowOffset)
{
    const Color oldColor = color;
    color = shadowColor;
    Print(text, x + shadowOffset.x, y + shadowOffset.y);
    color = textColor;
    Print(text, x, y);
    color = oldColor;
}

void SpriteFont::PrintWithOutline(const char *text, float x, float y, const Color &textColor, const Color &outlineColor, float thickness)
{
    const Color oldColor = color;
    color = outlineColor;
    Print(text, x - thickness, y);
    Print(text, x + thickness, y);
    Print(text, x, y - thickness);
    Print(text, x, y + thickness);
    color = textColor;
    Print(text, x, y);
    color = oldColor;
}
