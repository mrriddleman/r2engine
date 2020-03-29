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
    u64 GetMaxMemoryForAllocation(u64 allocationSize, u64 alignment)
    {
        return r2::util::RoundUp(allocationSize, alignment);
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
}
