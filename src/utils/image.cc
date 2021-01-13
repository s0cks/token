#include <png.h>
#include <glog/logging.h>
#include "utils/image.h"
#include "utils/buffer.h"

namespace Token{
  bool ReadPNG(const std::string& filename, Bitmap** bitmap){
    FILE *fp = fopen(filename.data(), "rb");

    png_structp png;
    if(!(png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL))){
      LOG(WARNING) << "couldn't create png read struct.";
      return false;
    }

    png_infop info;
    if(!(info = png_create_info_struct(png))){
      LOG(WARNING) << "couldn't create png info struct.";
      return false;
    }

    if(setjmp(png_jmpbuf(png))){
      LOG(WARNING) << "error occurred reading png file.";
      return false;
    }


    png_init_io(png, fp);
    png_read_info(png, info);
    int64_t width      = png_get_image_width(png, info);
    int64_t height     = png_get_image_height(png, info);
    uint8_t color_type = png_get_color_type(png, info);
    uint8_t bit_depth  = png_get_bit_depth(png, info);

    if(bit_depth == 16)
      png_set_strip_16(png);
    if(color_type == PNG_COLOR_TYPE_PALETTE)
      png_set_palette_to_rgb(png);
    if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
      png_set_expand_gray_1_2_4_to_8(png);
    if(png_get_valid(png, info, PNG_INFO_tRNS))
      png_set_tRNS_to_alpha(png);

    if(color_type == PNG_COLOR_TYPE_RGB ||
       color_type == PNG_COLOR_TYPE_GRAY ||
       color_type == PNG_COLOR_TYPE_PALETTE)
      png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

    if(color_type == PNG_COLOR_TYPE_GRAY ||
       color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
      png_set_gray_to_rgb(png);

    png_read_update_info(png, info);
    png_bytep* row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
    for(int y = 0; y < height; y++) {
      row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));
    }
    png_read_image(png, row_pointers);
    fclose(fp);
    png_destroy_read_struct(&png, &info, NULL);

    if(!(*bitmap = new Bitmap(width, height))){
      LOG(WARNING) << "couldn't create bitmap.";
      return false;
    }

    for(int y = 0; y < height; y++) {
      png_bytep row = row_pointers[y];
      for(int x = 0; x < width; x++) {
        png_bytep px = &(row[x * 4]);
        (*bitmap)->pixels[width*y+x][0] = px[0];
        (*bitmap)->pixels[width*y+x][1] = px[1];
        (*bitmap)->pixels[width*y+x][2] = px[2];
      }
    }
    png_free(png, row_pointers);
    return true;
  }

  struct WriteData{
    BufferPtr buffer;
  };

  static void
  WritePNGData(png_structp png, png_bytep data, size_t length){
    WriteData* write_data = (WriteData*)png_get_io_ptr(png);
    BufferPtr& buffer = write_data->buffer;
    if(!buffer->Resize(length))
      return;
    if(!buffer->PutBytes(data, length)){
      png_error(png, "WriteError");
    }
  }

  void FlushPNGData(png_structp png){}

  bool WritePNG(const BufferPtr& buff, Bitmap* bitmap){
    png_structp png;
    if(!(png = png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp)NULL,NULL,NULL))){
      LOG(WARNING) << "couldn't create png struct.";
      return false;
    }

    png_infop info;
    if(!(info = png_create_info_struct(png))){
      png_destroy_write_struct(&png, NULL);
      LOG(WARNING) << "couldn't create png info struct.";
      return false;
    }

    WriteData data;
    data.buffer = buff;
    png_set_write_fn(png, &data, &WritePNGData, &FlushPNGData);
    png_set_IHDR(png, info,
                 bitmap->width, bitmap->height, // width, height
                 8,                  // bits per pixel -- 16 does not work with blockbuster
                 PNG_COLOR_TYPE_RGB, // non-alpha options are PNG_COLOR_TYPE_RGB,PNG_COLOR_TYPE_GRAY,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png, info);
    png_byte* row_pointers[bitmap->height];
    for (int y = 0; y < bitmap->height; y++)
      row_pointers[y] = (png_byte*)(bitmap->pixels + y*bitmap->width);

    png_write_image(png, row_pointers);

    png_write_end(png, NULL);
    png_destroy_write_struct(&png, &info);
    return true;
  }

  bool WritePNG(const BufferPtr& buff, ZXing::Matrix<uint8_t>& bitmap){
    png_structp png;
    if(!(png = png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp)NULL,NULL,NULL))){
      LOG(WARNING) << "couldn't create png struct.";
      return false;
    }

    png_infop info;
    if(!(info = png_create_info_struct(png))){
      png_destroy_write_struct(&png, NULL);
      LOG(WARNING) << "couldn't create png info struct.";
      return false;
    }

    WriteData data;
    data.buffer = buff;
    png_set_write_fn(png, &data, &WritePNGData, &FlushPNGData);
    png_set_IHDR(png, info,
                 bitmap.width(), bitmap.height(), // width, height
                 8,                  // bits per pixel -- 16 does not work with blockbuster
                 PNG_COLOR_TYPE_GRAY, // non-alpha options are PNG_COLOR_TYPE_RGB,PNG_COLOR_TYPE_GRAY,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png, info);
    png_byte* row_pointers[bitmap.height()];
    for (int y = 0; y < bitmap.height(); y++)
      row_pointers[y] = (png_byte*)(bitmap.data() + y*bitmap.width());

    png_write_image(png, row_pointers);

    png_write_end(png, NULL);
    png_destroy_write_struct(&png, &info);
    return true;
  }

  bool WritePNG(const std::string& filename, ZXing::Matrix<uint8_t>& bitmap){
    FILE* file;
    if(!(file = fopen(filename.data(), "wb"))){
      LOG(WARNING) << "couldn't open png file " << filename;
      return false;
    }

    png_structp png;
    if(!(png = png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp)NULL,NULL,NULL))){
      fclose(file);
      LOG(WARNING) << "couldn't create png struct.";
      return false;
    }

    png_infop info;
    if(!(info = png_create_info_struct(png))){
      png_destroy_write_struct(&png, NULL);
      fclose(file);
      LOG(WARNING) << "couldn't create png info struct.";
      return false;
    }

    png_init_io(png, file);
    png_set_IHDR(png, info,
                 bitmap.width(), bitmap.height(), // width, height
                 8,                  // bits per pixel -- 16 does not work with blockbuster
                 PNG_COLOR_TYPE_GRAY, // non-alpha options are PNG_COLOR_TYPE_RGB,PNG_COLOR_TYPE_GRAY,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png, info);
    png_byte* row_pointers[bitmap.height()];
    for (int y = 0; y < bitmap.height(); y++)
      row_pointers[y] = (png_byte*)(bitmap.data() + y*bitmap.width());

    png_write_image(png, row_pointers);

    png_write_end(png, NULL);
    png_destroy_write_struct(&png, &info);
    fclose(file);
    return true;
  }

  bool WritePNG(const std::string& filename, Bitmap* bitmap){
    FILE* file;
    if(!(file = fopen(filename.data(), "wb"))){
      LOG(WARNING) << "couldn't open png file " << filename;
      return false;
    }

    png_structp png;
    if(!(png = png_create_write_struct(PNG_LIBPNG_VER_STRING, (png_voidp)NULL,NULL,NULL))){
      fclose(file);
      LOG(WARNING) << "couldn't create png struct.";
      return false;
    }

    png_infop info;
    if(!(info = png_create_info_struct(png))){
      png_destroy_write_struct(&png, NULL);
      fclose(file);
      LOG(WARNING) << "couldn't create png info struct.";
      return false;
    }

    png_init_io(png, file);
    png_set_IHDR(png, info,
                 bitmap->width, bitmap->height, // width, height
                 8,                  // bits per pixel -- 16 does not work with blockbuster
                 PNG_COLOR_TYPE_RGB, // non-alpha options are PNG_COLOR_TYPE_RGB,PNG_COLOR_TYPE_GRAY,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);

    png_write_info(png, info);
    png_byte* row_pointers[bitmap->height];
    for (int y = 0; y < bitmap->height; y++)
      row_pointers[y] = (png_byte*)(bitmap->pixels + y*bitmap->width);

    png_write_image(png, row_pointers);

    png_write_end(png, NULL);
    png_destroy_write_struct(&png, &info);
    fclose(file);
    return true;
  }
}