//
//  Memory.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-23.
//

#include "Memory.h"
#include <cstring>

namespace r2
{
    namespace mem
    {
        Allocator::Allocator(const char* name)
        {
            memset(mName, '\0', Kilobytes(1));
            strcpy(mName, name);
        }
    }
    
}
/* @TODO(Serge): enable one day
void * operator new(decltype(sizeof(0)) n) noexcept(false)
{
    R2_CHECK(false, "Don't use new!");
    return nullptr;
}

void operator delete(void * p) throw()
{
    R2_CHECK(false, "Don't use delete!");
}

void *operator new[](std::size_t s) noexcept(false)
{
    R2_CHECK(false, "Don't use new[]!");
    return nullptr;
}
void operator delete[](void *p) throw()
{
    R2_CHECK(false, "Don't use delete[]");
}
*/
