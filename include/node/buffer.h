#ifndef TOKEN_BUFFER_H
#define TOKEN_BUFFER_H

#include "common.h"
#include "message.h"

namespace Token{
    class ByteBuffer{
    private:
        uint8_t* buff_;
        uintptr_t capacity_;
        uintptr_t wpos_;
        uintptr_t rpos_;

        void Resize(uintptr_t ncap){
            if(ncap > capacity_){
                ncap = RoundUpPowTwo(ncap);
                uint8_t* ndata = (uint8_t*)realloc(buff_, sizeof(uint8_t)*ncap);
                buff_ = ndata;
                capacity_ = ncap;
            }
        }

        template<typename T>
        T Read() {
            T data = Read<T>(rpos_);
            rpos_ += sizeof(T);
            return data;
        }

        template<typename T>
        T Read(uintptr_t idx){
            if(idx + sizeof(T) > capacity_) return 0;
            return *((T*)&buff_[idx]);
        }

        template<typename T>
        void Append(T data){
            if((wpos_ + sizeof(T)) > capacity_){
                Resize(capacity_ + sizeof(data));
            }
            memcpy(&buff_[wpos_], (uint8_t*)&data, sizeof(T));
            wpos_ += sizeof(T);
        }

        template<typename T>
        void Insert(T data, uintptr_t idx){
            if((idx + sizeof(T)) > capacity_) return;
            memcpy(&buff_[idx], (uint8_t*)&data, sizeof(T));
            wpos_ = idx + sizeof(T);
        }
    public:
        ByteBuffer(uintptr_t capacity):
            buff_(nullptr),
            capacity_(0),
            wpos_(0),
            rpos_(0){
            if(capacity > 0){
                capacity = RoundUpPowTwo(capacity);
                LOG(INFO) << "creating buffer of size: " << capacity;
                buff_ = (uint8_t*)malloc(sizeof(uint8_t)*capacity);
                capacity_ = capacity;
            }
        }
        ByteBuffer(uint8_t* bytes, uintptr_t capacity):
            ByteBuffer(capacity){
            PutBytes(bytes, capacity);
        }
        ~ByteBuffer(){
            if(buff_) free(buff_);
        }

        void PutBytes(uint8_t* bytes, size_t len){
            if((wpos_ + len) > capacity_) Resize(capacity_ + len);
            memcpy(&buff_[wpos_], bytes, len);
            wpos_ += len;
        }

        void PutBytes(uint8_t* bytes, size_t len, uintptr_t idx){
            if((idx + len) > capacity_) Resize(capacity_ + len);
            memcpy(&buff_[idx], bytes, len);
            wpos_ = idx + len;
        }

        void PutBytes(ByteBuffer* bb){
            if((wpos_ + bb->GetWrittenBytes()) > capacity_) Resize(capacity_ + bb->GetWrittenBytes());
            memcpy(&buff_[wpos_], bb->buff_, bb->GetWrittenBytes());
            wpos_ += bb->GetWrittenBytes();
        }

        void GetBytes(uint8_t* bytes, size_t len){
            if((rpos_ + len) > capacity_) {
                memset(bytes, 0, len);
                return;
            }
            memcpy(bytes, &buff_[rpos_], len);
            rpos_ += len;
        }

        void GetBytes(uint8_t* bytes, size_t len, uintptr_t idx){
            if((idx + len) > capacity_){
                memset(bytes, 0, len);
                return;
            }
            memcpy(bytes, &buff_[idx], len);
            rpos_ = idx + len;
        }

        void PutByte(uint8_t val){
            Append(val);
        }

        void PutByte(uint8_t val, uintptr_t idx){
            Insert(val, idx);
        }

        uint8_t GetByte(){
            return Read<uint8_t>();
        }

        uint8_t GetByte(uintptr_t idx){
            return Read<uint8_t>(idx);
        }

        void PutShort(uint16_t val){
            Append(val);
        }

        void PutShort(uint16_t val, uintptr_t idx){
            Insert(val, idx);
        }

        uint16_t GetShort(){
            return Read<uint16_t>();
        }

        uint16_t GetShort(uintptr_t idx){
            return Read<uint16_t>(idx);
        }

        void PutInt(uint32_t val){
            Append(val);
        }

        void PutInt(uint32_t val, uintptr_t idx){
            Insert(val, idx);
        }

        uint32_t GetInt(){
            return Read<uint32_t>();
        }

        uint32_t GetInt(uintptr_t idx){
            return Read<uint32_t>(idx);
        }

        void PutLong(uint64_t val){
            Append(val);
        }

        void PutLong(uint64_t val, uintptr_t idx){
            Insert(val, idx);
        }

        uint64_t GetLong(){
            return Read<uint64_t>();
        }

        uint64_t GetLong(uintptr_t idx){
            return Read<uint64_t>(idx);
        }

        void PutMessage(Message* msg){
            uint64_t size = msg->GetSize();
            if((wpos_ + size) > capacity_) return;
            uint8_t bytes[size];
            if(!msg->Encode(bytes, size)){
                LOG(ERROR) << "couldn't encode message " << msg->GetName() << " to byte buffer";
                return;
            }
            memcpy(&buff_[wpos_], bytes, size);
            wpos_ += size;
        }

        void PutMessage(Message* msg, uintptr_t idx){
            uint64_t size = msg->GetSize();
            if((idx + size) > capacity_) return;
            uint8_t bytes[size];
            if(!msg->Encode(bytes, size)){
                LOG(ERROR) << "couldn't encode message " << msg->GetName() << " to byte buffer";
                return;
            }
            memcpy(&buff_[idx], bytes, size);
            wpos_ = idx + size;
        }

        uintptr_t GetWrittenBytes() const{
            return wpos_;
        }

        uintptr_t GetCapacity() const{
            return capacity_;
        }

        uint8_t* data(){
            return buff_;
        }

        void clear(){
            memset(buff_, 0, sizeof(uint8_t)*capacity_);
            rpos_ = wpos_ = 0;
        }

        friend ByteBuffer& operator<<(ByteBuffer& buff, uint8_t val) {
            buff.PutByte(val);
            return buff;
        }

        friend ByteBuffer& operator<<(ByteBuffer& buff, uint16_t val){
            buff.PutShort(val);
            return buff;
        }

        friend ByteBuffer& operator<<(ByteBuffer& buff, uint32_t val){
            buff.PutInt(val);
            return buff;
        }

        friend ByteBuffer& operator<<(ByteBuffer& buff, uint64_t val){
            buff.PutLong(val);
            return buff;
        }

        friend ByteBuffer& operator<<(ByteBuffer& buff, Message* val){
            buff.PutMessage(val);
            return buff;
        }
    };
}

#endif //TOKEN_BUFFER_H