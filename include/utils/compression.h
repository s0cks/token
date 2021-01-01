#ifndef TOKEN_INCLUDE_UTILS_COMPRESSOR_H
#define TOKEN_INCLUDE_UTILS_COMPRESSOR_H

#include <zlib.h>
#include "utils/buffer.h"

namespace Token{
  static inline bool
  Compress(const BufferPtr& in, const BufferPtr& out){
    z_stream defstream;
    defstream.zalloc = Z_NULL;
    defstream.zfree = Z_NULL;
    defstream.opaque = Z_NULL;
    // setup "a" as the input and "b" as the compressed output
    defstream.avail_in = (uInt) in->GetBufferSize();
    defstream.next_in = (Bytef*) in->data();
    defstream.avail_out = (uInt) out->GetBufferSize(); // size of output
    defstream.next_out = (Bytef*) out->data(); // output char array

    // the actual compression work.
    deflateInit(&defstream, Z_BEST_COMPRESSION);
    deflate(&defstream, Z_FINISH);
    deflateEnd(&defstream);
    return true;
  }

  static inline bool
  Decompress(const BufferPtr& in, const BufferPtr& out){
    z_stream infstream;
    infstream.zalloc = Z_NULL;
    infstream.zfree = Z_NULL;
    infstream.opaque = Z_NULL;
    infstream.avail_in = (uInt) in->GetBufferSize(); // size of input
    infstream.next_in = (Bytef*) in->data(); // input char array
    infstream.avail_out = (uInt) out->GetBufferSize(); // size of output
    infstream.next_out = (Bytef*) out->data(); // output char array

    inflateInit(&infstream);
    inflate(&infstream, Z_NO_FLUSH);
    inflateEnd(&infstream);
    return true;
  }
}

#endif //TOKEN_INCLUDE_UTILS_COMPRESSOR_H