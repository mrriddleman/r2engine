//
//  Random.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-03-16.
//

#ifndef Random_h
#define Random_h

//taken from game code complete 4th ed.
#define CMATH_N 624
#define CMATH_M 397

#define CMATH_MATRIX_A 0x9908b0df   /* constant vector a */
#define CMATH_UPPER_MASK 0x80000000 /* most significant w-r bits */
#define CMATH_LOWER_MASK 0x7fffffff /* least significant r bits */

#define CMATH_TEMPERING_MASK_B 0x9d2c5680
#define CMATH_TEMPERING_MASK_C 0xefc60000
#define CMATH_TEMPERING_SHIFT_U(y)  (y >> 11)
#define CMATH_TEMPERING_SHIFT_S(y)  (y << 7)
#define CMATH_TEMPERING_SHIFT_T(y)  (y << 15)
#define CMATH_TEMPERING_SHIFT_L(y)  (y >> 18)


namespace r2
{
    namespace util
    {
        class Random
        {
            u32 rseed;
            u32 rseed_sp;
            u32 mt[CMATH_N];
            int mti;
            
        public:
            Random();
            u32 RandomNum(u32 n);
            u32 RandomNum(u32 from, u32 to);
            float Randomf();
            void SetRandomSeed(u32 n);
            u32 GetRandomSeed(void);
            void Randomize();
        };
    }
}


#endif /* Random_h */
