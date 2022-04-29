#include "stdio.h"
#include "stdlib.h"
#include "bitmap.h"

//--------------------------------------------------------------------------------------------------

void* read_file(const char* path, int* size) {
  FILE* file = fopen(path, "r");
  
  if (file == 0) exit(-1);

  fseek(file, 0, SEEK_END);
  int total = ftell(file);
  fseek(file, 0, SEEK_SET);

  void* data = malloc(total);
  fread(data, 1, total, file);
  *size = total;
  return data;
}

//--------------------------------------------------------------------------------------------------

#define max(x, y)            ((x < y) ? y : x)
#define min(x, y)            ((x < y) ? x : y)
#define inside(x, min, max)  ((x < min) ? min : (x > max) ? max : x)

//--------------------------------------------------------------------------------------------------

void apply_blur(void* input, int width, int height, int radius) {
  u32* data = input;
  int size = 2 * radius + 1;

  // First do 1D horizontal blur, then do 1D vertical blur. The sliding window is modifed by adding
  // the newest value and subtracting the oldest value. The buffer hold the result of the horizontal blur
  // and is deleted afterwards.

  u32* buffer = malloc(sizeof(u32) * width * height);

  for (int y = 0; y < height; y++) {
    int red_sum   = 0;
    int green_sum = 0;
    int blue_sum  = 0;

    for (int r = -radius; r <= radius; r++) {
      u32 value = data[y * width + inside(r, 0, width - 1)];

      red_sum   += (value >> 0)  & 0xFF;
      green_sum += (value >> 8)  & 0xFF;
      blue_sum  += (value >> 16) & 0xFF;
    }

    for (int x = 0; x < width; x++) {
      buffer[y * width + x] = 0xFF << 24 | (blue_sum / size) << 16 | (green_sum / size) << 8 | (red_sum / size);

      u32 add = data[y * width + min(x + radius + 1, width - 1)];
      u32 sub = data[y * width + max(x - radius, 0)];

      red_sum += (add >> 0) & 0xFF;
      red_sum -= (sub >> 0) & 0xFF;

      green_sum += (add >> 8) & 0xFF;
      green_sum -= (sub >> 8) & 0xFF;

      blue_sum += (add >> 16) & 0xFF;
      blue_sum -= (sub >> 16) & 0xFF;
    }
  }

  for (int x = 0; x < width; x++) {
    int red_sum   = 0;
    int green_sum = 0;
    int blue_sum  = 0;

    for (int r = -radius; r <= radius; r++) {
      u32 value = buffer[inside(r, 0, height - 1) * width + x];

      red_sum   += (value >> 0)  & 0xFF;
      green_sum += (value >> 8)  & 0xFF;
      blue_sum  += (value >> 16) & 0xFF;
    }

    for (int y = 0; y < height; y++) {
      data[y * width + x] = 0xFF << 24 | (blue_sum / size) << 16 | (green_sum / size) << 8 | (red_sum / size);

      u32 add = buffer[min(y + radius + 1, height - 1) * width + x];
      u32 sub = buffer[max(y - radius, 0) * width + x];

      red_sum += (add >> 0) & 0xFF;
      red_sum -= (sub >> 0) & 0xFF;

      green_sum += (add >> 8) & 0xFF;
      green_sum -= (sub >> 8) & 0xFF;

      blue_sum += (add >> 16) & 0xFF;
      blue_sum -= (sub >> 16) & 0xFF;
    }
  }

  free(buffer);
}

//--------------------------------------------------------------------------------------------------

int main() {
  char* input_path  = "test/fortnite.bmp";
  char* output_path = "test/fortnite_blurred.bmp";

  // Load BMP file.
  int input_size;
  u8* input_data = read_file(input_path, &input_size);

  // Extract raw bitmap.
  int width, height;
  u8* data = bitmap_read_rgba(input_data, input_size, &width, &height);

  // Do some processing.
  apply_blur(data, width, height, 10);

  // Write modified bitmap to new BMP file.
  int output_size;
  u8* output_data = bitmap_write_rgba(data, width, height, &output_size);

  // Save new BMP file.
  FILE* file = fopen(output_path, "w");
  fwrite(output_data, 1, output_size, file);

  return 0;
}