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
            
            s32 code = 0;
            u32 modifiers = 0;
            b32 repeated = 0;
            u8 state = 0;
        };
    }
}


#if defined(R2_PLATFORM_WINDOWS) || defined(R2_PLATFORM_MAC) || defined(R2_PLATFORM_LINUX)

#include "SDL2/SDL2Input.h"

#endif

#endif /* PlatformInput_h */
