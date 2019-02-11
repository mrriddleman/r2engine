//
//  PlatformInput.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-09.
//

#ifndef PlatformInput_h
#define PlatformInput_h

namespace r2
{
    namespace io
    {
        extern u8 MOUSE_BUTTON_LEFT;
        extern u8 MOUSE_BUTTON_RIGHT;
        extern u8 MOUSE_BUTTON_MIDDLE;
        
        extern u8 BUTTON_PRESSED;
        extern u8 BUTTON_RELEASED;
        
        extern u32 MOUSE_DIRECTION_NORMAL;
        extern u32 MOUSE_DIRECTION_FLIPPED;
    
        extern u32 SHIFT_PRESSED;
        extern u32 CTRL_PRESSED;
        extern u32 ALT_PRESSED;
        
        extern u32 KEY_RETURN;
        extern u32 KEY_ESCAPE;
        extern u32 KEY_BACKSPACE;
        extern u32 KEY_TAB;
        extern u32 KEY_SPACE;
        extern u32 KEY_EXCLAIM;
        extern u32 KEY_QUOTEDBL;
        extern u32 KEY_HASH;
        extern u32 KEY_PERCENT;
        extern u32 KEY_DOLLAR;
        extern u32 KEY_AMPERSAND;
        extern u32 KEY_QUOTE;
        extern u32 KEY_LEFTPAREN;
        extern u32 KEY_RIGHTPAREN;
        extern u32 KEY_ASTERISK;
        extern u32 KEY_PLUS;
        extern u32 KEY_COMMA;
        extern u32 KEY_MINUS;
        extern u32 KEY_PERIOD;
        extern u32 KEY_SLASH;
        extern u32 KEY_0;
        extern u32 KEY_1;
        extern u32 KEY_2;
        extern u32 KEY_3;
        extern u32 KEY_4;
        extern u32 KEY_5;
        extern u32 KEY_6;
        extern u32 KEY_7;
        extern u32 KEY_8;
        extern u32 KEY_9;
        extern u32 KEY_COLON;
        extern u32 KEY_SEMICOLON;
        extern u32 KEY_LESS;
        extern u32 KEY_EQUALS;
        extern u32 KEY_GREATER;
        extern u32 KEY_QUESTION;
        extern u32 KEY_AT;
        extern u32 KEY_LEFTBRACKET;
        extern u32 KEY_BACKSLASH;
        extern u32 KEY_RIGHTBRACKET;
        extern u32 KEY_CARET;
        extern u32 KEY_UNDERSCORE;
        extern u32 KEY_BACKQUOTE;
        extern u32 KEY_a;
        extern u32 KEY_b;
        extern u32 KEY_c;
        extern u32 KEY_d;
        extern u32 KEY_e;
        extern u32 KEY_f;
        extern u32 KEY_g;
        extern u32 KEY_h;
        extern u32 KEY_i;
        extern u32 KEY_j;
        extern u32 KEY_k;
        extern u32 KEY_l;
        extern u32 KEY_m;
        extern u32 KEY_n;
        extern u32 KEY_o;
        extern u32 KEY_p;
        extern u32 KEY_q;
        extern u32 KEY_r;
        extern u32 KEY_s;
        extern u32 KEY_t;
        extern u32 KEY_u;
        extern u32 KEY_v;
        extern u32 KEY_w;
        extern u32 KEY_x;
        extern u32 KEY_y;
        extern u32 KEY_z;
        extern u32 KEY_CAPSLOCK;
        extern u32 KEY_F1;
        extern u32 KEY_F2;
        extern u32 KEY_F3;
        extern u32 KEY_F4;
        extern u32 KEY_F5;
        extern u32 KEY_F6;
        extern u32 KEY_F7;
        extern u32 KEY_F8;
        extern u32 KEY_F9;
        extern u32 KEY_F10;
        extern u32 KEY_F11;
        extern u32 KEY_F12;
        extern u32 KEY_PRINTSCREEN;
        extern u32 KEY_SCROLLLOCK;
        extern u32 KEY_PAUSE;
        extern u32 KEY_INSERT;
        extern u32 KEY_HOME;
        extern u32 KEY_PAGEUP;
        extern u32 KEY_DELETE;
        extern u32 KEY_END;
        extern u32 KEY_PAGEDOWN;
        extern u32 KEY_RIGHT;
        extern u32 KEY_LEFT;
        extern u32 KEY_DOWN;
        extern u32 KEY_UP;
        
        struct R2_API MouseData
        {
            u8 numClicks = 0;
            u8 state  = 0;
            u8 button = 0;
            s32 x = 0, y = 0;
            s32 xrel = 0, yrel = 0;
            u32 direction = MOUSE_DIRECTION_NORMAL;
        };
        
        struct R2_API Key
        {
            enum
            {
                SHIFT_PRESSED = 1 << 0,
                CONTROL_PRESSED = 1 << 1,
                ALT_PRESSED = 1 << 2,
            };
            
            u32 code = 0;
            u32 modifiers = 0;
            b32 repeated = 0;
            u8 state = 0;
        };
    }
}


#if defined(R2_PLATFORM_WINDOWS) || defined(R2_PLATFORM_MAC) || defined(R2_PLATFORM_LINUX)

#include "SDL2Input.h"

#endif

#endif /* PlatformInput_h */
