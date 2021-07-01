#ifndef TOKEN_CODEC_ENCODER_H
#define TOKEN_CODEC_ENCODER_H

#include <set>
#include "object.h"
#include "bitfield.h"

namespace token{
  namespace codec{
    typedef uint16_t EncoderFlags;

    enum EncoderFlagsLayout{
      kEncodeVersionFlagPosition = 0,
      kBitsForEncodeVersion = 1,

      kEncodeTypeFlagPosition = kEncodeVersionFlagPosition + kBitsForEncodeVersion,
      kBitsForEncodeType = 1,
    };

    class EncodeVersionFlag : public BitField<EncoderFlags, bool, kEncodeVersionFlagPosition, kBitsForEncodeVersion>{};
    class EncodeTypeFlag : public BitField<EncoderFlags, bool, kEncodeTypeFlagPosition, kBitsForEncodeType>{};

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
      virtual bool Encode(const BufferPtr& buff) const = 0;

      const T& value() const{
        return value_;
      }

      EncoderFlags& flags(){
        return flags_;
      }

      EncoderFlags flags() const{
        return flags_;
      }

      bool ShouldEncodeVersion() const{
        return EncodeVersionFlag::Decode(flags());
      }

      bool ShouldEncodeType() const{
        return EncodeTypeFlag::Decode(flags());
      }

      virtual int64_t GetBufferSize() const{
        int64_t size = 0;
        if (ShouldEncodeVersion())
          size += Version::GetSize();
        if (ShouldEncodeType())
          size += sizeof(uint64_t);//TODO: check size?
        return size;
      }

      EncoderBase<T>& operator=(const EncoderBase<T>& other) = default;
    };
  }
}

#define CANNOT_ENCODE_CODEC_VERSION(LevelName) \
  LOG(LevelName) << "cannot encode codec version: " << codec::GetCurrentVersion()

#define CANNOT_ENCODE_VALUE(LevelName, Type, Value) \
  LOG(LevelName) << "cannot encode " << (Value) << " (" << #Type << ")"

#endif//TOKEN_CODEC_ENCODER_H