//
//  SoundLayer.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-05-21.
//

#ifndef SoundLayer_h
#define SoundLayer_h

#include "r2/Core/Layer/Layer.h"

namespace r2
{
    class SoundLayer: public Layer
    {
    public:
        SoundLayer();
    
        virtual void Init() override;
        virtual void Shutdown() override;
        virtual void Update() override;
    };
}

#endif /* SoundLayer_h */
