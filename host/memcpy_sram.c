#include <stddef.h>
#include <string.h>

void memcpy_to_sram(void* restrict dest, void* restrict src, size_t n) {
	memcpy(dest, src, n);
}
void memcpy_from_sram(void* restrict dest, void* restrict src, size_t n) {
	memcpy(dest, src, n);
}
