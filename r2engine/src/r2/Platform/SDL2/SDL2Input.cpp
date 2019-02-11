//
//  SDL2Input.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-09.
//

#include "SDL2Input.h"
#include <SDL2/SDL.h>

namespace r2
{
    namespace io
    {
        u8 MOUSE_BUTTON_LEFT = SDL_BUTTON_LEFT;
        u8 MOUSE_BUTTON_RIGHT = SDL_BUTTON_RIGHT;
        u8 MOUSE_BUTTON_MIDDLE = SDL_BUTTON_MIDDLE;
        
        u8 BUTTON_PRESSED = SDL_PRESSED;
        u8 BUTTON_RELEASED = SDL_RELEASED;
        
        u32 MOUSE_DIRECTION_NORMAL = SDL_MOUSEWHEEL_NORMAL;
        u32 MOUSE_DIRECTION_FLIPPED = SDL_MOUSEWHEEL_FLIPPED;
        
        u32 SHIFT_PRESSED = KMOD_SHIFT;
        u32 CTRL_PRESSED = KMOD_CTRL;
        u32 ALT_PRESSED = KMOD_ALT;
        
        u32 KEY_RETURN = SDLK_RETURN;
        u32 KEY_ESCAPE = SDLK_ESCAPE;
        u32 KEY_BACKSPACE = SDLK_BACKSPACE;
        u32 KEY_TAB = SDLK_TAB;
        u32 KEY_SPACE = SDLK_SPACE;
        u32 KEY_EXCLAIM = SDLK_EXCLAIM;
        u32 KEY_QUOTEDBL = SDLK_QUOTEDBL;
        u32 KEY_HASH = SDLK_HASH;
        u32 KEY_PERCENT = SDLK_PERCENT;
        u32 KEY_DOLLAR = SDLK_DOLLAR;
        u32 KEY_AMPERSAND = SDLK_AMPERSAND;
        u32 KEY_QUOTE = SDLK_QUOTE;
        u32 KEY_LEFTPAREN = SDLK_LEFTPAREN;
        u32 KEY_RIGHTPAREN = SDLK_RIGHTPAREN;
        u32 KEY_ASTERISK = SDLK_ASTERISK;
        u32 KEY_PLUS = SDLK_PLUS;
        u32 KEY_COMMA = SDLK_COMMA;
        u32 KEY_MINUS = SDLK_MINUS;
        u32 KEY_PERIOD = SDLK_PERIOD;
        u32 KEY_SLASH = SDLK_SLASH;
        u32 KEY_0 = SDLK_0;
        u32 KEY_1 = SDLK_1;
        u32 KEY_2 = SDLK_2;
        u32 KEY_3 = SDLK_3;
        u32 KEY_4 = SDLK_4;
        u32 KEY_5 = SDLK_5;
        u32 KEY_6 = SDLK_6;
        u32 KEY_7 = SDLK_7;
        u32 KEY_8 = SDLK_8;
        u32 KEY_9 = SDLK_9;
        u32 KEY_COLON = SDLK_COLON;
        u32 KEY_SEMICOLON = SDLK_SEMICOLON;
        u32 KEY_LESS = SDLK_LESS;
        u32 KEY_EQUALS = SDLK_EQUALS;
        u32 KEY_GREATER = SDLK_GREATER;
        u32 KEY_QUESTION = SDLK_QUESTION;
        u32 KEY_AT = SDLK_AT;
        
        u32 KEY_LEFTBRACKET = SDLK_LEFTBRACKET;
        u32 KEY_BACKSLASH = SDLK_BACKSLASH;
        u32 KEY_RIGHTBRACKET = SDLK_RIGHTBRACKET;
        u32 KEY_CARET = SDLK_CARET;
        u32 KEY_UNDERSCORE = SDLK_UNDERSCORE;
        u32 KEY_BACKQUOTE = SDLK_BACKQUOTE;
        u32 KEY_a = SDLK_a;
        u32 KEY_b = SDLK_b;
        u32 KEY_c = SDLK_c;
        u32 KEY_d = SDLK_d;
        u32 KEY_e = SDLK_e;
        u32 KEY_f = SDLK_f;
        u32 KEY_g = SDLK_g;
        u32 KEY_h = SDLK_h;
        u32 KEY_i = SDLK_i;
        u32 KEY_j = SDLK_j;
        u32 KEY_k = SDLK_k;
        u32 KEY_l = SDLK_l;
        u32 KEY_m = SDLK_m;
        u32 KEY_n = SDLK_n;
        u32 KEY_o = SDLK_o;
        u32 KEY_p = SDLK_p;
        u32 KEY_q = SDLK_q;
        u32 KEY_r = SDLK_r;
        u32 KEY_s = SDLK_s;
        u32 KEY_t = SDLK_t;
        u32 KEY_u = SDLK_u;
        u32 KEY_v = SDLK_v;
        u32 KEY_w = SDLK_w;
        u32 KEY_x = SDLK_x;
        u32 KEY_y = SDLK_y;
        u32 KEY_z = SDLK_z;
        u32 KEY_CAPSLOCK = SDLK_CAPSLOCK;
        u32 KEY_F1 = SDLK_F1;
        u32 KEY_F2 = SDLK_F2;
        u32 KEY_F3 = SDLK_F3;
        u32 KEY_F4 = SDLK_F4;
        u32 KEY_F5 = SDLK_F5;
        u32 KEY_F6 = SDLK_F6;
        u32 KEY_F7 = SDLK_F7;
        u32 KEY_F8 = SDLK_F8;
        u32 KEY_F9 = SDLK_F9;
        u32 KEY_F10 = SDLK_F10;
        u32 KEY_F11 = SDLK_F11;
        u32 KEY_F12 = SDLK_F12;
        u32 KEY_PRINTSCREEN = SDLK_PRINTSCREEN;
        u32 KEY_SCROLLLOCK = SDLK_SCROLLLOCK;
        u32 KEY_PAUSE = SDLK_PAUSE;
        u32 KEY_INSERT = SDLK_INSERT;
        u32 KEY_HOME = SDLK_HOME;
        u32 KEY_PAGEUP = SDLK_PAGEUP;
        u32 KEY_DELETE = SDLK_DELETE;
        u32 KEY_END = SDLK_END;
        u32 KEY_PAGEDOWN = SDLK_PAGEDOWN;
        u32 KEY_RIGHT = SDLK_RIGHT;
        u32 KEY_LEFT = SDLK_LEFT;
        u32 KEY_DOWN = SDLK_DOWN;
        u32 KEY_UP = SDLK_UP;
    }
}
