#ifndef TOKEN_QRCODE_H
#define TOKEN_QRCODE_H

#include "ZXing//BarcodeFormat.h"
#include "ZXing/BitMatrix.h"
#include "ZXing/ByteMatrix.h"
#include "ZXing/MultiFormatWriter.h"
#include "ZXing/TextUtfEncoding.h"
#include "ZXing/ZXStrConvWorkaround.h"

#include "hash.h"
#include "utils/image.h"

namespace Token{
  static inline bool
  WriteQRCode(const BufferPtr& buff, const Hash& hash){
    using namespace ZXing;
    int width = 256;
    int height = 256;
    std::string text = hash.HexString();

    BarcodeFormat format = BarcodeFormatFromString("QRCode");
    int eccLevel = -1;
    int margin = 10;
    auto writer = ZXing::MultiFormatWriter(format)
      .setMargin(margin)
      .setEccLevel(eccLevel);
    auto bitmap = ToMatrix<uint8_t>(writer.encode(TextUtfEncoding::FromUtf8(text), width, height));
    if(!WritePNG(buff, bitmap)){
      LOG(WARNING) << "couldn't write bitmap";
      return false;
    }
    return true;
  }

  static inline bool
  WriteQRCode(const std::string& filename, const Hash& hash){
    using namespace ZXing;
    int width = 256;
    int height = 256;
    std::string text = hash.HexString();

    BarcodeFormat format = BarcodeFormatFromString("QRCode");
    int eccLevel = -1;
    int margin = 10;
    auto writer = ZXing::MultiFormatWriter(format)
                      .setMargin(margin)
                      .setEccLevel(eccLevel);
    auto bitmap = ToMatrix<uint8_t>(writer.encode(TextUtfEncoding::FromUtf8(text), width, height));
    if(!WritePNG(filename, bitmap)){
      LOG(WARNING) << "couldn't write bitmap";
      return false;
    }
    return true;
  }
}

#endif//TOKEN_QRCODE_H