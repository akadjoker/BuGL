#pragma once
#include <SDL2/SDL.h>
#include <string>

class BinaryStream
{
public:
    BinaryStream(const std::string &path, const char *mode);
    ~BinaryStream();

    bool isOpen() const { return rw_ != nullptr; }

    // ── Write ────────────────────────────────────────────────
    void writeU8(uint8_t v);
    void writeU32(uint32_t v);           // little-endian
    void writeI32(int32_t v);            // little-endian
    void writeF32(float v);              // IEEE 754, little-endian
    void writeStr(const std::string &s); // null-terminated UTF-8
    void writeRaw(const void *data, size_t bytes);

    // ── Read ─────────────────────────────────────────────────
    uint8_t readU8();
    uint32_t readU32();
    int32_t readI32();
    float readF32();
    std::string readStr();
    void readRaw(void *data, size_t bytes);

    // ── Seek / Tell ──────────────────────────────────────────
    Sint64 tell() const;
    void seek(Sint64 pos);
    Sint64 size();
    bool eof() const;

private:
    SDL_RWops *rw_ = nullptr;
};