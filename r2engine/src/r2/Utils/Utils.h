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

namespace r2
{
    struct Resolution
    {
        u32 width, height;
    };
}

#endif /* Utils_h */
