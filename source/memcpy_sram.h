#ifndef MEMCPY_SRAM_H
#define MEMCPY_SRAM_H

#include <stddef.h>

void memcpy_to_sram(volatile void* restrict dest, const void* restrict src, size_t n);
void memcpy_from_sram(void* restrict dest, volatile void* restrict src, size_t n);

#endif //  #ifndef MEMCPY_SRAM_H
