//
//  PathUtils.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-16.
//

#include "PathUtils.h"

namespace r2::fs::utils
{
    char* GetNextSubPath(const char* path, char* subPath, const char delim)
    {
        if (!path || !subPath)
        {
            return nullptr;
        }
        
        u32 maxPathLength = strlen(path);
        char* tempPath = const_cast<char*>(path);
        u32 i = 0;
        
        //Ignore any/all start delims
        while (tempPath[i] != '\0' && tempPath[i] == delim)
        {
            ++i;
        }

        u32 lengthOfSubPath = 0;
        
        for (; i < maxPathLength && tempPath[i] != '\0'; ++i)
        {
            if (tempPath[i] != delim)
            {
                subPath[lengthOfSubPath++] = tempPath[i];
            }
            else
            {
                break;
            }
        }
        
        char* returnPath = &tempPath[i];
        
        subPath[lengthOfSubPath] = '\0';
        
        return returnPath;
    }
    
    bool CopyFileNameWithExtension(const char* path, char* filename)
    {
        char* tempPath = const_cast<char*>(path);
        char* result = GetLastSubPath(tempPath, filename, PATH_SEPARATOR);
        return result && filename && strlen(filename) > 0;
    }
    
    bool CopyFileName(const char* path, char* filename)
    {
        char* tempPath = const_cast<char*>(path);
        char* result = GetLastSubPath(tempPath, filename, PATH_SEPARATOR);
        char* tempFilename = filename;
        GetNextSubPath(tempFilename, filename, FILE_EXTENSION_DELIM);
        
        return result && filename && strlen(filename) > 0;
    }
    
    bool CopyFileExtension(const char* path, char* ext)
    {
        char* tempPath = const_cast<char*>(path);
        char* result = GetLastSubPath(tempPath, ext, PATH_SEPARATOR);
        
        result = GetLastSubPath(result, ext, FILE_EXTENSION_DELIM);
        
        strcpy(ext, result);
        
        return result && ext && strlen(ext) > 0;
    }
    
    char* GetLastSubPath(const char* path, char* subPath, const char delim)
    {
        if (!path || !subPath)
        {
            return nullptr;
        }
        
        u64 length = strlen(path);
        
        if (length == 0)
        {
            return nullptr;
        }
        
        u64 i = length;
        char* tempPath = const_cast<char*>(path);
        
        while (i > 0 && tempPath[i-1] == delim)
        {
            --i;
        }
        
        u32 lengthOfSubPath = 0;
        u64 lastCharIndex = i;
        u64 firstCharIndex = lastCharIndex;
        
        for (; i > 0; --i)
        {
            if (tempPath[i-1] == delim)
            {
                firstCharIndex = i;
                break;
            }
        }
        
        if (firstCharIndex == lastCharIndex)
        {
            firstCharIndex = 0;
        }
        
        for (u64 j = firstCharIndex; j < lastCharIndex; ++j)
        {
            subPath[lengthOfSubPath++] = tempPath[j];
        }
        
        char* returnPath = &tempPath[i-1];
        
        subPath[lengthOfSubPath] = '\0';
        
        return returnPath;
    }
    
    u32 NumMatchingSubPaths(const char* path1, const char* path2, const char delim)
    {
        if (!path1 || !path2)
        {
            return 0;
        }
        
        char subPath[Kilobytes(1)] = "";
        char storageSubPath[Kilobytes(1)] = "";
        
        //Promise you won't modify me
        char* splitPath = const_cast<char*>(path1);
        char* storageAreaPath = const_cast<char*>(path2);
        
        u32 numSubMatches = 0;
        
        while (*splitPath && *storageAreaPath)
        {
            splitPath = GetNextSubPath(splitPath, subPath, delim);
            storageAreaPath = GetNextSubPath(storageAreaPath, storageSubPath, delim);

            if (splitPath && storageAreaPath &&
                strcmp(storageSubPath, subPath) == 0)
            {
                ++numSubMatches;
            }
            else
            {
                break;
            }
        }
        
        return numSubMatches;
    }
}
