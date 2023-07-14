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
    enum Directory: u32
    {
        ROOT = 0,
        SOUND_DEFINITIONS,
        SOUNDS,
        TEXTURES,
        SHADERS_RAW,
        SHADERS_BIN,
        SHADERS_MANIFEST,
        MODELS,
        ANIMATIONS,
        LEVELS
    };
    
#ifdef R2_PLATFORM_WINDOWS
    static const char PATH_SEPARATOR = '/';
#else
    static const char PATH_SEPARATOR = '/';
#endif
    
    static const char FILE_EXTENSION_DELIM = '.';
    
    char* GetNextSubPath(const char* path, char* subPath, const char delim = PATH_SEPARATOR);
    
    bool CopyFileNameWithExtension(const char* path, char* filename);
    
    bool CopyFileName(const char* path, char* filename);
    
    bool CopyFileExtension(const char* path, char* ext);
    
    bool CopyFileExtensionWithoutTheDot(const char* path, char* ext);

    bool CopyDirectoryOfFile(const char* filePath, char* path);
    
    bool CopyFileNameWithParentDirectories(const char* filePath, char* fileNameWithDirectories, u32 numParents);

    char* GetLastSubPath(const char* path, char* subPath, const char delim = PATH_SEPARATOR);
    
    u32 NumMatchingSubPaths(const char* path1, const char* path2, const char delim = PATH_SEPARATOR);
    
    //@TODO(Serge): fix ordering of the params - resultPath should be first?
    bool AppendSubPath(const char* path, char* resultPath, const char* subPath, const char delim = PATH_SEPARATOR);
    
    bool AppendExt(char* path, const char* ext);

    bool SetNewExtension(char* path, const char* ext);
    
    bool BuildPathFromCategory(Directory category, const char* fileName, char* outPath);
    
    bool GetParentDirectory(const char* path, char* result);
    
    bool SanitizeSubPath(const char* rawSubPath, char* result);

    bool GetRelativePath(const char* basePath, const char* path, char* result, const char delim = PATH_SEPARATOR);

}

#endif /* PathUtils_h */
