#ifndef TOKEN_CODEC_DECODER_H
#define TOKEN_CODEC_DECODER_H

#include "type.h"
#include "bitfield.h"

namespace token{
  namespace codec{
    typedef uint64_t DecoderHints;

    class StrictHint : public BitField<DecoderHints, bool, 0, 1>{};
    class ExpectTypeHint : public BitField<DecoderHints, bool, 1, 1>{};
    class ExpectVersionHint : public BitField<DecoderHints, bool, 2, 1>{};
    class ExpectMagicHint : public BitField<DecoderHints, bool, 3, 1>{};

    static const DecoderHints kDefaultDecoderHints = 0;

    template<class T>
    class DecoderBase{
     protected:
      typedef DecoderBase<T> BaseType;

      DecoderHints hints_;

      explicit DecoderBase(const DecoderHints& hints):
        hints_(hints){}
      DecoderBase(const DecoderBase& other) = default;

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
      virtual ~DecoderBase() = default;

      DecoderHints& hints(){
        return hints_;
      }

      DecoderHints hints() const{
        return hints_;
      }

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

      virtual bool Decode(const internal::BufferPtr& buff, T& result) const = 0;

      DecoderBase& operator=(const DecoderBase& other) = default;
    };

    template<class T, class E>
   class ListDecoder : public DecoderBase<std::vector<T>>{
    private:
     typedef DecoderBase<std::vector<T>> BaseType;
    protected:
     explicit ListDecoder(const codec::DecoderHints& hints):
      BaseType(hints){}
    public:
     ListDecoder(const ListDecoder& other) = default;
     ~ListDecoder() override = default;

     bool Decode(const internal::BufferPtr& buff, std::vector<T>& results) const override{
       //TODO: better error handling
       //TODO: decode type
       //TODO: decode version

       auto length = buff->GetLong();
       DLOG(INFO) << "decoded InputList length: " << length;

       T next;
       for(auto idx = 0; idx < length; idx++){
         E decoder(BaseType::hints());
         if(!decoder.Decode(buff, next)){
           LOG(FATAL) << "couldn't deserialize list item #" << (idx) << " from buffer.";
           return false;
         }
         DLOG(INFO) << "decoded list item #" << (idx) << ": " << next;
         results.push_back(next);
       }
       return true;
     }

     ListDecoder& operator=(const ListDecoder& other) = default;
   };
  }
}

#define DECODED_FIELD(Name, Type, Value) \
  DVLOG(1) << "decoded " << #Name << " (" << #Type << "): " << (Value)

#endif//TOKEN_CODEC_DECODER_H