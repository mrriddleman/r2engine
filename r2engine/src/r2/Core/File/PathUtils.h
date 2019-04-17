//
//  PathUtils.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-16.
//

#ifndef PathUtils_h
#define PathUtils_h

namespace r2::fs::utils
{
#ifdef R2_PLATFORM_WINDOWS
    static const char PATH_SEPARATOR = '\\';
#else
    static const char PATH_SEPARATOR = '/';
#endif
    
    static const char FILE_EXTENSION_DELIM = '.';
    
    char* GetNextSubPath(const char* path, char* subPath, const char delim);
    
    bool CopyFileNameWithExtension(const char* path, char* filename);
    
    bool CopyFileName(const char* path, char* filename);
    
    bool CopyFileExtension(const char* path, char* ext);
    
    char* GetLastSubPath(const char* path, char* subPath, const char delim);
    
    u32 NumMatchingSubPaths(const char* path1, const char* path2, const char delim);
    
}

#endif /* PathUtils_h */
