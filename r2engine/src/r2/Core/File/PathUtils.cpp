//
//  PathUtils.cpp
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-16.
//
#include "r2pch.h"
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
        
        size_t maxPathLength = strlen(path);
        char* tempPath = const_cast<char*>(path);
        size_t i = 0;
        
        //Ignore any/all start delims
        while (tempPath[i] != '\0' && tempPath[i] == delim)
        {
            ++i;
        }

        size_t lengthOfSubPath = 0;
        
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
        
        r2::util::PathCpy(ext, result);
        
        return result && ext && strlen(ext) > 0;
    }

    bool CopyFileExtensionWithoutTheDot(const char* path, char* ext)
    {
        char* tempPath = const_cast<char*>(path);
        char* result = GetLastSubPath(tempPath, ext, PATH_SEPARATOR);

        result = GetLastSubPath(result, ext, FILE_EXTENSION_DELIM);

        return result && ext && strlen(ext) > 0;
    }
    
    bool CopyDirectoryOfFile(const char* filePath, char* path)
    {
        if (!filePath || !path)
        {
            return false;
        }
        
        r2::util::PathCpy(path, "");
        
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

            r2::util::PathCat(path, nexSubPath);
            auto len = strlen(path);

            R2_CHECK(len < r2::fs::FILE_PATH_LENGTH, "should be less than 1 kilobyte");
            path[len++] = PATH_SEPARATOR;
            R2_CHECK(len < r2::fs::FILE_PATH_LENGTH, "should be less than 1 kilobyte");
            path[len] = '\0';
        }
        
        return true;
    }

    bool CopyFileNameWithParentDirectories(const char* filePath, char* fileNameWithDirectories, u32 numParents)
    {
		if (!filePath || !fileNameWithDirectories)
		{
			return false;
		}

		r2::util::PathCpy(fileNameWithDirectories, "");

        const u32 subPathEndCounter = numParents + 1;
        size_t len = strlen(filePath);
        if (len == 0)
            return false;


        if (numParents == 0)
        {
            //we need to find the start of the string
            //which will be either the first encounter of a PATH_SEPARATOR or the start of the string
            s32 startIndex = static_cast<s32>(len) - 1;

            for (s32 i = startIndex; i >= 0; --i)
            {
                startIndex = i;
                if (filePath[i] == PATH_SEPARATOR)
                {
                    startIndex++;
                    break;
                }
            }

			for (u32 i = 0; i < len; ++i)
			{
				fileNameWithDirectories[i] = filePath[i + startIndex];
			}

            fileNameWithDirectories[len] = '\0';

            return true;
        }

        u32 subPathCounter = 0;
        size_t startIndex = len - 1;


        bool finishedCorrectly = false;
        for (s32 i = static_cast<s32>(startIndex); i >= 0; --i)
        {
            if (filePath[i] == PATH_SEPARATOR)
            {
                subPathCounter++;

                if (subPathCounter == subPathEndCounter)
                {
                    finishedCorrectly = true;
                    startIndex = static_cast<size_t>(i) +1;
                    break;
                }
            }
        }

        if (!finishedCorrectly)
            return false;

        size_t resultLen = len - startIndex;

        for (u32 i = 0; i < resultLen; ++i)
        {
            fileNameWithDirectories[i] = filePath[startIndex + i];
        }

        fileNameWithDirectories[resultLen] = '\0';

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
        char sanitizedRootPath[r2::fs::FILE_PATH_LENGTH];
        SanitizeSubPath(path, sanitizedRootPath);

        auto len = strlen(sanitizedRootPath);
        r2::util::PathCpy(resultPath, sanitizedRootPath);
        
        if (resultPath[len-1] != delim)
        {
            resultPath[len++] = delim;
            resultPath[len] = '\0';
        }

        char sanitizedSubPath[r2::fs::FILE_PATH_LENGTH];
        
        //@TODO(Serge): should do this in a loop so we can resolve /../../ etc
        if (strlen(subPath) > 2)
        {
            if (subPath[0] == '.' && subPath[1] == '.')
            {
                GetParentDirectory(path, resultPath);
            }
        }
        
        SanitizeSubPath(subPath, sanitizedSubPath);
        
        r2::util::PathCat(resultPath, sanitizedSubPath);
        
        return true;
    }
    
    bool AppendExt(char* path, const char* ext)
    {
        r2::util::PathCat(path, ext);
        return true;
    }

    bool SetNewExtension(char* path, const char* newExt)
    {
        size_t len = strlen(path);
        s32 index = -1;
        for (s32 i = (u32)len - 1; i >= 0; --i)
        {
            if (path[i] == FILE_EXTENSION_DELIM)
            {
                index = i;
                break;
            }
        }

        size_t extLen = strlen(newExt);

        for (s32 i = 0; i < extLen; ++i)
        {
            path[index + i] = newExt[i];
        }

        path[index + extLen] = '\0';

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
    
    bool SanitizeSubPath(const char* rawSubPath, char* result)
    {
        size_t startingIndex = 0;
        //@TODO(Serge): do in a loop
        if (strlen(rawSubPath) > 2) {
            if (rawSubPath[0] == '.' && rawSubPath[1] == '.')
            {
                startingIndex = 2;
            }
            else if(rawSubPath[0] == '.' && rawSubPath[1] != '.')
            {
                startingIndex = 1;
            }
        }
        
        size_t len = strlen(rawSubPath);
        size_t resultIndex = 0;
        for ( ;startingIndex < len; startingIndex++)
        {
            if (PATH_SEPARATOR != '\\' && rawSubPath[startingIndex] == '\\')
            {
                result[resultIndex] = PATH_SEPARATOR;
            }
            else
            {
                result[resultIndex] = rawSubPath[startingIndex];
            }
            
            ++resultIndex;
        }
        
        result[resultIndex] = '\0';
        
        return true;
    }

    bool GetRelativePath(const char* basePath, const char* path, char* result, const char delim)
    {
        size_t basePathLength = strlen(basePath);
        size_t pathLength = strlen(path);
       
        
        if (pathLength < basePathLength)
        {
            r2::util::PathCpy(result, path);
            return true;
        }

        char baseSubPath[r2::fs::FILE_PATH_LENGTH];
        char nextSubPath[r2::fs::FILE_PATH_LENGTH];

        char* resultBasePath = const_cast<char*>( basePath );
        char* resultPath = const_cast<char*>(path);

        char* prevResultBasePath = resultBasePath;
        char* prevResultPath = resultPath;

        while (*resultBasePath && *resultPath)
        {
            resultBasePath = GetNextSubPath(resultBasePath, baseSubPath, delim);
            resultPath = GetNextSubPath(resultPath, nextSubPath, delim);

            if (!(resultBasePath && resultPath &&
                strcmp(baseSubPath, nextSubPath) == 0))
            {
                break;
            }
            
            prevResultBasePath = resultBasePath;
            prevResultPath = resultPath;
        } 


        int offset = 0;
        if (prevResultPath[0] == delim)
        {
            offset = 1;
        }

        r2::util::PathCpy(result, prevResultPath + offset);

        return strlen(result) > 0;
    }
    
    bool GetParentDirectory(const char* path, char* result)
    {
        size_t len = strlen(path);
        size_t startingIndex = len - 1;
        if (path[startingIndex] == PATH_SEPARATOR)
        {
            --startingIndex;
        }
        
        for (; startingIndex > 0; --startingIndex)
        {
            if (path[startingIndex] == PATH_SEPARATOR)
            {
                break;
            }
        }
        
        r2::util::PathNCpy(result, path, startingIndex + 2);
        return true;
    }

#ifdef R2_ASSET_PIPELINE
    bool HasParentInPath(const std::filesystem::path& path, const std::filesystem::path& parent)
    {
        std::filesystem::path nextPath = path.parent_path();

        while (nextPath != "")
        {
            if (nextPath == parent)
            {
                return true;
            }
            nextPath = nextPath.parent_path();
        }

        return false;
    }
#endif
}
