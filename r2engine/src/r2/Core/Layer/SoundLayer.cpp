//
//  SoundLayer.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-05-21.
//

#include "SoundLayer.h"
#include "r2/Audio/AudioEngine.h"

namespace r2
{
    SoundLayer::SoundLayer(): Layer("Sound Layer", false){}
    
    void SoundLayer::Init()
    {
        r2::audio::AudioEngine::Init();
    }
    
    void SoundLayer::Shutdown()
    {
        r2::audio::AudioEngine::Shutdown();
    }
    
    void SoundLayer::Update()
    {
        r2::audio::AudioEngine::Update();
    }
}