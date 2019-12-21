//
//  DirectoryUtils.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-12-20.
//

#ifndef DirectoryUtils_h
#define DirectoryUtils_h

namespace r2::fs::dir
{
    void CreateFileListFromDirectory(const char* directory, const char* ext, r2::SArray<char[r2::fs::FILE_PATH_LENGTH]>* fileList);
}

#endif /* DirectoryUtils_h */
