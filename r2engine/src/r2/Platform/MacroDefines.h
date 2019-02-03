//
//  r2_windows.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-02-02.
//  Copyright © 2019 Serge Lansiquot. All rights reserved.
//

#ifndef r2_macro_h
#define r2_macro_h

    #ifdef R2_PLATFORM_WINDOWS
        #ifdef R2_BUILD_DLL
            #define R2_API __declspec(dllexport)
        #else
            #define R2_API __declspec(dllimport)
        #endif
    #elif defined R2_PLATFORM_MAC
        #define R2_API
    #else
        #error r2 is only supported by Windows and Mac
    #endif

#endif /* r2_macro_h */
