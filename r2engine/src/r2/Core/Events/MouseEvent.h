//
//  MouseEvent.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-10.
//

#ifndef MouseEvent_h
#define MouseEvent_h

#include "r2/Core/Events/Event.h"
#include "r2/Platform/IO.h"
#include <sstream>
namespace r2
{
    namespace evt
    {
        class R2_API MouseMovedEvent : public Event
        {
        public:
            MouseMovedEvent(u32 x, u32 y): mX(x), mY(y) {}
            
            inline u32 X() const {return mX;}
            inline u32 Y() const {return mY;}
            
            std::string ToString() const override
            {
                std::stringstream ss;
                ss << "MouseMovedEvent To: " << mX << ", " << mY;
                return ss.str();
            }
            
            EVENT_CLASS_TYPE(EVT_MOUSE_MOVED)
            EVENT_CLASS_CATEGORY(ECAT_MOUSE | ECAT_INPUT)
            
        private:
            u32 mX, mY;
        };
        
        class R2_API MouseButtonEvent : public Event
        {
        public:
            inline u8 MouseButton() const {return mButton;}
            inline s32 XPos() const {return mX;}
            inline s32 YPos() const {return mY;}
            EVENT_CLASS_CATEGORY(ECAT_MOUSE | ECAT_MOUSE_BUTTON | ECAT_INPUT)
        protected:
            MouseButtonEvent(u8 button, s32 x, s32 y): mButton(button), mX(x), mY(y) {}
            std::string ButtonName() const
            {
                if(mButton == r2::io::MOUSE_BUTTON_LEFT)
                {
                    return "Left Click";
                }
                else if(mButton == r2::io::MOUSE_BUTTON_RIGHT)
                {
                    return "Right Click";
                }
                else if(mButton == r2::io::MOUSE_BUTTON_MIDDLE)
                {
                    return "Middle Click";
                }
                
                return "";
            }
            
            u8 mButton;
            s32 mX, mY;
        };
        
        class R2_API MouseButtonPressedEvent : public MouseButtonEvent
        {
        public:
            
            MouseButtonPressedEvent(u8 button, s32 x, s32 y, u8 numClicks): MouseButtonEvent(button, x, y), mNumClicks(numClicks) {}
            
            inline u8 NumClicks() const {return mNumClicks;}
            
            std::string ToString() const override
            {
                std::stringstream ss;
                
                ss << "MouseButtonPressedEvent: " << ButtonName() << " Pressed - " << mNumClicks << " times";
                
                return ss.str();
            }
            
            EVENT_CLASS_TYPE(EVT_MOUSE_BUTTON_PRESSED)
            
        private:
            u8 mNumClicks;
        };
        
        class R2_API MouseButtonReleasedEvent : public MouseButtonEvent
        {
        public:
            MouseButtonReleasedEvent(u8 button, s32 x, s32 y) : MouseButtonEvent(button, x, y) {}
            
            std::string ToString() const override
            {
                std::stringstream ss;
                
                ss << "MouseButtonReleasedEvent: " << ButtonName() << " Released";
                
                return ss.str();
            }
            
            EVENT_CLASS_TYPE(EVT_MOUSE_BUTTON_RELEASED)
        };
        
        class R2_API MouseWheelEvent : public Event
        {
        public:
            
            MouseWheelEvent(s32 xDelta, s32 yDelta, u32 direction): mXAmount(xDelta), mYAmount(yDelta), mDirection(direction) {}
            inline s32 XAmount() const {return mXAmount;}
            inline s32 YAmount() const {return mYAmount;}
            inline u32 Direction() const {return mDirection;}
            std::string ToString() const override
            {
                std::stringstream ss;
                
                ss << "MouseWheelEvent - x amount: " << mXAmount << ", y amount: " << mYAmount << ", direction: " << DirectionString();
                
                return ss.str();
            }
            
            EVENT_CLASS_TYPE(EVT_MOUSE_WHEEL)
            EVENT_CLASS_CATEGORY(ECAT_MOUSE | ECAT_INPUT)
            
        private:
            s32 mXAmount, mYAmount;
            u32 mDirection;
            
            std::string DirectionString() const
            {
                if(mDirection == io::MOUSE_DIRECTION_NORMAL)
                {
                    return "Normal mouse direction";
                }
                else
                {
                    return "Flipped mouse direction";
                }
            }
        };
    }
}

#endif /* MouseEvent_h */
