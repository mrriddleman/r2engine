//
//  Utils.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-03-18.
//
#include "r2pch.h"
#include "r2/Utils/Utils.h"
#include <cmath>


namespace r2::mem::utils
{
    u64 GetMaxMemoryForAllocation(u64 allocationSize, u64 alignment, u32 headerSize, u32 boundsChecking)
    {
        return r2::util::RoundUp(allocationSize + headerSize + boundsChecking, alignment);
    }
}

namespace r2::util
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
    
    float MillisecondsToSeconds(u32 milliseconds)
    {
        return static_cast<float>(milliseconds) / 1000.f;
    }
    
    u32 SecondsToMilliseconds(float seconds)
    {
        return static_cast<u32>(seconds * 1000);
    }
    
    bool WildcardMatch(
                            const char * cpTameText,             // A string without wildcards
                            const char * cpWildText,             // A (potentially) corresponding string with wildcards
                            bool bCaseSensitive,  // By default, match on 'X' vs 'x'
                            char cAltTerminator   // For function names, for example, you can stop at the first '('
    )
    {
        char * pTameText = const_cast<char*>(cpTameText);
        char * pWildText = const_cast<char*>(cpWildText);
        
        bool bMatch = true;
        char * pAfterLastWild = NULL; // The location after the last '*', if we’ve encountered one
        char * pAfterLastTame = NULL; // The location in the tame string, from which we started after last wildcard
        char t, w;
        
        // Walk the text strings one character at a time.
        while (1)
        {
            t = *pTameText;
            w = *pWildText;
            
            // How do you match a unique text string?
            if (!t || t == cAltTerminator)
            {
                // Easy: unique up on it!
                if (!w || w == cAltTerminator)
                {
                    break;                                   // "x" matches "x"
                }
                else if (w == '*')
                {
                    pWildText++;
                    continue;                           // "x*" matches "x" or "xy"
                }
                else if (pAfterLastTame)
                {
                    if (!(*pAfterLastTame) || *pAfterLastTame == cAltTerminator)
                    {
                        bMatch = false;
                        break;
                    }
                    pTameText = pAfterLastTame++;
                    pWildText = pAfterLastWild;
                    continue;
                }
                
                bMatch = false;
                break;                                           // "x" doesn't match "xy"
            }
            else
            {
                if (!bCaseSensitive)
                {
                    // Lowercase the characters to be compared.
                    if (t >= 'A' && t <= 'Z')
                    {
                        t += ('a' - 'A');
                    }
                    
                    if (w >= 'A' && w <= 'Z')
                    {
                        w += ('a' - 'A');
                    }
                }
                
                // How do you match a tame text string?
                if (t != w)
                {
                    // The tame way: unique up on it!
                    if (w == '*')
                    {
                        pAfterLastWild = ++pWildText;
                        pAfterLastTame = pTameText;
                        w = *pWildText;
                        
                        if (!w || w == cAltTerminator)
                        {
                            break;                           // "*" matches "x"
                        }
                        continue;                           // "*y" matches "xy"
                    }
                    else if (pAfterLastWild)
                    {
                        if (pAfterLastWild != pWildText)
                        {
                            pWildText = pAfterLastWild;
                            w = *pWildText;
                            
                            if (!bCaseSensitive && w >= 'A' && w <= 'Z')
                            {
                                w += ('a' - 'A');
                            }
                            
                            if (t == w)
                            {
                                pWildText++;
                            }
                        }
                        pTameText++;
                        continue;                           // "*sip*" matches "mississippi"
                    }
                    else
                    {
                        bMatch = false;
                        break;                                   // "x" doesn't match "y"
                    }
                }
            }
            
            pTameText++;
            pWildText++;
        }
        
        return bMatch;
    }

    void PathCpy(char* dest, const char* source, u64 destLength)
    {
#ifdef R2_PLATFORM_WINDOWS
        strcpy_s(dest, destLength, source);
#else
        strcpy(source, dest);
#endif 

    }

    void PathCat(char* dest, const char* source, u64 destLength)
    {
#ifdef R2_PLATFORM_WINDOWS
        strcat_s(dest, destLength, source);
#else
        strcat(source, dest);
#endif 
    }

    void PathNCpy(char* dest, const char* source, u64 num, u64 destLength)
    {
#ifdef R2_PLATFORM_WINDOWS
        strncpy_s(dest, destLength, source, num);
#else
        strncpy(source, dest, num);
#endif 
    }

    bool AreSizesEqual(const Size& s1, const Size& s2)
    {
        return s1.width == s2.width && s1.height == s2.height;
    }

    bool IsSizeEqual(const Size& s1, u32 width, u32 height)
    {
        return s1.width == width && s1.height == height;
    }

	u32 AsU32(const f32 x)
	{
		return *(u32*)&x;
	}

	f32 AsF32(const u32 x)
	{
		return *(float*)&x;
	}

	f32 HalfToFloat(const u16 x)
	{
		const u32 e = (x & 0x7C00) >> 10; // exponent
		const u32 m = (x & 0x03FF) << 13; // mantissa
		const u32 v = AsU32((float)m) >> 23; // evil log2 bit hack to count leading zeros in denormalized format
		return AsF32((x & 0x8000) << 16 | (e != 0) * ((e + 112) << 23 | m) | ((e == 0) & (m != 0)) * ((v - 37) << 23 | ((m << (150 - v)) & 0x007FE000))); // sign : normalized : denormalized
	}


	u16 FloatToHalf(const f32 x)
	{
		const u32 b = AsU32(x) + 0x00001000; // round-to-nearest-even: add last bit after truncated mantissa
		const u32 e = (b & 0x7F800000) >> 23; // exponent
		const u32 m = b & 0x007FFFFF; // mantissa; in line below: 0x007FF000 = 0x00800000-0x00001000 = decimal indicator flag - initial rounding

		return (b & 0x80000000) >> 16 | (e > 112) * ((((e - 112) << 10) & 0x7C00) | m >> 13) | ((e < 113) & (e > 101)) * ((((0x007FF000 + m) >> (125 - e)) + 1) >> 1) | (e > 143) * 0x7FFF; // sign : normalized : denormalized : saturate
	}

	u32 FloatToU24(const f32 x)
	{
		const u32 b = AsU32(x) + 0x00001000; // round-to-nearest-even: add last bit after truncated mantissa
		const u32 e = (b & 0x7F800000) >> 23; // exponent
		const u32 m = b & 0x007FFFFF; // mantissa; in line below: 0x007FF000 = 0x00800000-0x00001000 = decimal indicator flag - initial rounding

		return (b & 0x80000000) >> 8 | (e > 104) * ((((e - 104) << 12) & 0x7FF000) | m >> 11) | ((e < 105) & (e > 93)) * ((((0x007FF000 + m) >> (125 - e)) + 1) >> 1) | (e > 151) * 0x7FFFFF;
	}

    u32 GetWarpSize()
    {
        return 32;
    }
}
