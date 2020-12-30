#ifndef TOKEN_FILE_READER_H
#define TOKEN_FILE_READER_H

#include "hash.h"
#include "object.h"

namespace Token{
    class FileReader{
    private:
        template<typename T>
        T ReadRaw(){
            uint8_t bytes[T::GetSize()];
            if(!ReadBytes(bytes, T::GetSize())){
                LOG(WARNING) << "couldn't read " << T::GetSize() << " bytes from file: " << GetFilename();
                return T();
            }
            return T(bytes, T::GetSize());
        }
    protected:
        FileReader* parent_;
        std::string filename_;
        FILE* file_;

        FileReader(const std::string& filename):
            parent_(nullptr),
            filename_(filename),
            file_(nullptr){}
        FileReader(FileReader* parent):
            parent_(parent),
            filename_(parent->filename_),
            file_(parent->file_){}

        bool HasParent() const{
            return parent_ != nullptr;
        }

        bool HasFilePointer() const{
            return file_ != nullptr;
        }

        FILE* GetFilePointer() const{
            return file_;
        }

        int64_t GetCurrentPosition();
        void SetCurrentPosition(int64_t pos);
    public:
        virtual ~FileReader(){
            if(HasFilePointer() && !HasParent())
                Close();
        }

        std::string GetFilename() const{
            return filename_;
        }

        inline Hash ReadHash(){
            return ReadRaw<Hash>();
        }

        inline User ReadUser(){
            return ReadRaw<User>();
        }

        inline Product ReadProduct(){
            return ReadRaw<Product>();
        }

        bool ReadBytes(uint8_t* bytes, size_t size);
        std::string ReadString();
        int32_t ReadInt();
        uint32_t ReadUnsignedInt();
        int64_t ReadLong();
        uint64_t ReadUnsignedLong();
        void Close();
    };

    class BinaryFileReader : public FileReader{
    protected:
        BinaryFileReader(const std::string& filename):
            FileReader(filename){
            if((file_ = fopen(filename.c_str(), "rb")) == NULL)
                LOG(WARNING) << "couldn't open binary file " << filename << ": " << strerror(errno);
        }
        BinaryFileReader(BinaryFileReader* parent): FileReader(parent){}
    public:
        virtual ~BinaryFileReader() = default;
    };
}

#endif //TOKEN_FILE_READER_H
