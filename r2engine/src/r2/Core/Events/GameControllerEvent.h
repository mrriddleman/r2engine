//
//  GameControllerEvent.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-10-06.
//

#ifndef GameControllerEvent_h
#define GameControllerEvent_h

#include "r2/Core/Events/Event.h"
#include "r2/Platform/IO.h"

namespace r2::evt
{
    class R2_API GameControllerEvent: public Event
    {
    public:
        r2::io::ControllerID GetControllerID() const {return mControllerID;}
        EVENT_CLASS_CATEGORY(ECAT_CONTROLLER | ECAT_INPUT);
    protected:
        
        GameControllerEvent(r2::io::ControllerID theControllerID):mControllerID(theControllerID){}
        
        r2::io::ControllerID mControllerID;
    };
    
    class R2_API GameControllerDisconnectedEvent: public GameControllerEvent
    {
    public:
        
        GameControllerDisconnectedEvent(r2::io::ControllerID controllerID):GameControllerEvent(controllerID){}
        
        virtual std::string ToString() const override
        {
            std::stringstream ss;
            ss << "GameControllerDisconnectedEvent controllerId: " << GetControllerID();
            return ss.str();
        }
        EVENT_CLASS_TYPE(EVT_CONTROLLER_DISCONNECTED);
    };
    
    class R2_API GameControllerConnectedEvent: public GameControllerEvent
    {
    public:
        
        GameControllerConnectedEvent(r2::io::ControllerID controllerID, r2::io::ControllerInstanceID instanceID):GameControllerEvent(controllerID), mInstanceID(instanceID){}
        
        r2::io::ControllerInstanceID GetInstanceID() const { return mInstanceID; }
        virtual std::string ToString() const override
        {
            std::stringstream ss;
            ss << "GameControllerConnectedEvent controllerId: " << GetControllerID();
            return ss.str();
        }
        EVENT_CLASS_TYPE(EVT_CONTROLLER_CONNECTED);

    protected:
        r2::io::ControllerInstanceID mInstanceID;
    };
    
    class R2_API GameControllerAxisEvent: public GameControllerEvent
    {
    public:
        
        GameControllerAxisEvent(r2::io::ControllerID controllerID, const r2::io::ControllerAxis& axisState): GameControllerEvent(controllerID), mAxisState(axisState){}
        
        virtual std::string ToString() const override
        {
            std::stringstream ss;
            ss << "GameControllerAxisEvent controllerId: " << GetControllerID() << " axis: " << AxisName() <<", value: " << AxisValue();
            return ss.str();
        }
        
        const r2::io::ControllerAxisName AxisName() const {return mAxisState.axis;}
        s16 AxisValue() const {return mAxisState.value;}
        EVENT_CLASS_TYPE(EVT_CONTROLLER_AXIS);
    private:
        r2::io::ControllerAxis mAxisState;
    };
    
    class R2_API GameControllerButtonEvent: public GameControllerEvent
    {
    public:
        
        GameControllerButtonEvent(r2::io::ControllerID controllerID, const r2::io::ControllerButton& buttonState)
        : GameControllerEvent(controllerID)
        , mButtonState(buttonState)
        {
            
        }
        
        const r2::io::ControllerButtonName ButtonName() const {return mButtonState.buttonName;}
        
        u8 ButtonState() const {return mButtonState.state;}
        
        virtual std::string ToString() const override
        {
            std::stringstream ss;
            std::string state = ButtonState() > 0 ? "Pressed" : "Released";
            ss << "GameControllerButtonEvent controllerId: " << GetControllerID() << " button name: " << ButtonName() <<", state: " << state;
            return ss.str();
        }
        
        EVENT_CLASS_TYPE(EVT_CONTROLLER_BUTTON);
        
    private:
        r2::io::ControllerButton mButtonState;
    };
    
    class R2_API GameControllerDetectedEvent: public GameControllerEvent
    {
    public:
        GameControllerDetectedEvent(r2::io::ControllerID controllerID)
        : GameControllerEvent(controllerID){}
        
        virtual std::string ToString() const override
        {
            std::stringstream ss;
            ss << "GameControllerDetectedEvent controllerId: " << GetControllerID();
            return ss.str();
        }
        
        EVENT_CLASS_TYPE(EVT_CONTROLLER_DETECTED)
    };
    
    class R2_API GameControllerRemappedEvent: public GameControllerEvent
    {
    public:
        GameControllerRemappedEvent(r2::io::ControllerID controllerID)
        : GameControllerEvent(controllerID)
        {
        }
        
        virtual std::string ToString() const override
        {
            std::stringstream ss;
            ss << "GameControllerRemappedEvent controllerId: " << GetControllerID();
            return ss.str();
        }
        
        EVENT_CLASS_TYPE(EVT_CONTROLLER_REMAPPED)
    };
    
}

#endif /* GameControllerEvent_h */
