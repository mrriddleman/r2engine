//
//  Layer.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-12.
//

#include "r2/Core/Layer/Layer.h"

namespace r2
{
    Layer::Layer(const std::string& name, b32 renderable):
    mDebugName(name),
    mDisabled(false),
    mRenderable(renderable)
    {
        
    }
}
