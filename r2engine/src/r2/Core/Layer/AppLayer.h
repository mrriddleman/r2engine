//
//  AppLayer.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-18.
//

#ifndef AppLayer_h
#define AppLayer_h

#include "r2/Core/Layer/Layer.h"
#include "r2/Core/Application.h"

namespace r2
{
    class R2_API AppLayer: public Layer
    {
    public:
        AppLayer(std::unique_ptr<Application> app);
        virtual ~AppLayer(){}
        
        virtual void Init() override;
        virtual void Shutdown() override;
        virtual void Update() override;
        virtual void Render(float alpha) override;
        virtual void OnEvent(evt::Event& event) override;

        const Application& GetApplication() const;
    private:
        std::unique_ptr<Application> mApp;
    };
}


#endif /* AppLayer_h */
