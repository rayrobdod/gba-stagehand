#include "scene_graphics.h"

#include "gba/bios.h"
#include "gba/palette.h"
#include "gba/vram.h"

struct scene_graphics {
    uint8_t palettes_count;
    palette16_t* palettes;

    uint16_t tileset_count;
    bg_tile_t* tileset;

    uint16_t tilemap_count;
    bg_tile_t* tilemap;
};

struct tileset_graphics {
    uint8_t palettes_count;
    palette16_t* palettes;

    uint16_t tileset_count;
    bg_tile_t* tileset;
};


void load_tileset_graphics(
        struct tileset_graphics* data,
        struct load_tileset_graphics param) {
    CpuFastSet(
        data->palettes,
        &background_palette[param.palette_offset],
        (struct CpuFastSet) {
            .word_count = data->palettes_count * (sizeof(palette16_t) / sizeof(uint32_t)),
            .mode = CPU_SET_COPY,
        });

    CpuFastSet(
        data->tileset,
        &vram.bg_charblock[param.charblock][param.tile_offset],
        (struct CpuFastSet) {
            .word_count = data->tileset_count * (sizeof(tile_4bpp_t) / sizeof(uint32_t)),
            .mode = CPU_SET_COPY,
        });
}
