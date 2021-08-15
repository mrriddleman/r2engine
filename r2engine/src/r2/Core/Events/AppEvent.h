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
            WindowResizeEvent(u32 originalWidth, u32 originalHeight, u32 newWidth, u32 newHeight)
                : mOriginalWidth(originalWidth)
                , mOriginalHeight(originalHeight)
                , mNewWidth(newWidth)
                , mNewHeight(newHeight)
            {}
            
            inline u32 OriginalWidth() const {return mOriginalWidth;}
            inline u32 OriginalHeight() const {return mOriginalHeight;}
            inline u32 NewWidth() const { return mNewWidth; }
            inline u32 NewHeight() const { return mNewHeight; }

            bool OverrideEventHandled() const { return true; }

            std::string ToString() const override
            {
                std::stringstream ss;
                
                ss << "WindowResizeEvent from: " << mOriginalWidth << ", " << mOriginalHeight <<  " to: " << mNewWidth << ", " << mNewHeight;
                return ss.str();
            }
            
            EVENT_CLASS_TYPE(EVT_WINDOW_RESIZED)
            EVENT_CLASS_CATEGORY(ECAT_APP)
        private:
            u32 mOriginalWidth, mOriginalHeight;
            u32 mNewWidth, mNewHeight;
        };

        class ResolutionChangedEvent : public Event 
        {
        public:

            ResolutionChangedEvent(u32 resX, u32 resY)
                : mResolutionX(resX)
                , mResolutionY(resY)
            {}

            u32 ResolutionX() const { return mResolutionX; }
            u32 ResolutionY() const { return mResolutionY; }

			std::string ToString() const override
			{
				std::stringstream ss;

                ss << "Resolution Changed to: " << mResolutionX << ", " << mResolutionY;
				return ss.str();
			}

            EVENT_CLASS_TYPE(EVT_RESOLUTION_CHANGED)
            EVENT_CLASS_CATEGORY(ECAT_APP)

        private:
            u32 mResolutionX, mResolutionY;
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
