#include "decompress/by_header.h"

#include <stddef.h>
#include <stdint.h>
#include "decompress/identity.h"
#include "gba/bios.h"
#include "mgba.h"

#define CAT1(A, B) A##B
#define CAT(A, B) CAT1(A, B)
#define STR(A) #A

typedef void (*UnCompFn)(const void*, volatile void*);

#define XRAM Vram
#include "decompress/by_header_template.h"
#undef XRAM

#define XRAM Wram
#include "decompress/by_header_template.h"
#undef XRAM
