#ifndef NES_TOOLS_READER_H
#define NES_TOOLS_READER_H
#include <stddef.h>
#include <stdint.h>

typedef struct {
    unsigned char planes[2][8];  // 2 bit-planes de 8 bytes chacun
} Tile;

typedef struct {
    Tile *tileset; // 2 Bytes per tile
    size_t tileset_bytes_size;
    int tileset_count;
    bool with_nametable;
    unsigned char *nametable;
    int nametable_count;
    uint32_t *output_pixels;
    int nametable_width;  // used later
    int nametable_height;  // used later
    int widthPixels;
    int heightPixels;
    int columnsNumber;
    int rowsNumber;
} NES_Screen;

int read_nametable(NES_Screen *screen, const char* nam_path);
int read_chr(NES_Screen *screen, const char *chr_path);

#endif //NES_TOOLS_READER_H
