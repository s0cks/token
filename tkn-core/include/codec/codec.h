#ifndef TOKEN_CODEC_H
#define TOKEN_CODEC_H

#include "buffer.h"

#define TOKEN_CODEC_MAGIC 0xFEFE

#define TOKEN_CODEC_VERSION_MAJOR 0
#define TOKEN_CODEC_VERSION_MINOR 0
#define TOKEN_CODEC_VERSION_REVISION 1

#define CURRENT_CODEC_IS(Major, Minor, Revision) \
  ((TOKEN_CODEC_VERSION_MAJOR == (Major)) && \
    (TOKEN_CODEC_VERSION_MINOR == (Minor)) &&\
    (TOKEN_CODEC_VERSION_REVISION == (Revision)))

#define CURRENT_CODEC_IS_GREATER_THAN(Major, Minor, Revision) \
  ((TOKEN_CODEC_VERSION_MAJOR >= (Major)) &&     \
    (TOKEN_CODEC_VERSION_MINOR >= (Minor)) &&    \
    (TOKEN_CODEC_VERSION_REVISION >= (Revision)))

#define CODEC_NOT_SUPPORTED(LevelName) \
  LOG(LevelName) << "The current codec v" << TOKEN_CODEC_VERSION_MAJOR << "." << TOKEN_CODEC_VERSION_MINOR << "." << TOKEN_CODEC_VERSION_REVISION << " is not supported."

#include "version.h"

namespace token{
  namespace codec{
    Version GetCurrentVersion();
  }
}

#include "codec/encoder.h"
#include "codec/decoder.h"

#endif//TOKEN_CODEC_H