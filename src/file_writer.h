#ifndef TOKEN_FILE_WRITER_H
#define TOKEN_FILE_WRITER_H

#include "common.h"
#include "bytes.h"
#include "object.h"
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
        void WriteBytes(uint8_t* bytes, size_t size);
        void WriteInt(int32_t value);
        void WriteUnsignedInt(uint32_t value);
        void WriteLong(int64_t value);
        void WriteUnsignedLong(uint64_t value);
        void WriteHash(const uint256_t& value);
        void WriteString(const std::string& value);
        void SetCurrentPosition(int64_t pos);
        void Flush();
        void Close();

        inline void
        WriteBytes(ByteBuffer* bytes){
            WriteBytes(bytes->data(), bytes->GetWrittenBytes());
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

        void WriteObject(Object* obj);

        template<typename T>
        void WriteObject(const Handle<T>& obj){
            WriteObject((Object*)obj);
        }
    };
}

#endif //TOKEN_FILE_WRITER_H