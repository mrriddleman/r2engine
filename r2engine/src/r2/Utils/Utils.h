//
//  Utils.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-03.
//

#ifndef Utils_h
#define Utils_h

using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using s8  = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using s64 = int64_t;

using b32 = u32;
using byte = char;

using uptr = uintptr_t;

#define Kilobytes(x) ((x)*1024LL)
#define Megabytes(x) (Kilobytes(x)*1024LL)
#define Gigabytes(x) (Megabytes(x)*1024LL)
#define Terabytes(x) (Gigabytes(x)*1024LL)

namespace r2
{
    namespace util
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
    }
}

#endif /* Utils_h */
