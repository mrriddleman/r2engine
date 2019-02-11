//
//  KeyEvent.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-10.
//

#ifndef KeyEvent_h
#define KeyEvent_h

#include "r2/Core/Events/Event.h"
#include "r2/Platform/IO.h"

namespace r2
{
    namespace evt
    {
        class R2_API KeyEvent : public Event
        {
        public:
            
            inline u32 KeyCode() const {return mKeyCode;}
            
            EVENT_CLASS_CATEGORY(ECAT_KEY | ECAT_INPUT)
            
        protected:
            
            KeyEvent(u32 keyCode): mKeyCode(keyCode) {}
            
            u32 mKeyCode;
        };
        
        class R2_API KeyPressedEvent : public KeyEvent
        {
        public:
            
            KeyPressedEvent(u32 keyCode, u32 modifiers, b32 repeated): KeyEvent(keyCode), mModifiers(modifiers), mRepeated(repeated) {}
            
            inline u32 Modifiers() const {return mModifiers;}
            inline b32 Repeated() const {return mRepeated;}
            
            std::string ToString() const override
            {
                std::stringstream ss;
                ss << "KeyPressedEvent keycode: " << mKeyCode << " (" << mRepeated << ")";
                return ss.str();
            }
            
            EVENT_CLASS_TYPE(EVT_KEY_PRESSED)
            
        private:
            u32 mModifiers;
            b32 mRepeated;
        };
        
        class R2_API KeyReleasedEvent : public KeyEvent
        {
        public:
            KeyReleasedEvent(u32 keycode): KeyEvent(keycode) {}
            
            std::string ToString() const override
            {
                std::stringstream ss;
                ss << "KeyReleasedEvent keycode: " << mKeyCode;
                return ss.str();
            }
            
            EVENT_CLASS_TYPE(EVT_KEY_RELEASED)
        };
        
        class R2_API KeyTypedEvent: public KeyEvent
        {
        public:
            KeyTypedEvent(u32 keycode): KeyEvent(keycode) {}
            
            std::string ToString() const override
            {
                std::stringstream ss;
                ss << "KeyTypedEvent keycode: " << mKeyCode;
                return ss.str();
            }
            
            EVENT_CLASS_TYPE(EVT_KEY_TYPED)
        };
    }
}

#endif /* KeyEvent_h */
