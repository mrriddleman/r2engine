//
//  Utils.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-03-18.
//

#include "r2/Utils/Utils.h"
#include <cmath>

namespace r2
{
    namespace util
    {
        u32 NextPowerOfTwo32Bit(u32 n)
        {
            --n;
            
            n |= n >> 1;
            n |= n >> 2;
            n |= n >> 4;
            n |= n >> 8;
            n |= n >> 16;
            
            return n + 1;
        }
        
        u64 NextPowerOfTwo64Bit(u64 n)
        {
            --n;
            
            n |= n >> 1;
            n |= n >> 2;
            n |= n >> 4;
            n |= n >> 8;
            n |= n >> 16;
            n |= n >> 32;
            
            return n + 1;
        }
        
        bool IsPowerOfTwo(u64 n)
        {
            return ceil(log2(n)) == floor(log2(n));
        }
        
        u64 RoundUp(u64 numToRound, u64 multiple)
        {
            if (multiple == 0)
                return numToRound;
            
            u64 remainder = numToRound % multiple;
            if (remainder == 0)
                return numToRound;
            
            return numToRound + multiple - remainder;
        }
    }
}
