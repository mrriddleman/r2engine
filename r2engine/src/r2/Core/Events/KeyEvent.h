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
            
            inline s32 KeyCode() const {return mKeyCode;}
            inline u32 Modifiers() const {return mModifiers;}
            EVENT_CLASS_CATEGORY(ECAT_KEY | ECAT_INPUT)
            
        protected:
            
            KeyEvent(s32 keyCode, u32 modifiers): mKeyCode(keyCode), mModifiers(modifiers) {}
            
            s32 mKeyCode;
            u32 mModifiers;
        };
        
        class R2_API KeyPressedEvent : public KeyEvent
        {
        public:
            
            KeyPressedEvent(s32 keyCode, u32 modifiers, b32 repeated): KeyEvent(keyCode, modifiers), mRepeated(repeated) {}
            
            
            inline b32 Repeated() const {return mRepeated;}
            
            std::string ToString() const override
            {
                std::stringstream ss;
                ss << "KeyPressedEvent keycode: " << mKeyCode << " (" << mRepeated << ")";
                return ss.str();
            }
            
            EVENT_CLASS_TYPE(EVT_KEY_PRESSED)
            
        private:
            
            b32 mRepeated;
        };
        
        class R2_API KeyReleasedEvent : public KeyEvent
        {
        public:
            KeyReleasedEvent(s32 keycode, u32 modifiers): KeyEvent(keycode, modifiers) {}
            
            std::string ToString() const override
            {
                std::stringstream ss;
                ss << "KeyReleasedEvent keycode: " << mKeyCode;
                return ss.str();
            }
            
            EVENT_CLASS_TYPE(EVT_KEY_RELEASED)
        };
        
        class R2_API KeyTypedEvent: public Event
        {
        public:
            KeyTypedEvent(const char* text):mText(text) {}
            
            const char* Text() const {return mText;}
            
            std::string ToString() const override
            {
                std::stringstream ss;
                ss << "KeyTypedEvent keycode: " << mText;
                return ss.str();
            }
            
            EVENT_CLASS_CATEGORY(ECAT_KEY | ECAT_INPUT)
            EVENT_CLASS_TYPE(EVT_KEY_TYPED)
            
        private:
            const char* mText;
        };
    }
}

#endif /* KeyEvent_h */
