//
//  FileStorageArea.h
//  r2engine
//
//  Created by Serge Lansiquot on 2019-04-06.
//

#ifndef FileStorageArea_h
#define FileStorageArea_h

namespace r2
{
    namespace fs
    {
        class FileStorageArea
        {
        public:
            
            FileStorageArea(const char* rootPath);
            virtual ~FileStorageArea();
            
            virtual bool Mount() {};
            virtual bool Unmount() {};
            virtual bool Commit() {};
            
            inline const char* RootPath() const {return &rootPath[0];}
        private:
            char mRootPath[Kilobytes(1)];
        };
    }
}

#endif /* FileStorageArea_h */
