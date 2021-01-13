#ifndef TOKEN_IMAGE_H
#define TOKEN_IMAGE_H

#include <string>
#include <cstdint>
#include "ZXing/BitMatrix.h"
#include "utils/buffer.h"

namespace Token{
  struct Pixel{
    uint8_t red;
    uint8_t green;
    uint8_t blue;

    Pixel():
      red(0),
      green(0),
      blue(0){}
    Pixel(uint8_t r, uint8_t g, uint8_t b):
      red(r),
      green(g),
      blue(b){}

    void operator=(const Pixel& pixel){
      red = pixel.red;
      green = pixel.green;
      blue = pixel.blue;
    }
  };

  struct Bitmap{
    uint8_t (*pixels)[3];
    int64_t width;
    int64_t height;

    Bitmap(int64_t w, int64_t h):
      pixels(new uint8_t[w*h][3]),
      width(w),
      height(h){}
    ~Bitmap(){
      delete[] pixels;
    }

    void Put(int64_t x, int64_t y, uint8_t r, uint8_t g, uint8_t b){
      pixels[y*width+x][0] = r;
      pixels[y*width+x][1] = g;
      pixels[y*width+x][2] = b;
    }
  };

  static const Pixel BLACK;
  static const Pixel WHITE;

  bool WritePNG(const std::string& filename, Bitmap* bitmap);
  bool WritePNG(const std::string& filename, ZXing::Matrix<uint8_t>& bitmap);
  bool WritePNG(const BufferPtr& buff, Bitmap* bitmap);
  bool WritePNG(const BufferPtr& buff, ZXing::Matrix<uint8_t>& bitmap);

  bool ReadPNG(const BufferPtr& buff, Bitmap** bitmap);
  bool ReadPNG(const std::string& filename, Bitmap** bitmap);

  static inline std::string
  ToBinary(Bitmap* bitmap){
    std::stringstream binary;
    for(int y = 0; y < bitmap->height; y++){
      for(int x = 0; x < bitmap->width; x++) {
        uint8_t* px = bitmap->pixels[bitmap->width*y+x];
        if(px[0] == 255 && px[1] == 255 && px[2] == 255){
          binary << '1';
        } else{
          binary << '0';
        }
      }
    }
    return binary.str();
  }
}

#endif //TOKEN_IMAGE_H