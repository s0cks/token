#ifndef TOKEN_FILE_READER_H
#define TOKEN_FILE_READER_H

#include "common.h"
#include "byte_buffer.h"
#include "object.h"
#include "uint256_t.h"

namespace Token{
    class FileReader{
    private:
        std::string filename_;
    protected:
        FileReader(const std::string& filename):
            filename_(filename){}

        int64_t GetCurrentPosition();
        void SetCurrentPosition(int64_t pos);
        virtual FILE* GetFilePointer() const = 0;
    public:
        virtual ~FileReader() = default;

        std::string GetFilename() const{
            return filename_;
        }

        bool ReadBytes(uint8_t* bytes, size_t size);
        std::string ReadString();
        uint256_t ReadHash();
        int32_t ReadInt();
        uint32_t ReadUnsignedInt();
        int64_t ReadLong();
        uint64_t ReadUnsignedLong();
        void Close();

        inline bool
        ReadBytes(ByteBuffer* bytes, size_t size){
            return ReadBytes(bytes->data(), size);
        }

        inline bool
        ReadBytes(ByteBuffer* bytes){
            return ReadBytes(bytes, bytes->GetCapacity());
        }
    };

    class BinaryFileReader : public FileReader{
    private:
        FILE* file_;
    protected:
        BinaryFileReader(const std::string& filename):
            FileReader(filename),
            file_(NULL){
            if((file_ = fopen(filename.c_str(), "rb")) == NULL)
                LOG(WARNING) << "couldn't open binary file " << filename << ": " << strerror(errno);
        }

        virtual FILE* GetFilePointer() const{
            return file_;
        }
    public:
        virtual ~BinaryFileReader(){
            if(file_ != NULL) Close();
        }

        bool ReadRegion(MemoryRegion* region, intptr_t nbytes);
    };
}

#endif //TOKEN_FILE_READER_H
