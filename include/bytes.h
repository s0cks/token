#ifndef TOKEN_BYTES_H
#define TOKEN_BYTES_H

#include <vector>
#include <fstream>
#include <cstring>
#include <cstdint>
#include <bitset>
#include <google/protobuf/message.h>

namespace Token{
    //TODO: Remove/Refactor Class
    class ByteBuffer{
    public:
        static const uint32_t DEFAULT_SIZE = 4096;
    private:
        uint32_t wpos_;
        uint32_t rpos_;
        std::vector<uint8_t> buff_;

        template<typename T>
        T Read(uint32_t idx) {
            if(idx + sizeof(T) <= buff_.size()){
                return *reinterpret_cast<T*>(&buff_[idx]);
            }
            return 0;
        }

        template<typename T>
        T Read(){
            T data = Read<T>(rpos_);
            rpos_ += sizeof(T);
            return data;
        }

        template<typename T>
        void Append(T data){
            uint32_t s = sizeof(data);
            if(Size() < (wpos_ + s)){
                buff_.resize(wpos_ + s);
            }
            memcpy(&buff_[wpos_], (uint8_t*)&data, s);
            wpos_ += s;
        }

        template<typename T>
        void Insert(T data, uint32_t idx){
            if((idx + sizeof(data)) > Size()) return;
            memcpy(&buff_[idx], (uint8_t*)&data, sizeof(data));
            wpos_ = idx + sizeof(data);
        }
    public:
        ByteBuffer(std::fstream& stream):
            rpos_(0),
            wpos_(0),
            buff_(){
                stream.seekg(0, std::ios_base::end);
                size_t length = stream.tellg();
                stream.seekg(0, std::ios_base::beg);
                buff_.reserve(length);
                std::copy(
                        std::istreambuf_iterator<char>(stream),
                        std::istreambuf_iterator<char>(),
                        std::back_inserter(buff_)
                );
            }
        ByteBuffer(uint32_t size = DEFAULT_SIZE):
            rpos_(0),
            wpos_(0),
            buff_(size){}
        ByteBuffer(uint8_t* data, uint32_t size):
            rpos_(0),
            wpos_(0),
            buff_(size){
                buff_.reserve(size);
                Clear();
                if(data != nullptr){
                    PutBytes(data, size);
                }
            }
        ByteBuffer(const ByteBuffer& other):
            wpos_(other.wpos_),
            rpos_(other.rpos_),
            buff_(other.buff_){}
        ~ByteBuffer(){}

        uint8_t* GetBytes() {
            return buff_.data();
        }

        uint32_t Size(){
            return static_cast<uint32_t>(buff_.size());
        }

        uint32_t BytesRemaining(){
            return Size() - rpos_;
        }

        uint8_t* GetRemainingBytes(){
            return &buff_.data()[rpos_];
        }

        uint32_t WrittenBytes(){
            return wpos_;
        }

        void Clear(){
            rpos_ = 0;
            wpos_ = 0;
            buff_.clear();
        }

        void Resize(uint32_t nsize){
            buff_.resize(nsize);
            rpos_ = 0;
            wpos_ = 0;
        }

        uint8_t Get() {
            return Read<uint8_t>(rpos_);
        }

        uint8_t Get(uint32_t idx){
            return Read<uint8_t>(idx);
        }

        void GetBytes(uint8_t* buff, uint32_t len) {
            memcpy(buff, GetBytes(), len);
        }

        void GetBytes(uint8_t* buff, uint32_t offset, uint32_t len) {
            for(int i = 0; i < len; i++){
                buff[i] = Get(offset + i);
            }
        }

        char GetChar(){
            return Read<char>();
        }

        char GetChar(uint32_t idx){
            return Read<char>(idx);
        }

        double GetDouble(){
            return Read<double>();
        }

        double GetDouble(uint32_t idx){
            return Read<double>(idx);
        }

        uint32_t GetInt(){
            return Read<uint32_t>();
        }

        uint32_t GetInt(uint32_t idx){
            return Read<uint32_t>(idx);
        }
        uint64_t GetLong(){
            return Read<uint64_t>();
        }

        uint64_t GetLong(uint32_t idx){
            return Read<uint64_t>(idx);
        }

        void Put(ByteBuffer* src){
            for(uint32_t idx = 0; idx < src->Size(); idx++){
                Append<uint8_t>(src->Get(idx));
            }
        }

        void Put(uint8_t val){
            Append<uint8_t>(val);
        }

        void Put(uint8_t val, uint32_t idx){
            Insert<uint8_t>(val, idx);
        }

        void PutBytes(uint8_t* val, uint32_t len){
            for(uint32_t idx = 0; idx < len; idx++) Append<uint8_t>(val[idx]);
        }

        void PutBytes(uint8_t* val, uint32_t len, uint32_t idx){
            wpos_ = idx;
            for(idx = 0; idx < len; idx++){
                Append<uint8_t>(val[idx]);
            }
        }

        void PutChar(char val){
            Append<char>(val);
        }

        void PutChar(char val, uint32_t idx){
            Insert<char>(val, idx);
        }

        void PutDouble(double val){
            Append<double>(val);
        }

        void PutDouble(double val, uint32_t idx){
            Insert<double>(val, idx);
        }

        void PutInt(uint32_t val){
            Append<uint32_t>(val);
        }

        void PutInt(uint32_t val, uint32_t idx){
            Insert<uint32_t>(val, idx);
        }

        void PutLong(uint64_t val){
            Append<uint64_t>(val);
        }

        void PutLong(uint64_t val, uint32_t idx){
            Insert<uint64_t>(val, idx);
        }

        void PutMessage(google::protobuf::Message* msg){
            uint8_t bytes[msg->ByteSizeLong()];
            msg->SerializeToArray(bytes, msg->ByteSizeLong());
            PutBytes(bytes, msg->ByteSizeLong());
        }

        friend std::ostream& operator<<(std::ostream& stream, const ByteBuffer& bb){
            stream << "ByteBuffer(";
            for(uint8_t byte : bb.buff_){
                stream << byte << std::endl;
            }
            return stream;
        }
    };
}

#endif //TOKEN_BYTES_H