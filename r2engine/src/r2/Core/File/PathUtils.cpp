//
//  PathUtils.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-16.
//

#include "PathUtils.h"
#include "r2/Core/File/File.h"
#include "r2/Platform/Platform.h"

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
    
    bool CopyDirectoryOfFile(const char* filePath, char* path)
    {
        if (!filePath || !path)
        {
            return false;
        }
        
        strcpy(path, "");
        
        char filenameWithExtension[r2::fs::FILE_PATH_LENGTH];
        
        bool result = CopyFileNameWithExtension(filePath, filenameWithExtension);
        
        if (!result)
        {
            return false;
        }
        
        char nexSubPath[r2::fs::FILE_PATH_LENGTH];
        char* nextPath = const_cast<char*>(filePath);
        
        if (nextPath[0] == PATH_SEPARATOR)
        {
            path[0] = PATH_SEPARATOR;
            path[1] = '\0';
        }
        
        while (true)
        {
            nextPath = GetNextSubPath(nextPath, nexSubPath, PATH_SEPARATOR);
            
            if (strlen(nexSubPath) <= 0 || strcmp(nexSubPath, filenameWithExtension) == 0)
            {
                //done
                break;
            }

            strcat(path, nexSubPath);
            auto len = strlen(path);

            R2_CHECK(len < r2::fs::FILE_PATH_LENGTH, "should be less than 1 kilobyte");
            path[len++] = PATH_SEPARATOR;
            R2_CHECK(len < r2::fs::FILE_PATH_LENGTH, "should be less than 1 kilobyte");
            path[len] = '\0';
        }
        
        return true;
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
        
        char subPath[r2::fs::FILE_PATH_LENGTH] = "";
        char storageSubPath[r2::fs::FILE_PATH_LENGTH] = "";
        
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
    
    bool AppendSubPath(const char* path, char* resultPath, const char* subPath, const char delim)
    {
        auto len = strlen(path);
        strcpy(resultPath, path);
        
        if (resultPath[len-1] != delim)
        {
            resultPath[len++] = delim;
            resultPath[len] = '\0';
        }
        
        strcat(resultPath, subPath);
        
        return true;
    }
    
    bool AppendExt(char* path, const char* ext)
    {
        strcat(path, ext);
        return true;
    }
    
    bool BuildPathFromCategory(Directory category, const char* fileName, char* outPath)
    {
        auto pathResolver = CPLAT.GetPathResolver();
        char rootPath[r2::fs::FILE_PATH_LENGTH];
        
        if (pathResolver(category, rootPath))
        {
            return AppendSubPath(rootPath, outPath, fileName);
        }
        
        return false;
    }
}
