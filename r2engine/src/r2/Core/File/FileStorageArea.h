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
        class R2_API FileStorageArea
        {
        public:
            
            FileStorageArea(const char* rootPath);
            virtual ~FileStorageArea();
            
            virtual bool Mount() {return true;}
            virtual bool Unmount() {return true;}
            virtual bool Commit() {return true;}
            
            inline const char* RootPath() const {return &mRootPath[0];}
        private:
            char mRootPath[Kilobytes(1)];
        };
    }
}

#endif /* FileStorageArea_h */
