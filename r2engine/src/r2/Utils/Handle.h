//
//  Handle.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-05-15.
//

#ifndef Handle_h
#define Handle_h

namespace r2
{
    enum class PolicyAccess: u8
    {
        READ_ACCESS = 0,
        WRITE_ACCESS
    };
    
    enum class PolicyArea: u8
    {
        FILE_AREA = 0
    };
    
    struct Policy
    {
        PolicyAccess access;
        PolicyArea area;
    };
    
    class PolicyBuilder
    {
    public:
        PolicyBuilder() = default;
        
        bool AddPolicy(const Policy& p)
        {
            if(mCount +1 < MAX_POLICIES)
            {
                mPolicies[mCount++] = p;
                return true;
            }
            
            return false;
        }
        
        bool AddPolicy(PolicyArea area, PolicyAccess access)
        {
            Policy p {access, area};
            return AddPolicy(p);
        }
        
        //@TODO(Serge): figure out a way to compare Policies
        
    private:
        static const u8 MAX_POLICIES = 256;
        Policy mPolicies[MAX_POLICIES];
        u8 mCount = 0;
    };
        
    const char* PolicyStr(PolicyAccess p)
    {
        if (p == PolicyAccess::READ_ACCESS)
        {
            static const char* readAccess = "Read Access";
            return readAccess;
        }
        else if(p == PolicyAccess::WRITE_ACCESS)
        {
            static const char* writeAccess = "Write Access";
            return writeAccess;
        }
        
        return "Unknown Access";
    }
    
    template<typename T, PolicyAccess>
    class R2_API Handle
    {
    public:
        
        Handle(T* data): mData(data){}
        
        const T* GetConstPtr() const
        {
            R2_CHECK(mPolicy <= PolicyAccess::WRITE_ACCESS, "You're trying to get read access on %p where only have %s permission", mData, PolicyStr(mPolicy));
            return mData;
        }
        
        T* GetPtr() const
        {
            R2_CHECK(mPolicy == PolicyAccess::WRITE_ACCESS, "You're trying to get write access on %p where only have %s permission", mData, PolicyStr(mPolicy));
            
            return mData;
        }
        
        const T& GetContRef() const
        {
            R2_CHECK(mPolicy <= PolicyAccess::WRITE_ACCESS && mData != nullptr, "You're trying to get read access on %p where only have %s permission", mData, PolicyStr(mPolicy));
            
            return *mData;
        }
        
        T& GetRef() const
        {
            R2_CHECK(mPolicy == PolicyAccess::WRITE_ACCESS && mData != nullptr, "You're trying to get write access on %p where only have %s permission", mData, PolicyStr(mPolicy));
            
            return *mData;
        }
        
        PolicyAccess Policy() const {return mPolicy;}
    private:
        T* mData;
        PolicyAccess mPolicy;
    };
}

#endif /* Handle_h */
