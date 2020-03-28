//
//  Random.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-03-16.
//
#include "r2pch.h"
#include "Random.h"
#include <ctime>

namespace r2
{
    namespace util
    {
        Random::Random()
        {
            rseed = 1;
            rseed_sp = 0;
            mti = CMATH_N+1;
        }
        
        
        u32 Random::RandomNum(u32 n)
        {
            u32 y;
            static u32 mag01[2] = {0x0, CMATH_MATRIX_A};
            
            if (n == 0)
            {
                return 0;
            }
            
            if (mti >= CMATH_N)
            {
                int kk;
                if (mti == CMATH_N+1)
                {
                    SetRandomSeed(4357);
                }
                
                for (kk = 0; kk <CMATH_N - CMATH_M; ++kk)
                {
                    y = (mt[kk]&CMATH_UPPER_MASK)|(mt[kk+1] & CMATH_LOWER_MASK);
                    mt[kk] = mt[kk+CMATH_M] ^ (y >> 1) ^ mag01[y & 0x1];
                }
                
                for (; kk<CMATH_N - 1; ++kk)
                {
                    y = (mt[kk]&CMATH_UPPER_MASK)|(mt[0]&CMATH_LOWER_MASK);
                    mt[kk] = mt[kk+(CMATH_M - CMATH_N)] ^ (y >> 1) ^ mag01[y & 0x1];
                }
                
                y = (mt[CMATH_N-1]&CMATH_UPPER_MASK)|(mt[0]&CMATH_LOWER_MASK);
                mt[CMATH_N-1] = mt[CMATH_M-1] ^ (y >> 1) ^ mag01[y & 0x1];
                
                mti = 0;
            }
            
            y = mt[mti++];
            y ^= CMATH_TEMPERING_SHIFT_U(y);
            y ^= CMATH_TEMPERING_SHIFT_S(y) & CMATH_TEMPERING_MASK_B;
            y ^= CMATH_TEMPERING_SHIFT_T(y) & CMATH_TEMPERING_MASK_C;
            y ^= CMATH_TEMPERING_SHIFT_L(y);
            
            return (y%n);
        }
        
        u32 Random::RandomNum(u32 from, u32 to)
        {
            if (to < from)
            {
                return 0;
            }
            
            unsigned int diff = to - from;
            
            return RandomNum(diff+1) + from ;
        }
        
        float Random::Randomf()
        {
            float r = (float)RandomNum(INT_MAX);
            float divisor = (float)INT_MAX;
            return (r / divisor);
        }
        
        void Random::SetRandomSeed(u32 n)
        {
            mt[0] = n & 0xffffffff;
            for (mti=1; mti < CMATH_N; ++mti)
            {
                mt[mti] = (69069*mt[mti-1]) & 0xffffffff;
            }
            
            rseed = n;
        }
        
        u32 Random::GetRandomSeed()
        {
            return rseed;
        }
        
        void Random::Randomize()
        {
            SetRandomSeed((u32) time(nullptr));
        }
    }
}
