#ifndef TOKEN_CODEC_ENCODER_H
#define TOKEN_CODEC_ENCODER_H

#include <set>

#include "type.h"
#include "version.h"
#include "bitfield.h"

namespace token{
  namespace codec{
#define ENCODED_FIELD(Name, Type, Value) \
    DVLOG(1) << "encoded " << #Name << " (" << #Type << "): " << (Value)

    typedef uint8_t EncoderFlags;

    class EncodeTypeFlag : public BitField<EncoderFlags, bool, 0, 1>{};
    class EncodeVersionFlag : public BitField<EncoderFlags, bool, 1, 1>{};
    class EncodeMagicFlag : public BitField<EncoderFlags, bool, 2, 1>{};

    static const EncoderFlags kDefaultEncoderFlags = 0;

    class EncoderBase{
     protected:
      EncoderFlags flags_;

      explicit EncoderBase(const EncoderFlags& flags):
        flags_(flags){}
      EncoderBase(const EncoderBase& other) = default;

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
      virtual ~EncoderBase() = default;

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

      virtual int64_t GetBufferSize() const = 0;
      virtual bool Encode(const internal::BufferPtr& buff) const = 0;

      EncoderBase& operator=(const EncoderBase& other) = default;
    };

    template<class T>
    class TypeEncoder : public EncoderBase{
    protected:
      const T* value_;

      TypeEncoder(const T* value, const codec::EncoderFlags& flags):
        codec::EncoderBase(flags),
        value_(value){}

      inline const T*
      value() const{
        return value_;
      }
    public:
      ~TypeEncoder() override = default;

      int64_t GetBufferSize() const override{
        int64_t size = 0;
        if(codec::EncoderBase::ShouldEncodeMagic())
          size += sizeof(uint16_t);
        if(codec::EncoderBase::ShouldEncodeType())
          size += sizeof(uint64_t);
        if(codec::EncoderBase::ShouldEncodeVersion())
          size += Version::GetSize();
        return size;
      }

      bool Encode(const BufferPtr& buff) const override{
        if(codec::EncoderBase::ShouldEncodeType()){
          const auto& type = value()->type();
          if(!buff->PutUnsignedLong(static_cast<uint64_t>(type))){
            LOG(FATAL) << "couldn't encoder type to buffer.";
            return false;
          }
          ENCODED_FIELD(type_, Type, type);
        }

        if(codec::EncoderBase::ShouldEncodeVersion()){
          const auto& version = Version::CurrentVersion();
          if(!(buff->PutShort(version.major()) && buff->PutShort(version.minor() && buff->PutShort(version.revision())))){
            LOG(FATAL) << "couldn't encoder version to buffer.";
            return false;
          }
          ENCODED_FIELD(version_, Version, version);
        }

        if(codec::EncoderBase::ShouldEncodeMagic()){
          const uint16_t magic = TOKEN_CODEC_MAGIC;
          if(!buff->PutUnsignedShort(magic)){
            LOG(FATAL) << "couldn't encode magic to buffer.";
            return false;
          }
          ENCODED_FIELD(magic_, uint16_t, magic);
        }
        return true;
      }
    };

    template<typename T>
    class ListEncoder : public EncoderBase{
    protected:
      const std::vector<T>& items_;

      ListEncoder(const std::vector<T>& items, const codec::EncoderFlags& flags):
        EncoderBase(flags),
        items_(items){}

      inline const std::vector<T>&
      items() const{
        return items_;
      }
    public:
      ListEncoder(const ListEncoder<T>& rhs) = default;
      ~ListEncoder() override = default;

      int64_t GetBufferSize() const override{
        int64_t size = 0;
        size += sizeof(int64_t); // length
        auto first = items().front();
        typename T::Encoder encoder(&first, flags());
        size += (encoder.GetBufferSize() * items().size());
        return size;
      }

      bool Encode(const BufferPtr& buff) const override{
        auto length = static_cast<int64_t>(items().size());
        if(!buff->PutLong(length)){
          LOG(FATAL) << "couldn't serialize list length to buffer.";
          return false;
        }

        for(auto& item : items()){
          typename T::Encoder encoder(&item, flags());
          if(!encoder.Encode(buff)){
            LOG(FATAL) << "couldn't serialize list item #" << (length - length--) << ": " << item;
            return false;
          }
        }
        return true;
      }

      ListEncoder& operator=(const ListEncoder<T>& rhs) = default;
    };
  }
}

#define CANNOT_ENCODE_CODEC_VERSION(LevelName) \
  LOG(LevelName) << "cannot encoder codec version: " << codec::GetCurrentVersion()

#define CANNOT_ENCODE_VALUE(LevelName, Type, Value) \
  LOG(LevelName) << "cannot encoder " << (Value) << " (" << #Type << ")"

#endif//TOKEN_CODEC_ENCODER_H