#include <glog/logging.h>
#include "deeplink.h"

namespace token{
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

  bool WritePNG(const BufferPtr& buff, Bitmap& bitmap){
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
}