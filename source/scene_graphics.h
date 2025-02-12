#ifndef SCENE_GRAPHICS_H
#define SCENE_GRAPHICS_H

#include <stdint.h>

struct scene_graphics;
struct compressed_scene_graphics;
struct tileset_graphics;
struct compressed_tileset_graphics;

struct load_scene_graphics {

};

struct load_tileset_graphics {
    uint8_t charblock;
    uint8_t palette_offset;
    uint8_t tile_offset;
};

void load_scene_graphics(
    const struct scene_graphics*,
    const struct load_scene_graphics);

void load_tileset_graphics(
    const struct tileset_graphics*,
    const struct load_tileset_graphics);

void load_compressed_scene_graphics(
    const struct compressed_scene_graphics*,
    const struct load_scene_graphics);

void load_compressed_tileset_graphics(
    const struct compressed_tileset_graphics*,
    const struct load_tileset_graphics);

#endif // #ifndef SCENE_GRAPHICS_H
