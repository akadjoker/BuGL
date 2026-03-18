#include "interpreter.hpp"

#ifdef BU_ENABLE_CRYPTO

#include <cstring>
#include <cstdint>
#include <random>

// ============================================
// BASE64 ENCODE/DECODE
// ============================================

static const char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const int base64_inv[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1
};

static std::string base64_encode(const uint8_t *data, size_t len)
{
    std::string result;
    result.reserve(((len + 2) / 3) * 4);

    for (size_t i = 0; i < len; i += 3)
    {
        uint32_t n = ((uint32_t)data[i]) << 16;
        if (i + 1 < len) n |= ((uint32_t)data[i + 1]) << 8;
        if (i + 2 < len) n |= data[i + 2];

        result.push_back(base64_chars[(n >> 18) & 0x3F]);
        result.push_back(base64_chars[(n >> 12) & 0x3F]);
        result.push_back((i + 1 < len) ? base64_chars[(n >> 6) & 0x3F] : '=');
        result.push_back((i + 2 < len) ? base64_chars[n & 0x3F] : '=');
    }

    return result;
}

static std::string base64_decode(const char *data, size_t len)
{
    std::string result;
    if (len == 0) return result;

    result.reserve((len / 4) * 3);

    uint32_t n = 0;
    int bits = 0;

    for (size_t i = 0; i < len; ++i)
    {
        char c = data[i];
        if (c == '=' || c == '\n' || c == '\r' || c == ' ') continue;
        if (c < 0 || c >= 128) continue;

        int val = base64_inv[(int)c];
        if (val < 0) continue;

        n = (n << 6) | val;
        bits += 6;

        if (bits >= 8)
        {
            bits -= 8;
            result.push_back((char)((n >> bits) & 0xFF));
        }
    }

    return result;
}

// base64.encode(string) -> string
static int native_base64_encode(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isString())
    {
        vm->runtimeError("base64.encode expects a string");
        return 0;
    }

    const char *str = args[0].asStringChars();
    size_t len = args[0].asString()->length();

    std::string encoded = base64_encode((const uint8_t *)str, len);
    vm->push(vm->makeString(encoded.c_str()));
    return 1;
}

// base64.decode(string) -> string
static int native_base64_decode(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isString())
    {
        vm->runtimeError("base64.decode expects a string");
        return 0;
    }

    const char *str = args[0].asStringChars();
    size_t len = args[0].asString()->length();

    std::string decoded = base64_decode(str, len);
    vm->push(vm->makeString(decoded.c_str()));
    return 1;
}

// ============================================
// MD5 HASH
// ============================================

struct MD5Context
{
    uint32_t state[4];
    uint32_t count[2];
    uint8_t buffer[64];
};

static void md5_transform(uint32_t state[4], const uint8_t block[64]);

static void md5_init(MD5Context *ctx)
{
    ctx->count[0] = ctx->count[1] = 0;
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xefcdab89;
    ctx->state[2] = 0x98badcfe;
    ctx->state[3] = 0x10325476;
}

static void md5_update(MD5Context *ctx, const uint8_t *input, size_t len)
{
    size_t i, index, partLen;

    index = (ctx->count[0] >> 3) & 0x3F;

    if ((ctx->count[0] += ((uint32_t)len << 3)) < ((uint32_t)len << 3))
        ctx->count[1]++;
    ctx->count[1] += ((uint32_t)len >> 29);

    partLen = 64 - index;

    if (len >= partLen)
    {
        memcpy(&ctx->buffer[index], input, partLen);
        md5_transform(ctx->state, ctx->buffer);

        for (i = partLen; i + 63 < len; i += 64)
            md5_transform(ctx->state, &input[i]);

        index = 0;
    }
    else
    {
        i = 0;
    }

    memcpy(&ctx->buffer[index], &input[i], len - i);
}

static void md5_final(uint8_t digest[16], MD5Context *ctx)
{
    static uint8_t padding[64] = {
        0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    uint8_t bits[8];
    size_t index, padLen;

    for (int i = 0; i < 4; i++)
    {
        bits[i] = (uint8_t)(ctx->count[0] >> (i * 8));
        bits[i + 4] = (uint8_t)(ctx->count[1] >> (i * 8));
    }

    index = (ctx->count[0] >> 3) & 0x3f;
    padLen = (index < 56) ? (56 - index) : (120 - index);
    md5_update(ctx, padding, padLen);
    md5_update(ctx, bits, 8);

    for (int i = 0; i < 4; i++)
    {
        digest[i] = (uint8_t)(ctx->state[0] >> (i * 8));
        digest[i + 4] = (uint8_t)(ctx->state[1] >> (i * 8));
        digest[i + 8] = (uint8_t)(ctx->state[2] >> (i * 8));
        digest[i + 12] = (uint8_t)(ctx->state[3] >> (i * 8));
    }
}

#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))

#define FF(a, b, c, d, x, s, ac) { \
    (a) += F((b), (c), (d)) + (x) + (uint32_t)(ac); \
    (a) = ROTATE_LEFT((a), (s)); \
    (a) += (b); \
}
#define GG(a, b, c, d, x, s, ac) { \
    (a) += G((b), (c), (d)) + (x) + (uint32_t)(ac); \
    (a) = ROTATE_LEFT((a), (s)); \
    (a) += (b); \
}
#define HH(a, b, c, d, x, s, ac) { \
    (a) += H((b), (c), (d)) + (x) + (uint32_t)(ac); \
    (a) = ROTATE_LEFT((a), (s)); \
    (a) += (b); \
}
#define II(a, b, c, d, x, s, ac) { \
    (a) += I((b), (c), (d)) + (x) + (uint32_t)(ac); \
    (a) = ROTATE_LEFT((a), (s)); \
    (a) += (b); \
}

static void md5_transform(uint32_t state[4], const uint8_t block[64])
{
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    uint32_t x[16];

    for (int i = 0; i < 16; i++)
        x[i] = ((uint32_t)block[i*4]) | (((uint32_t)block[i*4+1]) << 8) |
               (((uint32_t)block[i*4+2]) << 16) | (((uint32_t)block[i*4+3]) << 24);

    FF(a, b, c, d, x[ 0],  7, 0xd76aa478); FF(d, a, b, c, x[ 1], 12, 0xe8c7b756);
    FF(c, d, a, b, x[ 2], 17, 0x242070db); FF(b, c, d, a, x[ 3], 22, 0xc1bdceee);
    FF(a, b, c, d, x[ 4],  7, 0xf57c0faf); FF(d, a, b, c, x[ 5], 12, 0x4787c62a);
    FF(c, d, a, b, x[ 6], 17, 0xa8304613); FF(b, c, d, a, x[ 7], 22, 0xfd469501);
    FF(a, b, c, d, x[ 8],  7, 0x698098d8); FF(d, a, b, c, x[ 9], 12, 0x8b44f7af);
    FF(c, d, a, b, x[10], 17, 0xffff5bb1); FF(b, c, d, a, x[11], 22, 0x895cd7be);
    FF(a, b, c, d, x[12],  7, 0x6b901122); FF(d, a, b, c, x[13], 12, 0xfd987193);
    FF(c, d, a, b, x[14], 17, 0xa679438e); FF(b, c, d, a, x[15], 22, 0x49b40821);

    GG(a, b, c, d, x[ 1],  5, 0xf61e2562); GG(d, a, b, c, x[ 6],  9, 0xc040b340);
    GG(c, d, a, b, x[11], 14, 0x265e5a51); GG(b, c, d, a, x[ 0], 20, 0xe9b6c7aa);
    GG(a, b, c, d, x[ 5],  5, 0xd62f105d); GG(d, a, b, c, x[10],  9, 0x02441453);
    GG(c, d, a, b, x[15], 14, 0xd8a1e681); GG(b, c, d, a, x[ 4], 20, 0xe7d3fbc8);
    GG(a, b, c, d, x[ 9],  5, 0x21e1cde6); GG(d, a, b, c, x[14],  9, 0xc33707d6);
    GG(c, d, a, b, x[ 3], 14, 0xf4d50d87); GG(b, c, d, a, x[ 8], 20, 0x455a14ed);
    GG(a, b, c, d, x[13],  5, 0xa9e3e905); GG(d, a, b, c, x[ 2],  9, 0xfcefa3f8);
    GG(c, d, a, b, x[ 7], 14, 0x676f02d9); GG(b, c, d, a, x[12], 20, 0x8d2a4c8a);

    HH(a, b, c, d, x[ 5],  4, 0xfffa3942); HH(d, a, b, c, x[ 8], 11, 0x8771f681);
    HH(c, d, a, b, x[11], 16, 0x6d9d6122); HH(b, c, d, a, x[14], 23, 0xfde5380c);
    HH(a, b, c, d, x[ 1],  4, 0xa4beea44); HH(d, a, b, c, x[ 4], 11, 0x4bdecfa9);
    HH(c, d, a, b, x[ 7], 16, 0xf6bb4b60); HH(b, c, d, a, x[10], 23, 0xbebfbc70);
    HH(a, b, c, d, x[13],  4, 0x289b7ec6); HH(d, a, b, c, x[ 0], 11, 0xeaa127fa);
    HH(c, d, a, b, x[ 3], 16, 0xd4ef3085); HH(b, c, d, a, x[ 6], 23, 0x04881d05);
    HH(a, b, c, d, x[ 9],  4, 0xd9d4d039); HH(d, a, b, c, x[12], 11, 0xe6db99e5);
    HH(c, d, a, b, x[15], 16, 0x1fa27cf8); HH(b, c, d, a, x[ 2], 23, 0xc4ac5665);

    II(a, b, c, d, x[ 0],  6, 0xf4292244); II(d, a, b, c, x[ 7], 10, 0x432aff97);
    II(c, d, a, b, x[14], 15, 0xab9423a7); II(b, c, d, a, x[ 5], 21, 0xfc93a039);
    II(a, b, c, d, x[12],  6, 0x655b59c3); II(d, a, b, c, x[ 3], 10, 0x8f0ccc92);
    II(c, d, a, b, x[10], 15, 0xffeff47d); II(b, c, d, a, x[ 1], 21, 0x85845dd1);
    II(a, b, c, d, x[ 8],  6, 0x6fa87e4f); II(d, a, b, c, x[15], 10, 0xfe2ce6e0);
    II(c, d, a, b, x[ 6], 15, 0xa3014314); II(b, c, d, a, x[13], 21, 0x4e0811a1);
    II(a, b, c, d, x[ 4],  6, 0xf7537e82); II(d, a, b, c, x[11], 10, 0xbd3af235);
    II(c, d, a, b, x[ 2], 15, 0x2ad7d2bb); II(b, c, d, a, x[ 9], 21, 0xeb86d391);

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

static std::string md5_hash(const uint8_t *data, size_t len)
{
    MD5Context ctx;
    uint8_t digest[16];
    char hex[33];

    md5_init(&ctx);
    md5_update(&ctx, data, len);
    md5_final(digest, &ctx);

    for (int i = 0; i < 16; i++)
        snprintf(hex + i * 2, 3, "%02x", digest[i]);
    hex[32] = '\0';

    return std::string(hex);
}

// hash.md5(string) -> string (hex)
static int native_hash_md5(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isString())
    {
        vm->runtimeError("hash.md5 expects a string");
        return 0;
    }

    const char *str = args[0].asStringChars();
    size_t len = args[0].asString()->length();

    std::string hash = md5_hash((const uint8_t *)str, len);
    vm->push(vm->makeString(hash.c_str()));
    return 1;
}

// ============================================
// SHA-256 HASH
// ============================================

static const uint32_t sha256_k[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

static std::string sha256_hash(const uint8_t *data, size_t len)
{
    uint32_t state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    // Padding
    size_t bit_len = len * 8;
    size_t padded_len = ((len + 8) / 64 + 1) * 64;
    std::vector<uint8_t> padded(padded_len, 0);
    memcpy(padded.data(), data, len);
    padded[len] = 0x80;

    // Length in bits (big-endian)
    for (int i = 0; i < 8; i++)
        padded[padded_len - 1 - i] = (uint8_t)(bit_len >> (i * 8));

    // Process blocks
    for (size_t i = 0; i < padded_len; i += 64)
    {
        uint32_t w[64];
        for (int j = 0; j < 16; j++)
        {
            w[j] = ((uint32_t)padded[i + j * 4] << 24) |
                   ((uint32_t)padded[i + j * 4 + 1] << 16) |
                   ((uint32_t)padded[i + j * 4 + 2] << 8) |
                   ((uint32_t)padded[i + j * 4 + 3]);
        }
        for (int j = 16; j < 64; j++)
            w[j] = SIG1(w[j - 2]) + w[j - 7] + SIG0(w[j - 15]) + w[j - 16];

        uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
        uint32_t e = state[4], f = state[5], g = state[6], h = state[7];

        for (int j = 0; j < 64; j++)
        {
            uint32_t t1 = h + EP1(e) + CH(e, f, g) + sha256_k[j] + w[j];
            uint32_t t2 = EP0(a) + MAJ(a, b, c);
            h = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }

        state[0] += a; state[1] += b; state[2] += c; state[3] += d;
        state[4] += e; state[5] += f; state[6] += g; state[7] += h;
    }

    char hex[65];
    for (int i = 0; i < 8; i++)
        snprintf(hex + i * 8, 9, "%08x", state[i]);
    hex[64] = '\0';

    return std::string(hex);
}

// hash.sha256(string) -> string (hex)
static int native_hash_sha256(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isString())
    {
        vm->runtimeError("hash.sha256 expects a string");
        return 0;
    }

    const char *str = args[0].asStringChars();
    size_t len = args[0].asString()->length();

    std::string hash = sha256_hash((const uint8_t *)str, len);
    vm->push(vm->makeString(hash.c_str()));
    return 1;
}

// ============================================
// UUID v4
// ============================================

static std::string generate_uuid_v4()
{
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dis;

    uint64_t ab = dis(gen);
    uint64_t cd = dis(gen);

    // Set version 4
    ab = (ab & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
    // Set variant
    cd = (cd & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

    char uuid[37];
    snprintf(uuid, sizeof(uuid),
             "%08x-%04x-%04x-%04x-%012llx",
             (uint32_t)(ab >> 32),
             (uint16_t)(ab >> 16),
             (uint16_t)(ab),
             (uint16_t)(cd >> 48),
             (unsigned long long)(cd & 0x0000FFFFFFFFFFFFULL));

    return std::string(uuid);
}

// uuid.v4() -> string
static int native_uuid_v4(Interpreter *vm, int argCount, Value *args)
{
    std::string uuid = generate_uuid_v4();
    vm->push(vm->makeString(uuid.c_str()));
    return 1;
}

// ============================================
// HEX ENCODE/DECODE
// ============================================

// hex.encode(string) -> string
static int native_hex_encode(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isString())
    {
        vm->runtimeError("hex.encode expects a string");
        return 0;
    }

    const char *str = args[0].asStringChars();
    size_t len = args[0].asString()->length();

    std::string result;
    result.reserve(len * 2);

    for (size_t i = 0; i < len; i++)
    {
        char hex[3];
        snprintf(hex, sizeof(hex), "%02x", (uint8_t)str[i]);
        result += hex;
    }

    vm->push(vm->makeString(result.c_str()));
    return 1;
}

// hex.decode(string) -> string
static int native_hex_decode(Interpreter *vm, int argCount, Value *args)
{
    if (argCount < 1 || !args[0].isString())
    {
        vm->runtimeError("hex.decode expects a string");
        return 0;
    }

    const char *str = args[0].asStringChars();
    size_t len = args[0].asString()->length();

    if (len % 2 != 0)
    {
        vm->runtimeError("hex.decode: invalid hex string length");
        return 0;
    }

    std::string result;
    result.reserve(len / 2);

    for (size_t i = 0; i < len; i += 2)
    {
        char hex[3] = {str[i], str[i + 1], '\0'};
        char *end;
        long val = strtol(hex, &end, 16);
        if (end != hex + 2)
        {
            vm->runtimeError("hex.decode: invalid hex character");
            return 0;
        }
        result.push_back((char)val);
    }

    vm->push(vm->makeString(result.c_str()));
    return 1;
}

// ============================================
// REGISTRATION
// ============================================

void Interpreter::registerCrypto()
{
    addModule("base64")
        .addFunction("encode", native_base64_encode, 1)
        .addFunction("decode", native_base64_decode, 1);

    addModule("hex")
        .addFunction("encode", native_hex_encode, 1)
        .addFunction("decode", native_hex_decode, 1);

    addModule("hash")
        .addFunction("md5", native_hash_md5, 1)
        .addFunction("sha256", native_hash_sha256, 1);

    addModule("uuid")
        .addFunction("v4", native_uuid_v4, 0);
}

#endif
