#ifndef TOKEN_QRCODE_H
#define TOKEN_QRCODE_H

#include <png.h>
#include <ZXing/BitMatrix.h>
#include <ZXing/ByteMatrix.h>
#include <ZXing/BarcodeFormat.h>
#include <ZXing/TextUtfEncoding.h>
#include <ZXing/MultiFormatWriter.h>
#include <ZXing/ZXStrConvWorkaround.h>

#include "hash.h"
#include "utils/buffer.h"

namespace Token{
#define DEEPLINK "https://tknevents-lab.web.app/scan"
#define DEEPLINK_DEFAULT_FORMAT "QRCode"
#define DEEPLINK_DEFAULT_WIDTH 64
#define DEEPLINK_DEFAULT_HEIGHT 64
#define DEEPLINK_DEFAULT_ECC -1
#define DEEPLINK_MIN_WIDTH 16
#define DEEPLINK_MIN_HEIGHT 16
#define DEEPLINK_MAX_WIDTH 512
#define DEEPLINK_MAX_HEIGHT 512

  typedef ZXing::Matrix<uint8_t> Bitmap;

  class DeeplinkGenerator{
    using Writer = ZXing::MultiFormatWriter;
   private:
    int width_;
    int height_;
    std::string format_;
    Writer writer_;

    Writer& GetWriter(){
      return writer_;
    }

    static inline std::string
    GenerateDeeplinkURL(const Hash& hash){
      std::stringstream ss;
      ss << DEEPLINK << "/" << hash.HexString();
      return ss.str();
    }
   public:
    DeeplinkGenerator(const std::string& format, int width, int height, int ecc_level):
      width_(width),
      height_(height),
      format_(format),
      writer_(Writer(ZXing::BarcodeFormatFromString(format)).setEccLevel(ecc_level)){}
    DeeplinkGenerator(int width, int height):
      DeeplinkGenerator(DEEPLINK_DEFAULT_FORMAT, width, height, DEEPLINK_DEFAULT_ECC){}
    DeeplinkGenerator():
      DeeplinkGenerator(DEEPLINK_DEFAULT_WIDTH, DEEPLINK_DEFAULT_HEIGHT){}
    ~DeeplinkGenerator() = default;

    std::string GetFormat() const{
      return format_;
    }

    int GetWidth() const{
      return width_;
    }

    int GetHeight() const{
      return height_;
    }

    Bitmap Generate(const Hash& hash){
      std::string deeplink = GenerateDeeplinkURL(hash);
      return ZXing::ToMatrix<uint8_t>(GetWriter().encode(ZXing::TextUtfEncoding::FromUtf8(deeplink), GetWidth(), GetHeight()));
    }
  };

  bool WritePNG(const BufferPtr& buff, Bitmap& bitmap);
}

#endif//TOKEN_QRCODE_H