#ifndef TOKEN_CODEC_ENCODER_H
#define TOKEN_CODEC_ENCODER_H

#include <set>

#include "type.h"
#include "version.h"
#include "bitfield.h"

namespace token{
  namespace codec{
    typedef uint8_t EncoderFlags;

    class EncodeTypeFlag : public BitField<EncoderFlags, bool, 0, 1>{};
    class EncodeVersionFlag : public BitField<EncoderFlags, bool, 1, 1>{};
    class EncodeMagicFlag : public BitField<EncoderFlags, bool, 2, 1>{};

    static const EncoderFlags kDefaultEncoderFlags = 0;

    template<class T>
    class EncoderBase{
     protected:
      typedef EncoderBase<T> BaseType;

      Type type_;
      const T& value_;
      EncoderFlags flags_;

      EncoderBase(const Type& type, const T& value, const codec::EncoderFlags& flags):
        type_(type),
        value_(value),
        flags_(flags){}
      EncoderBase(const T& value, const EncoderFlags& flags):
        EncoderBase(value.type(), value, flags){}
      EncoderBase(const EncoderBase<T>& other) = default;

      template<class V, class E>
      static inline int64_t
      GetBufferSize(const std::vector<V>& list, const codec::EncoderFlags& flags=codec::kDefaultEncoderFlags){
        E encoder(list.front(), flags);

        int64_t size = 0;
        size += sizeof(int64_t);
        size += list.size() * encoder.GetBufferSize();
        return size;
      }

      template<class V, class C, class E>
      static inline int64_t
      GetBufferSize(const std::set<std::shared_ptr<V>, C>& set, const codec::EncoderFlags& flags=codec::kDefaultEncoderFlags){
        int64_t size = 0;
        size += sizeof(int64_t); // length
        for(auto& it : set){
          E encoder(*it, flags);
          size += encoder.GetBufferSize();
        }
        return size;
      }

      inline Type GetObjectType() const{
        return value().type();
      }

      static inline bool
      EncodeType(const BufferPtr& buff, const Type& type){
        return buff->PutUnsignedLong(static_cast<uint64_t>(type));
      }
     public:
      virtual ~EncoderBase<T>() = default;

      Type& type(){
        return type_;
      }

      Type type() const{
        return type_;
      }

      const T& value() const{
        return value_;
      }

      EncoderFlags& flags(){
        return flags_;
      }

      EncoderFlags flags() const{
        return flags_;
      }

      bool ShouldEncodeType() const{
        return EncodeTypeFlag::Decode(flags());
      }

      bool ShouldEncodeVersion() const{
        return EncodeVersionFlag::Decode(flags());
      }

      bool ShouldEncodeMagic() const{
        return EncodeMagicFlag::Decode(flags());
      }

      virtual int64_t GetBufferSize() const{
        int64_t size = 0;
        if(ShouldEncodeVersion())
          size += (sizeof(uint16_t) * 3);
        if(ShouldEncodeMagic())
          size += sizeof(uint16_t);
        if(ShouldEncodeType())
          size += sizeof(uint64_t);
        return size;
      }

      virtual bool Encode(const internal::BufferPtr& buff) const{
        if(ShouldEncodeType()){
          auto& type = type_;
          if(!buff->PutUnsignedLong(static_cast<uint64_t>(type))){
            LOG(FATAL) << "couldn't encoder type to buffer.";
            return false;
          }
        }

        if(ShouldEncodeVersion()){
          const auto& version = Version::CurrentVersion();
          if(!(buff->PutShort(version.major()) && buff->PutShort(version.minor() && buff->PutShort(version.revision())))){
            LOG(FATAL) << "couldn't encoder version to buffer.";
            return false;
          }
        }

        if(ShouldEncodeMagic()){
          const uint16_t magic = TOKEN_CODEC_MAGIC;
          if(!buff->PutUnsignedShort(magic)){
            LOG(FATAL) << "couldn't encode magic to buffer.";
            return false;
          }
        }
        return true;
      }

      EncoderBase& operator=(const EncoderBase& other) = default;
    };

    template<class T, class E>
    class ListEncoder : public EncoderBase<std::vector<T>>{
     private:
      typedef EncoderBase<std::vector<T>> BaseType;
     protected:
      ListEncoder(const Type& type, const std::vector<T>& items, const codec::EncoderFlags& flags):
        BaseType(type, items, flags){}
      ListEncoder(const std::vector<T>& items, const codec::EncoderFlags& flags):
        BaseType(items, flags){}
     public:
      ListEncoder(const ListEncoder<T, E>& other) = default;
      ~ListEncoder() override = default;

      int64_t GetBufferSize() const override{
        auto size = BaseType::GetBufferSize();
        size += sizeof(int64_t); // length
        E encoder(BaseType::value().front(), BaseType::flags());
        size += (encoder.GetBufferSize() * BaseType::value().size());
        return size;
      }

      bool Encode(const internal::BufferPtr& buff) const override{
        auto length = static_cast<int64_t>(BaseType::value().size());
        if(!buff->PutLong(length)){
          LOG(FATAL) << "couldn't serialize list length to buffer.";
          return false;
        }
        auto& items = BaseType::value();
        for(auto& item : items){
          E encoder(item, BaseType::flags());
          if(!encoder.Encode(buff)){
            LOG(FATAL) << "couldn't serialize list item #" << (length - length--) << ": " << item;
            return false;
          }
        }
        return true;
      }

      ListEncoder& operator=(const ListEncoder& other) = default;
    };
  }
}

#define CANNOT_ENCODE_CODEC_VERSION(LevelName) \
  LOG(LevelName) << "cannot encoder codec version: " << codec::GetCurrentVersion()

#define CANNOT_ENCODE_VALUE(LevelName, Type, Value) \
  LOG(LevelName) << "cannot encoder " << (Value) << " (" << #Type << ")"

#endif//TOKEN_CODEC_ENCODER_H