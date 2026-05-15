#ifndef NES_TOOLS_READER_H
#define NES_TOOLS_READER_H
#include <stdint.h>

typedef struct {
    unsigned char *tileset; // 2 Bytes per tile
    int tileset_count; // == tiles number * 2
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
