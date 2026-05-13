#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>

#define TILE_WIDTH 8
#define TILE_HEIGHT 8
#define BYTES_PER_TILE 16
#define CHR_MAX 256

typedef struct {
    unsigned char planes[2][8];  // 2 bit-planes de 8 bytes chacun
} Tile;

typedef struct {
    Tile tileset[256];
    int tileset_count;
    unsigned char nametable[960];
    int nametable_index;
} NES_Screen;


/**
 * Converts a pixel (0-3) into bits for both NES bitplanes
 * Returns two bytes: [plane0, plane1]
 */
void pixel_to_nes_bits(int pixel, unsigned char *bit0, unsigned char *bit1) {
    *bit0 = (pixel & 1) ? 0xFF : 0x00;
    *bit1 = (pixel & 2) ? 0xFF : 0x00;
}

/**
 * Extracts pixels from an 8x8 tile and converts them to NES CHR format
 */
void extract_tile(png_bytep *row_pointers, int tile_x, int tile_y, Tile *output_tile) {
    memset(output_tile, 0, sizeof(Tile));

    int start_x = tile_x * TILE_WIDTH;
    int start_y = tile_y * TILE_HEIGHT;

    for (int py = 0; py < TILE_HEIGHT; py++) {
        unsigned char byte0 = 0;
        unsigned char byte1 = 0;

        for (int px = 0; px < TILE_WIDTH; px++) {
            int x = start_x + px;
            int y = start_y + py;

            // Retrieve a pixel (palette indexed 0-3)
            unsigned char pixel = row_pointers[y][x];
            pixel = pixel & 0x3;  // keep only 2 bits for color

            // Add the bite i the appropriate byte
            byte0 = (byte0 << 1) | (pixel & 1);
            byte1 = (byte1 << 1) | ((pixel >> 1) & 1);
        }

        output_tile->planes[0][py] = byte0;
        output_tile->planes[1][py] = byte1;
    }
}

bool tile_cmp(Tile* t1, Tile* t2) {
    return memcmp(t1, t2, sizeof(Tile)) == 0;
}

void process_tile(NES_Screen* nesScreen, Tile* new_tile) {
    // Reuse an identical tile if possible
    for (int i = 0; i < nesScreen->tileset_count; i++) {
        if (tile_cmp(new_tile, &nesScreen->tileset[i])) {
            nesScreen->nametable[nesScreen->nametable_index] = i;
            nesScreen->nametable_index++;
            return;
        }
    }
    // It's a new tile then add it
    if (nesScreen->tileset_count < 256) {
        nesScreen->tileset[nesScreen->tileset_count] = *new_tile;
        nesScreen->nametable[nesScreen->nametable_index] = nesScreen->tileset_count;
        nesScreen->tileset_count++;
        nesScreen->nametable_index++;
    }
}

NES_Screen extract_tile_and_nametable( png_bytep *row_pointers, int tiles_width, int tiles_height) {
    NES_Screen nesScreen = {0};
    nesScreen.tileset_count = 0;
    nesScreen.tileset[nesScreen.tileset_count] = (Tile){0};
    nesScreen.nametable_index=0;

    for (int ty = 0; ty < tiles_height; ty++) {
        for (int tx = 0; tx < tiles_width; tx++) {

            Tile tile = {0};
            extract_tile(row_pointers, tx, ty, &tile);
            process_tile(&nesScreen, &tile);

            if (nesScreen.tileset_count >= 256) {
                // Oups! There are too many tiles in the source for a NES :
                fprintf(stderr, "!!> Warning: tileset is full. Some tiles will be lost.\n");
                return nesScreen;
            }
        }
    }
    return nesScreen;
}

int write_nesScreen(NES_Screen* nesScreen, const char *chr_path, const char *nametable_path) {

    // Write CHR file
    FILE *chr_file = fopen(chr_path, "wb");
    if (!chr_file) {
        fprintf(stderr, "Error: File open failed for %s\n", chr_path);
        return 1;
    }

    size_t written = fwrite(nesScreen->tileset, sizeof(Tile), nesScreen->tileset_count, chr_file);
    if (written != nesScreen->tileset_count) {
        fprintf(stderr, "Error: Failed to write CHR data to file %s\n", chr_path);
        fclose(chr_file);
        return 1;
    }

    // Fill the bank
    int tiles_remaining = 256 - nesScreen->tileset_count;
    if (tiles_remaining > 0) {
        unsigned char empty_padding[16] = {0}; // First color black
        for (int i = 0; i < tiles_remaining; i++) {
            fwrite(empty_padding, 1, 16, chr_file);
        }
    }
    fclose(chr_file);
    printf("CHR file saved (%d unique tiles, 4096 octets) : %s\n", nesScreen->tileset_count, chr_path);

    // Write NAM file (nametable)
    FILE *nametable_file = fopen(nametable_path, "wb");
    if (!nametable_file) {
        fprintf(stderr, "Error: File open failed for %s\n", nametable_path);
        return 1;
    }
    written = fwrite(nesScreen->nametable, 1, 960, nametable_file);
    if (written != 960) {
        fprintf(stderr, "Error: Failed to write nametable to file %s\n", nametable_path);
        fclose(nametable_file);
        return 1;
    }
    fclose(nametable_file);
    printf("NAMETABLE file saved (%d bytes) : %s\n", 960, nametable_path);

    return 0;
}

/**
 * Converts a PNG file to CHR NES format
 */
int png_to_chr(const char *png_path, const char *chr_path, const char *nametable_path) {
    FILE *fp = fopen(png_path, "rb");
    if (!fp) {
        fprintf(stderr, "Error: File open failed for %s\n", png_path);
        return 1;
    }

    // Init libpng
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png) {
        fprintf(stderr, "Error: unable to create the structure PNG\n");
        fclose(fp);
        return 1;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        fprintf(stderr, "Error: unable to create the structure info PNG\n");
        png_destroy_read_struct(&png, NULL, NULL);
        fclose(fp);
        return 1;
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    png_uint_32 width = png_get_image_width(png, info);
    png_uint_32 height = png_get_image_height(png, info);
    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth = png_get_bit_depth(png, info);

    // Check size of source file
    if (width % TILE_WIDTH != 0 || height % TILE_HEIGHT != 0) {
        fprintf(stderr, "Error: PNG size not valid (%ux%u). Must be multiple de 8x8\n",
                width, height);
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return 1;
    }

    // Check colors profile
    if (color_type == PNG_COLOR_TYPE_RGB) {
        png_set_palette_to_rgb(png);
    }
    if (bit_depth == 16) {
        png_set_strip_16(png);
    }
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
        png_set_expand_gray_1_2_4_to_8(png);
    }

    png_read_update_info(png, info);

    // Memory allocation
    png_bytep *row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
    if (!row_pointers) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return 1;
    }

    for (png_uint_32 y = 0; y < height; y++) {
        row_pointers[y] = (png_byte *)malloc(png_get_rowbytes(png, info));
        if (!row_pointers[y]) {
            fprintf(stderr, "Error: Memory allocation failed for the line %u\n", y);
            for (png_uint_32 i = 0; i < y; i++) {
                free(row_pointers[i]);
            }
            free(row_pointers);
            png_destroy_read_struct(&png, &info, NULL);
            fclose(fp);
            return 1;
        }
    }

    png_read_image(png, row_pointers);

    int tiles_width = width / TILE_WIDTH;
    int tiles_height = height / TILE_HEIGHT;
    int total_tiles = tiles_width * tiles_height;

    printf("Conversion: %s -> %s\n", png_path, chr_path);
    printf("Dimensions: %ux%u pixels (%dx%d tiles)\n", width, height, tiles_width, tiles_height);

    if (total_tiles > CHR_MAX) {
        printf("!!> Before optimization: PNG is too big! A CHR file is limited to %d tiles but the source file has %d tiles.\n",
                CHR_MAX, total_tiles);
    }

    NES_Screen nesScreen = extract_tile_and_nametable(row_pointers, tiles_width, tiles_height);

    // Free memory isn't used for next step:
    for (png_uint_32 y = 0; y < height; y++) {
        free(row_pointers[y]);
    }
    free(row_pointers);
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);


    printf("> %d tiles successfully converted with nametable size %d\n", nesScreen.tileset_count, nesScreen.nametable_index);
    printf("> write output files:\n\t- CHR: %s\n\t- NAMETABLE: %s\n", chr_path, nametable_path);

    return write_nesScreen(&nesScreen, chr_path, nametable_path);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <input.png> <output.chr> <output.nametable>\n", argv[0]);
        fprintf(stderr, "\nRequirements:\n");
        fprintf(stderr, "  - PNG must be indexed color (palette-based)\n");
        fprintf(stderr, "  - Dimensions must be multiples of 8x8 pixels\n");
        fprintf(stderr, "  - Palette colors: 0-3 (2 bits per pixel)\n");
        return 1;
    }

    // Test required arguments :
    if (!argv[1] || !argv[2] || !argv[3]) {
        fprintf(stderr, "Error: Missing input.png or output.chr or output.nametable\n");
        return 1;
    }
    return png_to_chr(argv[1], argv[2], argv[3]);
}
