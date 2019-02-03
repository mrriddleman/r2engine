//
//  Application.hpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-02.
//  Copyright © 2019 Serge Lansiquot. All rights reserved.
//

#ifndef Application_hpp
#define Application_hpp

#include "r2/Platform/MacroDefines.h"

namespace r2
{
    class R2_API Application
    {
    public:
        Application()
        {
            
        }
        
        virtual ~Application()
        {
            
        }
        
        virtual void Run();
    };
}

#endif /* Application_hpp */
