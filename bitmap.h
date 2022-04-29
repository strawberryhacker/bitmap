#ifndef BITMAP_H
#define BITMAP_H

//--------------------------------------------------------------------------------------------------

#include "stdint.h"

//--------------------------------------------------------------------------------------------------

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;

//--------------------------------------------------------------------------------------------------

void* bitmap_write_rgba(void* data, int width, int height, int* output_size);
void* bitmap_read_rgba(void* data, int size, int* width, int* height);

#endif
