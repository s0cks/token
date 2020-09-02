#ifndef TOKEN_FILE_WRITER_H
#define TOKEN_FILE_WRITER_H

#include "common.h"
#include "bytes.h"
#include "object.h"
#include "vmemory.h"
#include "uint256_t.h"

namespace Token{
    class FileWriter{
    private:
        std::string filename_;
    protected:
        FileWriter(const std::string& filename):
            filename_(filename){}

        virtual FILE* GetFilePointer() const = 0;
    public:
        virtual ~FileWriter() = default;

        std::string GetFilename() const{
            return filename_;
        }

        int64_t GetCurrentPosition();
        bool WriteBytes(uint8_t* bytes, size_t size);
        bool WriteInt(int32_t value);
        bool WriteUnsignedInt(uint32_t value);
        bool WriteLong(int64_t value);
        bool WriteUnsignedLong(uint64_t value);
        bool WriteHash(const uint256_t& value);
        bool WriteString(const std::string& value);
        bool SetCurrentPosition(int64_t pos);
        bool Flush();
        bool Close();

        inline bool
        WriteBytes(ByteBuffer* bytes){
            return WriteBytes(bytes->data(), bytes->GetWrittenBytes());
        }

        inline bool
        WriteBytes(MemoryRegion* region){
            return WriteBytes((uint8_t*)region->GetStartAddress(), region->GetSize());
        }
    };

    class TextFileWriter : public FileWriter{

    };

    class BinaryFileWriter : public FileWriter{
    private:
        FILE* file_;
    protected:
        BinaryFileWriter(const std::string& filename):
            FileWriter(filename),
            file_(NULL){
            if((file_ = fopen(filename.c_str(), "wb")) == NULL)
                LOG(WARNING) << "couldn't create binary file " << filename << ": " << strerror(errno);
        }

        virtual FILE* GetFilePointer() const{
            return file_;
        }
    public:
        virtual ~BinaryFileWriter(){
            if(file_ != NULL) Close();
        }

        bool WriteObject(Object* obj);

        template<typename T>
        bool WriteObject(const Handle<T>& obj){
            return WriteObject((Object*)obj);
        }
    };
}

#endif //TOKEN_FILE_WRITER_H