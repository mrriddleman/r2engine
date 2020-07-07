//
//  AppEvent.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-10.
//

#ifndef AppEvent_h
#define AppEvent_h

#include "r2/Core/Events/Event.h"
#include <sstream>

namespace r2
{
    namespace evt
    {
        class R2_API WindowResizeEvent: public Event
        {
        public:
            WindowResizeEvent(u32 width, u32 height): mWidth(width), mHeight(height){}
            
            inline u32 Width() const {return mWidth;}
            inline u32 Height() const {return mHeight;}
            
            std::string ToString() const override
            {
                std::stringstream ss;
                
                ss << "WindowResizeEvent to: " << mWidth << ", " << mHeight;
                return ss.str();
            }
            
            EVENT_CLASS_TYPE(EVT_WINDOW_RESIZED)
            EVENT_CLASS_CATEGORY(ECAT_APP)
        private:
            u32 mWidth, mHeight;
        };
        
        class R2_API WindowCloseEvent: public Event
        {
        public:
            WindowCloseEvent() {}
            
            EVENT_CLASS_TYPE(EVT_WINDOW_CLOSE)
            EVENT_CLASS_CATEGORY(ECAT_APP)
            
            virtual std::string ToString() const override
            {
                return "WindowCloseEvent";
            }
        };
        
        class R2_API WindowMinimizedEvent: public Event
        {
        public:
            WindowMinimizedEvent(){}
            
            EVENT_CLASS_TYPE(EVT_WINDOW_MINIMIZED)
            EVENT_CLASS_CATEGORY(ECAT_APP)
            virtual std::string ToString() const override
            {
                return "WindowMinimizedEvent";
            }
        };
        
        class R2_API WindowUnMinimizedEvent: public Event
        {
        public:
            WindowUnMinimizedEvent(){}
            
            EVENT_CLASS_TYPE(EVT_WINDOW_EXPOSED)
            EVENT_CLASS_CATEGORY(ECAT_APP)
            virtual std::string ToString() const override
            {
                return "WindowUnMinimizedEvent";
            }
        };
    }
}


#endif /* AppEvent_h */
