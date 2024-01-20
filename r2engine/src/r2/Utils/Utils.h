//
//  Utils.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-03.
//

#ifndef Utils_h
#define Utils_h

#include <cstdint>
#include <functional>

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using s8  = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using f32 = float;
using f64 = double;

using b32 = u32;
using byte = u8;

using uptr = uintptr_t;

#define BytesToBits(x) ((x)*8)
#define Kilobytes(x) ((x)*1024LL)
#define Megabytes(x) (Kilobytes(x)*1024LL)
#define Gigabytes(x) (Megabytes(x)*1024LL)
#define Terabytes(x) (Gigabytes(x)*1024LL)
#define BytesToKilobytes(x) (f64(x) / 1024.0)
#define BytesToMegabytes(x) (f64(x) / f64(1024.0 * 1024.0))
#define BytesToGigabytes(x) (f64(x) / f64(1024.0 * 1024.0 * 1024.0))

#define COUNT_OF(_Array) (sizeof(_Array) / sizeof(_Array[0]))

namespace r2::fs
{
    static const u32 FILE_PATH_LENGTH = 256;
}

namespace r2::mem::utils
{
    u64 GetMaxMemoryForAllocation(u64 allocationSize, u64 alignment, u32 headerSize = 0, u32 boundsChecking = 0);
}

namespace r2::asset
{
    using PathResolver = std::function<bool (u32 category, char* path)>;
}

namespace r2
{
    const u32 INITIAL_SCREEN_WIDTH = 1280;
    const u32 INITIAL_SCREEN_HEIGHT = 720;
}

namespace r2::util
{
    struct Size
    {
        u32 width = 0, height = 0;
    };
    
    struct Rect
    {
        s32 x = 0, y = 0;
        s32 w = 0, h = 0;
    };
    
    u32 NextPowerOfTwo32Bit(u32 n);
    u64 NextPowerOfTwo64Bit(u64 n);
    bool IsPowerOfTwo(u64 n);
    u64 RoundUp(u64 numToRound, u64 multiple);
    float MillisecondsToSeconds(u32 milliseconds);
    float MillisecondsToSecondsF32(float milliseconds);
    u32 SecondsToMilliseconds(float seconds);
    bool WildcardMatch(const char* pTameText, const char* pWildText, bool bCaseSensitive = false, char cAltTerminator ='\0');
    void PathCpy(char* dest, const char* source, u64 destLength = r2::fs::FILE_PATH_LENGTH);
    void PathCat(char* dest, const char* source, u64 destLength = r2::fs::FILE_PATH_LENGTH);
    void PathNCpy(char* dest, const char* source, u64 num, u64 destLength = r2::fs::FILE_PATH_LENGTH);

    bool AreSizesEqual(const Size& s1, const Size& s2);
    bool IsSizeEqual(const Size& s1, u32 width, u32 height);

    u32 AsU32(const f32 x);
    f32 AsF32(const u32 x);

    f32 HalfToFloat(const u16 x);
    u16 FloatToHalf(const f32 x);

    u32 Pack216BitValues(const u16 high, const u16 low);

    u32 FloatToU24(const f32 x);

    u32 GetWarpSize();
}

#endif /* Utils_h */
