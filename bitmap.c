#include "bitmap.h"
#include "stdlib.h"
#include "assert.h"
#include "stdio.h"

//--------------------------------------------------------------------------------------------------

typedef struct __attribute__((packed)) {
  u16 magic_number;
  u32 total_size;
  u32 reserved;
  u32 data_offset;
} FileHeader;

//--------------------------------------------------------------------------------------------------

typedef struct __attribute__((packed)) {
  u32 header_size;
  u32 image_width;
  u32 image_height;
  u16 color_plane_count;
  u16 bits_per_pixel;
  u32 compression_method;
  u32 image_size;
  u32 horizontal_ppm;
  u32 vertical_ppm;
  u32 color_count_in_palette;
  u32 important_color_count;
  u32 red_mask;
  u32 green_mask;
  u32 blue_mask;
  u32 alpha_mask;
  u32 cs_type;
  u32 endpoints[9];
  u32 gamma_red;
  u32 gamma_green;
  u32 gamma_blue;
  u32 intent;
  u32 profile_data;
  u32 profile_size;
  u32 reserved;
} BitmapInfoHeader;

//--------------------------------------------------------------------------------------------------

void* bitmap_write_rgba(void* data, int width, int height, int* output_size) {
  int pixel_count = width * height;
  int header_size = sizeof(FileHeader) + sizeof(BitmapInfoHeader);
  int total_size  = header_size + pixel_count * 4;

  u8* output = malloc(total_size);

  FileHeader* file_header = (void* )output;
  BitmapInfoHeader* info_header = (void* )(output + sizeof(FileHeader));

  file_header->magic_number = 0x4D42;
  file_header->data_offset  = header_size;
  file_header->total_size   = total_size;
  
  info_header->header_size            = sizeof(BitmapInfoHeader);
  info_header->image_width            = width;
  info_header->image_height           = height;
  info_header->color_plane_count      = 1;
  info_header->bits_per_pixel         = 32;
  info_header->compression_method     = 3;
  info_header->image_size             = pixel_count * 4;
  info_header->horizontal_ppm         = 1000;
  info_header->vertical_ppm           = 1000;
  info_header->color_count_in_palette = 0;
  info_header->important_color_count  = 0;
  info_header->red_mask               = 0x000000FF;
  info_header->green_mask             = 0x0000FF00;
  info_header->blue_mask              = 0x00FF0000;
  info_header->alpha_mask             = 0xFF000000;
  info_header->cs_type                = 0x73524742; // sRGB.
  info_header->gamma_red              = 0;
  info_header->gamma_green            = 0;
  info_header->gamma_blue             = 0;
  info_header->intent                 = 0x00000004; // Picture.
  info_header->profile_data           = 0;
  info_header->profile_size           = 0;

  u32* source = data;
  u32* dest = (void* )(output + header_size);

  for (int i = 0; i < pixel_count; i++) {
    *dest++ = *source++;
  }

  *output_size = total_size;
  return output;
}

//--------------------------------------------------------------------------------------------------

int mask_to_shift(u32 mask) {
  for (int i = 0; i < 32; i++) {
    if (mask & 1) {
      return i;
    }

    mask >>= 1;
  }

  return 0;
}

//--------------------------------------------------------------------------------------------------

void* bitmap_read_rgba(void* data, int size, int* width, int* height) {
  u8* input = data;
  int header_size = sizeof(FileHeader) + sizeof(BitmapInfoHeader);

  assert(size >= header_size);

  FileHeader* file_header = (void* )input;
  BitmapInfoHeader* info_header = (void* )(input + sizeof(FileHeader));

  assert(file_header->magic_number == 0x4D42);

  int compression_method = info_header->compression_method;
  int bits_per_pixel     = info_header->bits_per_pixel;
  int image_width        = info_header->image_width;
  int image_height       = info_header->image_height;
  int data_offset        = file_header->data_offset;
  int total_size         = image_width * image_height;

  // Only support the Windows NT 5.0 header without compression. This is used in most BMP generators.
  assert(info_header->header_size == sizeof(BitmapInfoHeader));
  assert(compression_method == 0 || (compression_method == 3 && bits_per_pixel == 32));
  assert(bits_per_pixel == 24 || bits_per_pixel == 32);

  u32* bitmap = malloc(total_size * sizeof(u32));

  if (bits_per_pixel == 24) {
    u8* source = (u8* )(input + data_offset);
    u32* dest = bitmap;

    // Each row in the pixel array is padded to a multiple of 4 bytes.
    int pad = image_width % 4;

    for (int i = 0; i < image_height; i++) {
      for (int j = 0; j < image_width; j++) {
        u32 tmp = 0xFF000000;

        tmp |= *source++ << 16;
        tmp |= *source++ << 8;
        tmp |= *source++;

        *dest++ = tmp;
      }
      
      source += pad;
    }
  }
  else if (bits_per_pixel == 32) {
    int red_shift;
    int green_shift;
    int blue_shift;
    int alpha_shift;

    u32 red_mask;
    u32 green_mask;
    u32 blue_mask;
    u32 alpha_mask;

    if (compression_method == 3) {
      red_mask   = info_header->red_mask;
      green_mask = info_header->green_mask;
      blue_mask  = info_header->blue_mask;
      alpha_mask = info_header->alpha_mask;

      red_shift   = mask_to_shift(red_mask);
      green_shift = mask_to_shift(green_mask);
      blue_shift  = mask_to_shift(blue_mask);
      alpha_mask  = mask_to_shift(alpha_mask);
    }
    else if (compression_method == 0) {
      red_mask   = 0x000000FF;
      green_mask = 0x0000FF00;
      blue_mask  = 0x00FF0000;
      alpha_mask = 0xFF000000;

      red_shift   = 0;
      green_shift = 8;
      blue_shift  = 16;
      alpha_shift = 24;
    }

    u32* source = (u32* )(input + data_offset);

    for (int i = 0; i < total_size; i++) {
      u32 tmp = source[i];
      bitmap[i] = ((tmp & alpha_mask) >> alpha_shift) << 24 | ((tmp & blue_mask) >> blue_shift) << 16 | ((tmp & green_mask) >> green_shift) << 8 | ((tmp & red_mask) >> red_shift);
    }
  }

  *width = image_width;
  *height = image_height;

  return bitmap;
}
