#ifndef TOKEN_CODEC_DECODER_H
#define TOKEN_CODEC_DECODER_H

#include "type.h"
#include "bitfield.h"

#define DECODED_FIELD_VERBOSITY 1
#define DECODED_FIELD(Name, Type, Value) \
  DVLOG(DECODED_FIELD_VERBOSITY) << "decoded " << #Name << " (" << #Type << "): " << (Value)

#define CANNOT_DECODE_FIELD_SEVERITY FATAL
#define CANNOT_DECODE_FIELD(Name, Type)({ \
  DLOG(CANNOT_DECODE_FIELD_SEVERITY) << "cannot decode " << #Name << " (" << #Type << ")"; \
  return nullptr; \
})

namespace token{
  namespace codec{
    typedef uint64_t DecoderHints;

    class StrictHint: public BitField<DecoderHints, bool, 0, 1>{};
    class ExpectTypeHint: public BitField<DecoderHints, bool, 1, 1>{};
    class ExpectVersionHint: public BitField<DecoderHints, bool, 2, 1>{};
    class ExpectMagicHint: public BitField<DecoderHints, bool, 3, 1>{};

    static const DecoderHints kDefaultDecoderHints = 0;

    class DecoderBase{
    protected:
      const DecoderHints hints_;

      explicit DecoderBase(const DecoderHints& hints):
        hints_(hints){}

      inline const DecoderHints&
      hints() const{
        return hints_;
      }

      static inline bool
      IsValidType(const uint64_t& val){
        NOT_IMPLEMENTED(ERROR);
        return true;//TODO: implement
      }

      inline bool
      DecodeHeader(const BufferPtr& buff, Version& version, Type& type){
        if(ShouldExpectVersion()){
          auto major = buff->GetShort();
          auto minor = buff->GetShort();
          auto revision = buff->GetShort();
          version = Version(major, minor, revision);
          DLOG(INFO) << "decoded version: " << version;
        }
        if(ShouldExpectMagic()){
          auto magic = buff->GetUnsignedShort();
          if(magic != TOKEN_CODEC_MAGIC){
            if(IsStrict()){
              LOG(FATAL) << "cannot decode header, magic is not valid.";
              return false;
            }
            DLOG(WARNING) << "couldn't decode header, magic is not valid.";
          }
          DLOG(INFO) << "decoded magic: " << std::hex << magic;
        }
        if(ShouldExpectType()){
          auto raw_type = buff->GetUnsignedLong();
          if(IsStrict() && !IsValidType(raw_type)){
            LOG(FATAL) << "cannot decode header, type is not valid: " << raw_type;
            return false;
          }
          type = static_cast<Type>(raw_type);
          DLOG(INFO) << "decoded type: " << type;
        }
        return true;
      }
    public:
      DecoderBase(const DecoderBase& rhs) = delete;
      virtual ~DecoderBase() = default;

      bool IsStrict() const{
        return StrictHint::Decode(hints());
      }

      bool ShouldExpectMagic() const{
        return ExpectMagicHint::Decode(hints());
      }

      bool ShouldExpectVersion() const{
        return ExpectVersionHint::Decode(hints());
      }

      bool ShouldExpectType() const{
        return ExpectTypeHint::Decode(hints());
      }

      bool IsSupportedVersion(const Version& version) const{
        DLOG(INFO) << "checking version: " << version << " >= " << Version::CurrentVersion();
        return version >= Version::CurrentVersion();
      }
    };

    template<class T>
    class TypeDecoder : public DecoderBase{
    protected:
      explicit TypeDecoder(const DecoderHints& hints):
        DecoderBase(hints){}
    public:
      ~TypeDecoder() override = default;
      virtual T* Decode(const BufferPtr& data) const = 0;
    };

    template<class T, class C>
    class SetDecoder : public DecoderBase{
    public:
      explicit SetDecoder(const DecoderHints& hints):
        DecoderBase(hints){}
      ~SetDecoder() override = default;

      virtual bool Decode(const internal::BufferPtr& data, std::set<std::shared_ptr<T>, C>& results) const{
        auto length = data->GetLong();
        DECODED_FIELD(length_, Long, length);
        for(auto idx = 0; idx < length; idx++){
          typename T::Decoder decoder(hints());
          T* item = nullptr;
          if(!(item = decoder.Decode(data))){
            DLOG(FATAL) << "cannot decode list item #" << (idx+1) << " from buffer.";
            return false;
          }
          auto value = std::shared_ptr<T>(item);//TODO: fix allocation
          DLOG(INFO) << "decoded list item #" << (idx+1) << ": " << value->ToString();
          results.insert(value);
        }
        return true;
      }
    };

    template<class T, class D>
    class ArrayDecoder : public DecoderBase{
    public:
      typedef std::vector<std::shared_ptr<T>> ArrayType;
    protected:
      explicit ArrayDecoder(const DecoderHints& hints):
        DecoderBase(hints){}
    public:
      ~ArrayDecoder() override = default;

      virtual bool Decode(const internal::BufferPtr& data, ArrayType& results) const{
        //TODO:
        // - better error handling
        // - decode type
        // - decode version
        auto length = data->GetLong();
        DECODED_FIELD(length_, Long, length);
        for(auto idx = 0; idx < length; idx++){
          D decoder(hints());
          T* item = nullptr;
          if(!(item = decoder.Decode(data))){
            DLOG(FATAL) << "cannot decode list item #" << (idx+1) << " from buffer.";
            return false;
          }
          results.push_back(std::shared_ptr<T>(item));//TODO: use make_shared<T>
        }
        return true;
      }
    };

    template<class T, class D>
    class ListDecoder : public DecoderBase{
    public:
      typedef std::vector<std::shared_ptr<T>> ListType;
    protected:
      explicit ListDecoder(const DecoderHints& hints):
        DecoderBase(hints){}
    public:
      ~ListDecoder() override = default;

      virtual bool Decode(const internal::BufferPtr& data, ListType& results) const{
        //TODO:
        // - better error handling
        // - decode type
        // - decode version
        auto length = data->GetLong();
        DECODED_FIELD(length_, Long, length);
        for(auto idx = 0; idx < length; idx++){
          D decoder(hints());
          T* item = nullptr;
          if(!(item = decoder.Decode(data))){
            DLOG(FATAL) << "cannot decode list item #" << (idx+1) << " from buffer.";
            return false;
          }
          results.push_back(std::shared_ptr<T>(item));//TODO: use make_shared<T>
        }
        return true;
      }
    };
  }
}

#endif//TOKEN_CODEC_DECODER_H