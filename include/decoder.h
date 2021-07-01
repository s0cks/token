#ifndef TOKEN_CODEC_DECODER_H
#define TOKEN_CODEC_DECODER_H

#include "codec.h"
#include "object.h"
#include "bitfield.h"

namespace token{
  namespace codec{
    typedef uint16_t DecoderHints;

    enum DecoderHintsLayout{
      kExpectVersionFlagPosition = 0,
      kBitsForExpectVersionFlag = 1,
    };

    class ExpectVersionHint : public BitField<DecoderHints, bool, kExpectVersionFlagPosition, kBitsForExpectVersionFlag>{};

    static const DecoderHints kDefaultDecoderHints = 0;

    template<class T>
    class DecoderBase{
     protected:
      DecoderHints hints_;

      DecoderBase(const DecoderHints& hints):
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

      bool IsSupportedVersion(const Version& version) const{
        DLOG(INFO) << "checking version: " << version << " >= " << codec::GetCurrentVersion();
        return version >= codec::GetCurrentVersion();
      }

      virtual bool Decode(const BufferPtr& buff, T& result) const = 0;

      DecoderBase& operator=(const DecoderBase& other) = default;
    };
  }
}

#define CANNOT_DECODE_CODEC_VERSION(LevelName, Version) \
  LOG(LevelName) << "cannot decode codec version: " << (Version);

#define CANNOT_DECODE_VALUE(LevelName, Name, Type) \
  LOG(LevelName) << "cannot decode " << (#Name) << " (" << (#Type) << ")";

#define CHECK_CODEC_VERSION(LevelName, Buffer) \
  if(ShouldExpectVersion()){\
    Version version = (Buffer)->GetVersion(); \
    if(!IsSupportedVersion(version)){     \
      CANNOT_DECODE_CODEC_VERSION(LevelName, version); \
      return false;         \
    }                       \
    DLOG(INFO) << "decoded version: " << version;  \
  }

#endif//TOKEN_CODEC_DECODER_H