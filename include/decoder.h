#ifndef TOKEN_CODEC_DECODER_H
#define TOKEN_CODEC_DECODER_H

#include "type.h"
#include "codec.h"
#include "bitfield.h"

namespace token{
  namespace codec{
    typedef uint16_t DecoderHints;

    enum DecoderHintsLayout{
      kExpectTypeFlagPosition = 0,
      kBitsForExpectTypeFlag = 1,

      kExpectVersionFlagPosition = kExpectTypeFlagPosition+kBitsForExpectTypeFlag,
      kBitsForExpectVersionFlag = 1,
    };

    class ExpectTypeHint : public BitField<DecoderHints, bool, kExpectTypeFlagPosition, kBitsForExpectTypeFlag>{};
    class ExpectVersionHint : public BitField<DecoderHints, bool, kExpectVersionFlagPosition, kBitsForExpectVersionFlag>{};

    static const DecoderHints kDefaultDecoderHints = 0;

    template<class T>
    class DecoderBase{
     protected:
      typedef DecoderBase<T> BaseType;

      DecoderHints hints_;

      explicit DecoderBase(const DecoderHints& hints):
        hints_(hints){}
      DecoderBase(const DecoderBase& other) = default;
     public:
      virtual ~DecoderBase() = default;

      DecoderHints& hints(){
        return hints_;
      }

      DecoderHints hints() const{
        return hints_;
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

#endif//TOKEN_CODEC_DECODER_H