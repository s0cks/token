#ifndef TOKEN_FILE_WRITER_H
#define TOKEN_FILE_WRITER_H

#include <iostream>
#include "object.h"
#include "buffer.h"
#include "vmemory.h"

namespace Token{
    class FileWriter{
    private:
        std::string filename_;
    protected:
        FILE* file_;

        FileWriter(const std::string& filename):
            filename_(filename),
            file_(nullptr){}

        bool HasFilePointer() const{
            return file_ != nullptr;
        }

        FILE* GetFilePointer() const{
            return file_;
        }
    public:
        virtual ~FileWriter(){
            if(HasFilePointer()) Close();
        }

        std::string GetFilename() const{
            return filename_;
        }

        int64_t GetCurrentPosition();
        bool SetCurrentPosition(int64_t pos);
        bool Flush();
        bool Close();
    };

    class TextFileWriter : public FileWriter{
    protected:
        size_t indent_;

        TextFileWriter(const std::string& filename):
            FileWriter(filename),
            indent_(0){
            if((file_ = fopen(filename.c_str(), "w")) == NULL)
                LOG(WARNING) << "couldn't create text file " << filename << ": " << strerror(errno);
        }

        inline void Indent(){
            indent_++;
        }

        inline void DeIndent(){
            indent_--;
        }

        size_t GetIndent() const{
            return indent_;
        }
    public:
        ~TextFileWriter() = default;

        bool Write(const std::string& value);
        bool Write(uint32_t value);
        bool Write(int32_t value);
        bool Write(uint64_t value);
        bool Write(int64_t value);
        bool Write(const Hash& hash);
        bool Write(Object* obj);
        bool NewLine();

        inline bool
        Write(const std::stringstream& ss){
            return Write(ss.str());
        }

        template<typename T>
        inline bool Write(const Handle<T>& obj){
            return Write((Object*)obj);
        }

        inline bool
        WriteLine(const std::string& value){
            return Write(value + '\n');
        }

        inline bool
        WriteLine(const std::stringstream& value){
            return Write(value.str() + '\n');
        }
    };

    class BinaryFileWriter : public FileWriter{
    protected:
        BinaryFileWriter(const std::string& filename):
            FileWriter(filename){
            if((file_ = fopen(filename.c_str(), "wb")) == NULL)
                LOG(WARNING) << "couldn't create binary file " << filename << ": " << strerror(errno);
        }
    public:
        ~BinaryFileWriter() = default;

        bool WriteBytes(uint8_t* bytes, intptr_t size);
        bool WriteInt(int32_t value);
        bool WriteUnsignedInt(uint32_t value);
        bool WriteLong(int64_t value);
        bool WriteUnsignedLong(uint64_t value);
        bool WriteHash(const Hash& value);
        bool WriteString(const std::string& value);
        bool WriteObject(Object* obj);

        inline bool
        WriteBytes(const Handle<Buffer>& buff){
            return WriteBytes((uint8_t*)buff->data(), buff->GetWrittenBytes());
        }

        inline bool
        WriteRegion(MemoryRegion* region, intptr_t nbytes){
            return WriteBytes((uint8_t*)region->GetStartAddress(), nbytes);
        }

        inline bool
        WriteRegion(MemoryRegion* region){
            return WriteBytes((uint8_t*)region->GetStartAddress(), region->GetSize());
        }

        template<typename T>
        inline bool WriteObject(const Handle<T>& obj){
            return WriteObject((Object*)obj);
        }
    };
}

#endif //TOKEN_FILE_WRITER_H