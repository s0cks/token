#ifndef TOKEN_CODEC_ENCODER_H
#define TOKEN_CODEC_ENCODER_H

#include <set>
#include "object.h"
#include "bitfield.h"

namespace token{
  namespace codec{
    typedef uint16_t EncoderFlags;

    enum EncoderFlagsLayout{
      kEncodeTypeFlagPosition = 0,
      kBitsForEncodeType = 1,

      kEncodeVersionFlagPosition = kEncodeTypeFlagPosition+kBitsForEncodeType,
      kBitsForEncodeVersion = 1,
    };

    class EncodeTypeFlag : public BitField<EncoderFlags, bool, kEncodeTypeFlagPosition, kBitsForEncodeType>{};
    class EncodeVersionFlag : public BitField<EncoderFlags, bool, kEncodeVersionFlagPosition, kBitsForEncodeVersion>{};

    static const EncoderFlags kDefaultEncoderFlags = 0;

    template<class T>
    class EncoderBase{
     protected:
      const T& value_;
      EncoderFlags flags_;

      EncoderBase(const T& value, const EncoderFlags& flags):
          value_(value),
          flags_(flags){}
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
     public:
      virtual ~EncoderBase<T>() = default;

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

      virtual int64_t GetBufferSize() const{
        int64_t size = 0;
        if (ShouldEncodeType())
          size += sizeof(uint64_t);//TODO: check size?
        if (ShouldEncodeVersion())
          size += Version::GetSize();
        return size;
      }

      virtual bool Encode(const BufferPtr& buff) const = 0;

      EncoderBase& operator=(const EncoderBase& other) = default;
    };

    template<class T, class E>
    class ListEncoder : public EncoderBase<std::vector<T>>{
     private:
      typedef EncoderBase<std::vector<T>> BaseType;
     protected:
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

      bool Encode(const BufferPtr& buff) const override{
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
  LOG(LevelName) << "cannot encode codec version: " << codec::GetCurrentVersion()

#define CANNOT_ENCODE_VALUE(LevelName, Type, Value) \
  LOG(LevelName) << "cannot encode " << (Value) << " (" << #Type << ")"

#endif//TOKEN_CODEC_ENCODER_H