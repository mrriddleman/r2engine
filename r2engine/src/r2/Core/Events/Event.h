//
//  Event.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-10.
//

#ifndef Event_h
#define Event_h

namespace r2
{
    namespace evt
    {
        enum EventType
        {
            EVT_NONE = 0,
            EVT_WINDOW_CLOSE, EVT_WINDOW_RESIZED, EVT_WINDOW_SIZE_CHANGED, EVT_WINDOW_MINIMIZED, EVT_WINDOW_EXPOSED,
            EVT_KEY_PRESSED, EVT_KEY_RELEASED, EVT_KEY_TYPED,
            EVT_MOUSE_BUTTON_PRESSED, EVT_MOUSE_BUTTON_RELEASED, EVT_MOUSE_MOVED, EVT_MOUSE_WHEEL,
            EVT_CONTROLLER_CONNECTED, EVT_CONTROLLER_DISCONNECTED, EVT_CONTROLLER_AXIS, EVT_CONTROLLER_BUTTON, EVT_CONTROLLER_DETECTED, EVT_CONTROLLER_REMAPPED,
            EVT_RESOLUTION_CHANGED,
            EVT_RENDERER_BACKEND_CHANGED,
#ifdef R2_EDITOR
            EVT_EDITOR_CREATED_NEW_ENTITY,
            EVT_EDITOR_DESTROYED_ENTITY,
            EVT_EDITOR_DESTROYED_ENTITY_TREE,
#endif
        };
        
        enum EventCategory
        {
            ECAT_NONE         = 0,
            ECAT_APP          = BIT(0),
            ECAT_INPUT        = BIT(1),
            ECAT_KEY          = BIT(2),
            ECAT_MOUSE        = BIT(3),
            ECAT_MOUSE_BUTTON = BIT(4),
            ECAT_CONTROLLER   = BIT(5),
#ifdef R2_EDITOR
            ECAT_EDITOR       = BIT(6),
#endif
        };
        
#define EVENT_CLASS_TYPE(type) static EventType GetStaticType() { return type; }\
virtual EventType GetEventType() const override { return GetStaticType(); }\
virtual const char* GetName() const override { return #type; }
        
#define EVENT_CLASS_CATEGORY(category) virtual int GetCategoryFlags() const override { return category; }
        
        class R2_API Event
        {
        public:
            bool handled = false;
            
            virtual EventType GetEventType() const = 0;
            virtual const char * GetName() const = 0;
            virtual int GetCategoryFlags() const = 0;
            virtual std::string ToString() const { return GetName(); }
            virtual bool OverrideEventHandled() const { return false; }

            inline bool IsInCategory(EventCategory category)
            {
                return GetCategoryFlags() & category;
            }
        };
        
        class EventDispatcher
        {
            template<typename T>
            using EventFn = std::function<bool(const T&)>;
        public:
            EventDispatcher(Event& event)
            : m_Event(event)
            {
            }
            
            template<typename T>
            bool Dispatch(EventFn<T> func)
            {
                if (m_Event.GetEventType() == T::GetStaticType())
                {
                    m_Event.handled = func(*(T*)&m_Event);

                    return true;
                }
                return false;
            }
        private:
            Event& m_Event;
        };
        
    }
}


#endif /* Event_h */
