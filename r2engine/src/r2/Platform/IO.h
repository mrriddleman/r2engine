//
//  PlatformInput.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-09.
//

#ifndef PlatformInput_h
#define PlatformInput_h

namespace r2::io
{
    extern u8 MOUSE_BUTTON_LEFT;
    extern u8 MOUSE_BUTTON_RIGHT;
    extern u8 MOUSE_BUTTON_MIDDLE;
    
    extern u8 BUTTON_PRESSED;
    extern u8 BUTTON_RELEASED;
    
    extern u32 MOUSE_DIRECTION_NORMAL;
    extern u32 MOUSE_DIRECTION_FLIPPED;

    extern u32 SHIFT_PRESSED_BTN;
    extern u32 CTRL_PRESSED;
    extern u32 ALT_PRESSED;
    
    extern s32 KEY_RETURN;
    extern s32 KEY_ESCAPE;
    extern s32 KEY_BACKSPACE;
    extern s32 KEY_TAB;
    extern s32 KEY_SPACE;
    extern s32 KEY_EXCLAIM;
    extern s32 KEY_QUOTEDBL;
    extern s32 KEY_HASH;
    extern s32 KEY_PERCENT;
    extern s32 KEY_DOLLAR;
    extern s32 KEY_AMPERSAND;
    extern s32 KEY_QUOTE;
    extern s32 KEY_LEFTPAREN;
    extern s32 KEY_RIGHTPAREN;
    extern s32 KEY_ASTERISK;
    extern s32 KEY_PLUS;
    extern s32 KEY_COMMA;
    extern s32 KEY_MINUS;
    extern s32 KEY_PERIOD;
    extern s32 KEY_SLASH;
    extern s32 KEY_0;
    extern s32 KEY_1;
    extern s32 KEY_2;
    extern s32 KEY_3;
    extern s32 KEY_4;
    extern s32 KEY_5;
    extern s32 KEY_6;
    extern s32 KEY_7;
    extern s32 KEY_8;
    extern s32 KEY_9;
    extern s32 KEY_COLON;
    extern s32 KEY_SEMICOLON;
    extern s32 KEY_LESS;
    extern s32 KEY_EQUALS;
    extern s32 KEY_GREATER;
    extern s32 KEY_QUESTION;
    extern s32 KEY_AT;
    extern s32 KEY_LEFTBRACKET;
    extern s32 KEY_BACKSLASH;
    extern s32 KEY_RIGHTBRACKET;
    extern s32 KEY_CARET;
    extern s32 KEY_UNDERSCORE;
    extern s32 KEY_BACKQUOTE;
    extern s32 KEY_a;
    extern s32 KEY_b;
    extern s32 KEY_c;
    extern s32 KEY_d;
    extern s32 KEY_e;
    extern s32 KEY_f;
    extern s32 KEY_g;
    extern s32 KEY_h;
    extern s32 KEY_i;
    extern s32 KEY_j;
    extern s32 KEY_k;
    extern s32 KEY_l;
    extern s32 KEY_m;
    extern s32 KEY_n;
    extern s32 KEY_o;
    extern s32 KEY_p;
    extern s32 KEY_q;
    extern s32 KEY_r;
    extern s32 KEY_s;
    extern s32 KEY_t;
    extern s32 KEY_u;
    extern s32 KEY_v;
    extern s32 KEY_w;
    extern s32 KEY_x;
    extern s32 KEY_y;
    extern s32 KEY_z;
    extern s32 KEY_CAPSLOCK;
    extern s32 KEY_F1;
    extern s32 KEY_F2;
    extern s32 KEY_F3;
    extern s32 KEY_F4;
    extern s32 KEY_F5;
    extern s32 KEY_F6;
    extern s32 KEY_F7;
    extern s32 KEY_F8;
    extern s32 KEY_F9;
    extern s32 KEY_F10;
    extern s32 KEY_F11;
    extern s32 KEY_F12;
    extern s32 KEY_PRINTSCREEN;
    extern s32 KEY_SCROLLLOCK;
    extern s32 KEY_PAUSE;
    extern s32 KEY_INSERT;
    extern s32 KEY_HOME;
    extern s32 KEY_PAGEUP;
    extern s32 KEY_DELETE;
    extern s32 KEY_END;
    extern s32 KEY_PAGEDOWN;
    extern s32 KEY_RIGHT;
    extern s32 KEY_LEFT;
    extern s32 KEY_DOWN;
    extern s32 KEY_UP;
    
    enum ControllerButtonName
    {
        CONTROLLER_BUTTON_INVALID = -1,
        CONTROLLER_BUTTON_A,
        CONTROLLER_BUTTON_B,
        CONTROLLER_BUTTON_X,
        CONTROLLER_BUTTON_Y,
        CONTROLLER_BUTTON_BACK,
        CONTROLLER_BUTTON_GUIDE,
        CONTROLLER_BUTTON_START,
        CONTROLLER_BUTTON_LEFTSTICK,
        CONTROLLER_BUTTON_RIGHTSTICK,
        CONTROLLER_BUTTON_LEFTSHOULDER,
        CONTROLLER_BUTTON_RIGHTSHOULDER,
        CONTROLLER_BUTTON_DPAD_UP,
        CONTROLLER_BUTTON_DPAD_DOWN,
        CONTROLLER_BUTTON_DPAD_LEFT,
        CONTROLLER_BUTTON_DPAD_RIGHT,
        CONTROLLER_BUTTON_MAX
    };
    
    struct ControllerButton
    {
        const ControllerButtonName buttonName;
        u8 state = BUTTON_RELEASED;
        b32 held = false;

        bool IsPressed() const {
            return state == BUTTON_RELEASED;
        }
    };
    
    enum ControllerAxisName
    {
        CONTROLLER_AXIS_INVALID = -1,
        CONTROLLER_AXIS_LEFTX,
        CONTROLLER_AXIS_LEFTY,
        CONTROLLER_AXIS_RIGHTX,
        CONTROLLER_AXIS_RIGHTY,
        CONTROLLER_AXIS_TRIGGERLEFT,
        CONTROLLER_AXIS_TRIGGERRIGHT,
        CONTROLLER_AXIS_MAX
    };
    
    struct ControllerAxis
    {
        const ControllerAxisName axis;
        s16 value = 0;
    };
    
    struct MouseData
    {
        u8 numClicks = 0;
        u8 state  = 0;
        u8 button = 0;
        s32 x = 0, y = 0;
        s32 xrel = 0, yrel = 0;
        u32 direction = MOUSE_DIRECTION_NORMAL;
    };
    

    struct MouseButtonState
    {
		u8 state = 0;
		u8 button = 0;
        u8 numClicks = 0;
    };

    struct MouseWheelState
    {
		s32 x = 0;
		s32 y = 0;
        u32 direction = MOUSE_DIRECTION_NORMAL;
    };

    struct MousePositionState
    {
        s32 x = 0;
        s32 y = 0;
    };

    struct MouseState
    {
        MouseButtonState leftMouseButton{MOUSE_BUTTON_LEFT, BUTTON_RELEASED};
        MouseButtonState middleMouseButton{MOUSE_BUTTON_MIDDLE, BUTTON_RELEASED };
        MouseButtonState rightMouseButton{ MOUSE_BUTTON_RIGHT, BUTTON_RELEASED};

        MouseWheelState mouseWheelState;
        MousePositionState mousePositionState;
    };

    struct Key
    {
        enum
        {
            SHIFT_PRESSED_KEY = 1 << 0,
            CONTROL_PRESSED = 1 << 1,
            ALT_PRESSED = 1 << 2
        };
        
        s32 code = 0;
        u32 modifiers = 0;
        b32 repeated = 0;
        u8 state = 0;
    };

    struct KeyState
    {
        s32 code = 0;
        b32 repeated = 0;
        u8 state = 0;


        bool IsPressed() const {
            return state == BUTTON_PRESSED;
        }


    };

    struct KeyboardState
    {
        //ugh...
        u32 modifiers = 0;

        KeyState key_return{KEY_RETURN, 0, BUTTON_RELEASED};
		KeyState key_escape{ KEY_ESCAPE, 0, BUTTON_RELEASED };
		KeyState key_backspace{ KEY_BACKSPACE, 0, BUTTON_RELEASED };
		KeyState key_tab{ KEY_TAB, 0, BUTTON_RELEASED };
		KeyState key_space{ KEY_SPACE, 0, BUTTON_RELEASED };
		KeyState key_exclaim{ KEY_EXCLAIM, 0, BUTTON_RELEASED };
		KeyState key_quotedbl{ KEY_QUOTEDBL, 0, BUTTON_RELEASED };
		KeyState key_hash{ KEY_HASH, 0, BUTTON_RELEASED };
		KeyState key_percent{ KEY_PERCENT, 0, BUTTON_RELEASED };
		KeyState key_dollar{ KEY_DOLLAR, 0, BUTTON_RELEASED };
		KeyState key_ampersand{ KEY_AMPERSAND, 0, BUTTON_RELEASED };
		KeyState key_quote{ KEY_QUOTE, 0, BUTTON_RELEASED };
		KeyState key_leftparen{ KEY_LEFTPAREN, 0, BUTTON_RELEASED };
		KeyState key_rightparen{ KEY_RIGHTPAREN, 0, BUTTON_RELEASED };
		KeyState key_asterisk{ KEY_ASTERISK, 0, BUTTON_RELEASED };
		KeyState key_plus{ KEY_PLUS, 0, BUTTON_RELEASED };
		KeyState key_comma{ KEY_COMMA, 0, BUTTON_RELEASED };
		KeyState key_minus{ KEY_MINUS, 0, BUTTON_RELEASED };
		KeyState key_period{ KEY_PERIOD, 0, BUTTON_RELEASED };
		KeyState key_slash{ KEY_SLASH, 0, BUTTON_RELEASED };
		KeyState key_0{ KEY_0, 0, BUTTON_RELEASED };
		KeyState key_1{ KEY_1, 0, BUTTON_RELEASED };
		KeyState key_2{ KEY_2, 0, BUTTON_RELEASED };
		KeyState key_3{ KEY_3, 0, BUTTON_RELEASED };
		KeyState key_4{ KEY_4, 0, BUTTON_RELEASED };
		KeyState key_5{ KEY_5, 0, BUTTON_RELEASED };
		KeyState key_6{ KEY_6, 0, BUTTON_RELEASED };
		KeyState key_7{ KEY_7, 0, BUTTON_RELEASED };
		KeyState key_8{ KEY_8, 0, BUTTON_RELEASED };
		KeyState key_9{ KEY_9, 0, BUTTON_RELEASED };
		KeyState key_colon{ KEY_COLON, 0, BUTTON_RELEASED };
		KeyState key_semicolon{ KEY_SEMICOLON, 0, BUTTON_RELEASED };
		KeyState key_less{ KEY_LESS, 0, BUTTON_RELEASED };
		KeyState key_equals{ KEY_EQUALS, 0, BUTTON_RELEASED };
		KeyState key_greater{ KEY_GREATER, 0, BUTTON_RELEASED };
		KeyState key_question{ KEY_QUESTION, 0, BUTTON_RELEASED };
		KeyState key_at{ KEY_AT, 0, BUTTON_RELEASED };
		KeyState key_leftbracket{ KEY_LEFTBRACKET, 0, BUTTON_RELEASED };
		KeyState key_backslash{ KEY_BACKSLASH, 0, BUTTON_RELEASED };
		KeyState key_rightbracket{ KEY_RETURN, 0, BUTTON_RELEASED };
		KeyState key_caret{ KEY_CARET, 0, BUTTON_RELEASED };
		KeyState key_underscore{ KEY_UNDERSCORE, 0, BUTTON_RELEASED };
		KeyState key_backquote{ KEY_BACKQUOTE, 0, BUTTON_RELEASED };
		KeyState key_a{ KEY_a, 0, BUTTON_RELEASED };
		KeyState key_b{ KEY_b, 0, BUTTON_RELEASED };
		KeyState key_c{ KEY_c, 0, BUTTON_RELEASED };
		KeyState key_d{ KEY_d, 0, BUTTON_RELEASED };
		KeyState key_e{ KEY_e, 0, BUTTON_RELEASED };
		KeyState key_f{ KEY_f, 0, BUTTON_RELEASED };
		KeyState key_g{ KEY_g, 0, BUTTON_RELEASED };
		KeyState key_h{ KEY_h, 0, BUTTON_RELEASED };
		KeyState key_i{ KEY_i, 0, BUTTON_RELEASED };
		KeyState key_j{ KEY_j, 0, BUTTON_RELEASED };
		KeyState key_k{ KEY_k, 0, BUTTON_RELEASED };
		KeyState key_l{ KEY_l, 0, BUTTON_RELEASED };
		KeyState key_m{ KEY_m, 0, BUTTON_RELEASED };
		KeyState key_n{ KEY_n, 0, BUTTON_RELEASED };
		KeyState key_o{ KEY_o, 0, BUTTON_RELEASED };
		KeyState key_p{ KEY_p, 0, BUTTON_RELEASED };
		KeyState key_q{ KEY_q, 0, BUTTON_RELEASED };
		KeyState key_r{ KEY_r, 0, BUTTON_RELEASED };
		KeyState key_s{ KEY_s, 0, BUTTON_RELEASED };
		KeyState key_t{ KEY_t, 0, BUTTON_RELEASED };
		KeyState key_u{ KEY_u, 0, BUTTON_RELEASED };
		KeyState key_v{ KEY_v, 0, BUTTON_RELEASED };
		KeyState key_w{ KEY_w, 0, BUTTON_RELEASED };
		KeyState key_x{ KEY_x, 0, BUTTON_RELEASED };
		KeyState key_y{ KEY_y, 0, BUTTON_RELEASED };
		KeyState key_z{ KEY_z, 0, BUTTON_RELEASED };
		KeyState key_capslock{ KEY_CAPSLOCK, 0, BUTTON_RELEASED };
		KeyState key_f1{ KEY_F1, 0, BUTTON_RELEASED };
		KeyState key_f2{ KEY_F2, 0, BUTTON_RELEASED };
		KeyState key_f3{ KEY_F3, 0, BUTTON_RELEASED };
		KeyState key_f4{ KEY_F4, 0, BUTTON_RELEASED };
		KeyState key_f5{ KEY_F5, 0, BUTTON_RELEASED };
		KeyState key_f6{ KEY_F6, 0, BUTTON_RELEASED };
		KeyState key_f7{ KEY_F7, 0, BUTTON_RELEASED };
		KeyState key_f8{ KEY_F8, 0, BUTTON_RELEASED };
		KeyState key_f9{ KEY_F9, 0, BUTTON_RELEASED };
		KeyState key_f10{ KEY_F10, 0, BUTTON_RELEASED };
		KeyState key_f11{ KEY_F11, 0, BUTTON_RELEASED };
		KeyState key_f12{ KEY_F12, 0, BUTTON_RELEASED };
		KeyState key_printscreen{ KEY_PRINTSCREEN, 0, BUTTON_RELEASED };
		KeyState key_scrolllock{ KEY_SCROLLLOCK, 0, BUTTON_RELEASED };
		KeyState key_pause{ KEY_PAUSE, 0, BUTTON_RELEASED };
		KeyState key_insert{ KEY_INSERT, 0, BUTTON_RELEASED };
		KeyState key_home{ KEY_HOME, 0, BUTTON_RELEASED };
		KeyState key_pageup{ KEY_PAGEUP, 0, BUTTON_RELEASED };
		KeyState key_delete{ KEY_DELETE, 0, BUTTON_RELEASED };
		KeyState key_end{ KEY_END, 0, BUTTON_RELEASED };
		KeyState key_pagedown{ KEY_PAGEDOWN, 0, BUTTON_RELEASED };
		KeyState key_right{ KEY_RIGHT, 0, BUTTON_RELEASED };
		KeyState key_left{ KEY_LEFT, 0, BUTTON_RELEASED };
		KeyState key_down{ KEY_DOWN, 0, BUTTON_RELEASED };
		KeyState key_up{ KEY_UP, 0, BUTTON_RELEASED };
    };
    
    using ControllerID = s32;
    using ControllerInstanceID = s32;
    static const ControllerID INVALID_CONTROLLER_ID = -1;
    
    enum ControllerType
    {
		CONTROLLER_TYPE_UNKNOWN = 0,
		CONTROLLER_TYPE_XBOX360,
		CONTROLLER_TYPE_XBOXONE,
		CONTROLLER_TYPE_PS3,
		CONTROLLER_TYPE_PS4,
		CONTROLLER_TYPE_NINTENDO_SWITCH_PRO
    };

    const char* const controllerTypeStrings[]
    {
        "UNKNOWN",
        "XBOX360",
        "XBOX_ONE",
        "PS3",
        "PS4",
        "NintendoSwitchPro"
    };


    struct Controller
    {
		ControllerID controllerID = INVALID_CONTROLLER_ID;
        ControllerInstanceID controllerInstanceID = INVALID_CONTROLLER_ID; 
		b32 connected = false;
    };

    struct ControllerState
    {
        Controller controller;

        union
        {
            ControllerButton buttons[ControllerButtonName::CONTROLLER_BUTTON_MAX] = {
                {CONTROLLER_BUTTON_A, BUTTON_RELEASED},
                {CONTROLLER_BUTTON_B, BUTTON_RELEASED},
                {CONTROLLER_BUTTON_X, BUTTON_RELEASED},
                {CONTROLLER_BUTTON_Y, BUTTON_RELEASED},
                {CONTROLLER_BUTTON_BACK, BUTTON_RELEASED},
                {CONTROLLER_BUTTON_GUIDE, BUTTON_RELEASED},
                {CONTROLLER_BUTTON_START, BUTTON_RELEASED},
                {CONTROLLER_BUTTON_LEFTSTICK, BUTTON_RELEASED},
                {CONTROLLER_BUTTON_RIGHTSTICK, BUTTON_RELEASED},
                {CONTROLLER_BUTTON_LEFTSHOULDER,BUTTON_RELEASED},
                {CONTROLLER_BUTTON_RIGHTSHOULDER,BUTTON_RELEASED},
                {CONTROLLER_BUTTON_DPAD_UP,BUTTON_RELEASED},
                {CONTROLLER_BUTTON_DPAD_DOWN,BUTTON_RELEASED},
                {CONTROLLER_BUTTON_DPAD_LEFT,BUTTON_RELEASED},
                {CONTROLLER_BUTTON_DPAD_RIGHT,BUTTON_RELEASED}
            };
            
            struct buttons
            {
                ControllerButton buttonA = {CONTROLLER_BUTTON_A, BUTTON_RELEASED};
                ControllerButton buttonB = {CONTROLLER_BUTTON_B, BUTTON_RELEASED};
                ControllerButton buttonX = {CONTROLLER_BUTTON_X, BUTTON_RELEASED};
                ControllerButton buttonY = {CONTROLLER_BUTTON_Y, BUTTON_RELEASED};
                ControllerButton buttonBack = {CONTROLLER_BUTTON_BACK, BUTTON_RELEASED};
                ControllerButton buttonGuide = {CONTROLLER_BUTTON_GUIDE, BUTTON_RELEASED};
                ControllerButton buttonStart = {CONTROLLER_BUTTON_START, BUTTON_RELEASED};
                ControllerButton buttonLeftStick = {CONTROLLER_BUTTON_LEFTSTICK, BUTTON_RELEASED};
                ControllerButton buttonRightStick = {CONTROLLER_BUTTON_RIGHTSTICK, BUTTON_RELEASED};
                ControllerButton buttonLeftShoulder = {CONTROLLER_BUTTON_LEFTSHOULDER,BUTTON_RELEASED};
                ControllerButton buttonRightShoulder = {CONTROLLER_BUTTON_RIGHTSHOULDER,BUTTON_RELEASED};
                ControllerButton buttonDPadUp = {CONTROLLER_BUTTON_DPAD_UP,BUTTON_RELEASED};
                ControllerButton buttonDPadDown = {CONTROLLER_BUTTON_DPAD_DOWN,BUTTON_RELEASED};
                ControllerButton buttonDPadLeft = {CONTROLLER_BUTTON_DPAD_LEFT,BUTTON_RELEASED};
                ControllerButton buttonDPadRight = {CONTROLLER_BUTTON_DPAD_RIGHT,BUTTON_RELEASED};
            };
        };
        
        union
        {
            ControllerAxis axes[ControllerAxisName::CONTROLLER_AXIS_MAX] = {
                 {CONTROLLER_AXIS_LEFTX, 0},
                 {CONTROLLER_AXIS_LEFTY, 0},
                 {CONTROLLER_AXIS_RIGHTX, 0},
                 {CONTROLLER_AXIS_RIGHTY, 0},
                 {CONTROLLER_AXIS_TRIGGERLEFT, 0},
                 {CONTROLLER_AXIS_TRIGGERRIGHT, 0}
            };
            
            struct axis
            {
                ControllerAxis axisLeftX = {CONTROLLER_AXIS_LEFTX, 0};
                ControllerAxis axisLeftY = {CONTROLLER_AXIS_LEFTY, 0};
                ControllerAxis axisRightX = {CONTROLLER_AXIS_RIGHTX, 0};
                ControllerAxis axisRightY = {CONTROLLER_AXIS_RIGHTY, 0};
                ControllerAxis axisTriggerLeft = {CONTROLLER_AXIS_TRIGGERLEFT, 0};
                ControllerAxis axisTriggerRight = {CONTROLLER_AXIS_TRIGGERRIGHT, 0};
            };
        };
    };

    enum eInputType
    {
        INPUT_TYPE_KEYBOARD = 0,
        INPUT_TYPE_MOUSE,
        INPUT_TYPE_KEYBOARD_AND_MOUSE,
        INPUT_TYPE_CONTROLLER
    };
     
    struct InputType
    {
        eInputType inputType = INPUT_TYPE_KEYBOARD;
        ControllerID controllerID = INVALID_CONTROLLER_ID;
    };



}


#if defined(R2_PLATFORM_WINDOWS) || defined(R2_PLATFORM_MAC) || defined(R2_PLATFORM_LINUX)

#include "SDL2/SDL2Input.h"

#endif

#endif /* PlatformInput_h */
