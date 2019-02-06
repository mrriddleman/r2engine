//
//  Platform.hpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-03.
//

#ifndef Platform_hpp
#define Platform_hpp

//TODO(Serge): Remove
#include <memory>
#include <string>

#include "r2/Core/Application.h"

namespace r2
{
    class R2_API Platform
    {
    public:
        Platform(){}
        virtual ~Platform(){}
        
        static const r2::Platform& GetConst();
        static r2::Platform& Get();
        
        Platform(const Platform&) = delete;
        Platform& operator=(const Platform&) = delete;
        Platform(Platform && ) = delete;
        Platform& operator=(Platform &&) = delete;
        
        virtual bool Init(std::unique_ptr<r2::Application> app)
        {
            mApplication = std::move(app);
            return true;
        }
        
        virtual void Run() = 0;
        virtual void Shutdown() = 0;
        
        virtual const std::string& GetBasePath() const = 0;
        virtual const u32 TickRate() const = 0;

    protected:
        std::unique_ptr<r2::Application> mApplication;
    };
}


#endif /* Platform_hpp */
