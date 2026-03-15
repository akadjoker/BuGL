
#include "binaryStream.hpp"
// ============================================================
//  BinaryStream — SDL_RWops, little-endian explícito
// ============================================================
BinaryStream::BinaryStream(const std::string &path, const char *mode)
{
    rw_ = SDL_RWFromFile(path.c_str(), mode);
    if (!rw_)
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "[BinaryStream] SDL_RWFromFile failed: %s — %s",
                     path.c_str(), SDL_GetError());
}

BinaryStream::~BinaryStream()
{
    if (rw_)
        SDL_RWclose(rw_);
}

// ── Write ────────────────────────────────────────────────────
void BinaryStream::writeU8(uint8_t v)
{
    SDL_RWwrite(rw_, &v, 1, 1);
}

void BinaryStream::writeU32(uint32_t v)
{
    uint32_t le = SDL_SwapLE32(v);
    SDL_RWwrite(rw_, &le, 4, 1);
}

void BinaryStream::writeI32(int32_t v)
{
    writeU32((uint32_t)v);
}

void BinaryStream::writeF32(float v)
{
    // Reinterpret float como uint32 para swap seguro
    uint32_t bits;
    memcpy(&bits, &v, 4);
    writeU32(bits);
}

void BinaryStream::writeStr(const std::string &s)
{
    SDL_RWwrite(rw_, s.c_str(), 1, s.size() + 1);
}

void BinaryStream::writeRaw(const void *data, size_t bytes)
{
    SDL_RWwrite(rw_, data, 1, bytes);
}

// ── Read ─────────────────────────────────────────────────────
uint8_t BinaryStream::readU8()
{
    uint8_t v = 0;
    SDL_RWread(rw_, &v, 1, 1);
    return v;
}

uint32_t BinaryStream::readU32()
{
    uint32_t v = 0;
    SDL_RWread(rw_, &v, 4, 1);
    return SDL_SwapLE32(v);
}

int32_t BinaryStream::readI32()
{
    return (int32_t)readU32();
}

float BinaryStream::readF32()
{
    uint32_t bits = readU32();
    float v;
    memcpy(&v, &bits, 4);
    return v;
}

std::string BinaryStream::readStr()
{
    std::string r;
    r.reserve(64);
    while (true)
    {
        uint8_t c = readU8();
        if (c == 0)
            break;
        r += (char)c;
    }
    return r;
}

void BinaryStream::readRaw(void *data, size_t bytes)
{
    SDL_RWread(rw_, data, 1, bytes);
}

// ── Seek / Tell ──────────────────────────────────────────────
Sint64 BinaryStream::tell() const
{
    return SDL_RWtell(rw_);
}

void BinaryStream::seek(Sint64 pos)
{
    SDL_RWseek(rw_, pos, RW_SEEK_SET);
}

Sint64 BinaryStream::size()
{
    return SDL_RWsize(rw_);
}

bool BinaryStream::eof() const
{
     
    return SDL_RWtell(rw_) >= SDL_RWsize(rw_);
}