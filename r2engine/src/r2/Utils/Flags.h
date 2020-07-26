//
//  Flags.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-05.
//

#ifndef Flags_h
#define Flags_h

namespace r2
{
    template <class T, typename N>
    class Flags
    {
    public:
        inline Flags(void):mFlags(0)
        {
            
        }
        
        inline Flags(T flag):mFlags(flag)
        {
           
        }
        
        inline Flags(const Flags& flags) : mFlags(flags.mFlags)
        {

        }

        Flags& operator=(const Flags& flags)
        {
            if (this == &flags)
            {
                return *this;
            }

            mFlags = flags.mFlags;

            return *this;
        }

        inline void Set(T flag)
        {
            mFlags |= flag;
        }
        
        inline void Remove(T flag)
        {
            mFlags &= ~flag;
        }
        
        inline void Clear(void)
        {
            mFlags = 0;
        }
        
        inline bool IsSet(T flag) const
        {
            return ((mFlags & flag) != 0);
        }

        inline T GetRawValue() const
        {
            return mFlags;
        }
        
        inline Flags operator|(Flags other) const
        {
            return Flags(mFlags | other.mFlags);
        }
        
        inline Flags operator|(T flag) const
        {
            return Flags(mFlags | flag);
        }
        
        inline Flags& operator|=(Flags other)
        {
            mFlags |= other.mFlags;
            return *this;
        }
        
        inline bool operator==(const Flags& other) const
        {
            return mFlags == other.mFlags;
        }
    private:
//        inline explicit Flags(N flags):mFlags(flags)
//        {
//            
//        }
        N mFlags;
    };
}

#endif /* Flags_h */
