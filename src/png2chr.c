#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>
#include <stdint.h>

#define TILE_WIDTH 8
#define TILE_HEIGHT 8
#define BYTES_PER_TILE 16
#define CHR_MAX 256
#define NAMETABLE_SIZE 960
#define NAMETABLE_UNKNOWN_INDEX (-1)

typedef struct {
    unsigned char planes[2][8];  // 2 bit-planes de 8 bytes chacun
} Tile;

typedef struct {
    Tile tileset[CHR_MAX];
    int tileset_count;
    int nametable[NAMETABLE_SIZE]; // int during processing and will be converted to unsigned char
    int nametable_index;
    int nametable_uniqueIndex;
    bool with_nametable;
    bool fill_chr_with_empty_tiles;
    int source_tiles_width;
    int source_tiles_height;
    int output_tiles_width; // Multiple of 'output_ratio' to avoid misaligned row
    int output_tiles_height;
    int output_ratio;
    int tile_empty_index;
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
 * Extracts pixels from a 8x8 tile and converts them to NES CHR format
 */
void extract_tile(const png_bytep *row_pointers, int tile_x, int tile_y, Tile *output_tile) {

    int start_x = tile_x * TILE_WIDTH;
    int start_y = tile_y * TILE_HEIGHT;

    for (int py = 0; py < TILE_HEIGHT; py++) {
        const int y = start_y + py;
        unsigned char byte0 = 0, byte1 = 0;

        for (int px = 0; px < TILE_WIDTH; px++) {
            const int x = start_x + px;

            // Retrieve a pixel (palette indexed 0-3)
            unsigned char pixel = row_pointers[y][x];
            pixel = pixel & 0x3;  // keep only 2 bits for color

            // Add the bite in the appropriate byte
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

void process_tile(NES_Screen* nesScreen, Tile new_tile) {
    // Reuse an identical tile if possible and nametable is activated
    if (nesScreen->with_nametable) {
        for (int i = 0; i < nesScreen->tileset_count; i++) {
            if (tile_cmp(&new_tile, &nesScreen->tileset[i])) {
                nesScreen->nametable[nesScreen->nametable_index] = i;
                nesScreen->nametable_index++;
                return;
            }
        }
    }

    // It's a new tile then add it
    if (nesScreen->tileset_count < CHR_MAX) {
        nesScreen->tileset[nesScreen->tileset_count] = new_tile;
        if (nesScreen->with_nametable) {
            nesScreen->nametable[nesScreen->nametable_index] = nesScreen->tileset_count;
            nesScreen->nametable_uniqueIndex++;
            nesScreen->nametable_index++;
        }
        nesScreen->tileset_count++;
    }
}

void fill_row_with_empty_tile(NES_Screen* nesScreen) {
    Tile empty_tile = {0};
    process_tile(nesScreen, empty_tile);
}

// Return default value 0 in case of nothing is detected
void postprocess_detect_empty_tile_index(NES_Screen* nesScreen) {
    if  (nesScreen->tile_empty_index >= 0) {
        return ;
    }

    printf("Processing tileset to detect empty tile index...\n");

    uint64_t test_tile;
    int empty_tile_index = -10;
    for (int i = 0; i < nesScreen->tileset_count; i++) {

        // TODO FIX IT!
        if (i >= (i/nesScreen->output_ratio) * nesScreen->source_tiles_width &&
            i < (i/nesScreen->output_ratio) * nesScreen->output_ratio) {
            continue;
        }

        memcpy(&test_tile, nesScreen->tileset[i].planes[1], sizeof(test_tile));
        if(test_tile == 0x0000000000000000ULL || test_tile == 0xFFFFFFFFFFFFFFFFULL) {
            memcpy(&test_tile, nesScreen->tileset[i].planes[0], sizeof(test_tile));
            if (test_tile == 0x0000000000000000ULL || test_tile == 0xFFFFFFFFFFFFFFFFULL) {
                empty_tile_index = i;
                // Break if tile is filled with color 0 otherwise continue to search
                if (nesScreen->tileset[i].planes[0][0] == 0 && nesScreen->tileset[i].planes[1][0] == 0) {
                    break;
                }
            }
        }
    }

    if (empty_tile_index == -10) {
        nesScreen->tile_empty_index = 0;
        printf("No empty tile index detected, default value is used %d\n", nesScreen->tile_empty_index );
    } else {
        nesScreen->tile_empty_index = empty_tile_index;
        printf("Detected empty tile index %d\n", nesScreen->tile_empty_index );
    }

    for (int i = 0; i < NAMETABLE_SIZE; i++) {
        if (nesScreen->nametable[i] == NAMETABLE_UNKNOWN_INDEX) {
            nesScreen->nametable[i] = nesScreen->tile_empty_index;
        }
    }
}

void preprocess_detect_empty_tile_index(NES_Screen* nesScreen) {
    if (nesScreen->tile_empty_index > 0) {
        printf("Set nametable with empty tile index to %d\n", nesScreen->tile_empty_index);
        memset(nesScreen->nametable, nesScreen->tile_empty_index, sizeof(nesScreen->nametable));
    } else if  (nesScreen->tile_empty_index < 0) {
        memset(nesScreen->nametable, NAMETABLE_UNKNOWN_INDEX, sizeof(nesScreen->nametable));
    } else {
        printf("No empty tile index detected, use default value 0\n");
    }
}

void extract_tile_and_nametable(NES_Screen* nesScreen, const png_bytep *row_pointers) {

    printf("Extracting tiles from source file...\n");
    nesScreen->output_tiles_width =  ((nesScreen->source_tiles_width + (nesScreen->output_ratio-1)) / nesScreen->output_ratio) * nesScreen->output_ratio;
    nesScreen->output_tiles_height = nesScreen->source_tiles_height;

    printf("Output tiles to match a multiple of %d : %ux%u\n",
        nesScreen->output_ratio, nesScreen->output_tiles_width, nesScreen->output_tiles_height);

    preprocess_detect_empty_tile_index(nesScreen);

    for (int ty = 0; ty < nesScreen->source_tiles_height; ty++) {
        for (int tx = 0; tx < nesScreen->source_tiles_width; tx++) {
            Tile tile = {0};
            extract_tile(row_pointers, tx, ty, &tile);
            process_tile(nesScreen, tile);
        }
        // Fill CHR row with empty tiles to be aligned with tile per row (ratio) (first color in the palette 00)
        for (int tx = nesScreen->source_tiles_width; tx < nesScreen->output_tiles_width; tx++) {
            fill_row_with_empty_tile(nesScreen);
        }
    }

    postprocess_detect_empty_tile_index(nesScreen);

    if (nesScreen->tileset_count > CHR_MAX) {
        // Oups! There are too many tiles in the source for a NES :
        fprintf(stderr, "!!> Warning: tileset is full. Some tiles is lost.\n");
    }
}

int write_nesScreen(NES_Screen* nesScreen, const char *chr_path, const char *nametable_path) {

    // Write CHR file
    FILE *chr_file = fopen(chr_path, "wb");
    if (!chr_file) {
        fprintf(stderr, "!!> Error: File open failed for %s\n", chr_path);
        return 1;
    }

    size_t written = fwrite(nesScreen->tileset, sizeof(Tile), nesScreen->tileset_count, chr_file);
    if (written != nesScreen->tileset_count) {
        fprintf(stderr, "!!> Error: Failed to write CHR data to file %s\n", chr_path);
        fclose(chr_file);
        return 1;
    }

    // Fill the CHR bank
    if (nesScreen->fill_chr_with_empty_tiles) {
        int tiles_remaining = CHR_MAX - nesScreen->tileset_count;
        if (tiles_remaining > 0) {
            unsigned char empty_padding[16] = {0}; // First color black
            for (int i = 0; i < tiles_remaining; i++) {
                fwrite(empty_padding, 1, 16, chr_file);
            }
        }
        nesScreen->tileset_count = CHR_MAX;
        fclose(chr_file);
        printf("\n> CHR file saved (%d unique tiles, 4096 octets) : %s\n", nesScreen->tileset_count, chr_path);
    }

    // Write NAM file (nametable)
    if (nesScreen->with_nametable) {
        FILE *nametable_file = fopen(nametable_path, "wb");
        if (!nametable_file) {
            fprintf(stderr, "!!> Error: File open failed for %s\n", nametable_path);
            return 1;
        }

        // Convert nametable from int to unsigned char (int was used to have -1 value until empty tile detection)
        uint8_t oBuff[NAMETABLE_SIZE];
        int count = NAMETABLE_SIZE;
        while (count--) oBuff[count] = (uint8_t)nesScreen->nametable[count];

        written = fwrite(oBuff, 1, NAMETABLE_SIZE, nametable_file);
        if (written != NAMETABLE_SIZE) {
            fprintf(stderr, "!!> Error: Failed to write nametable to file %s\n", nametable_path);
            fclose(nametable_file);
            return 1;
        }
        fclose(nametable_file);
        printf("\n> NAMETABLE file saved (%d bytes) : %s\n", NAMETABLE_SIZE, nametable_path);
    }

    return 0;
}

/**
 * Converts a PNG file to CHR NES format
 */
int png_to_chr(const char *png_path, const char *chr_path, const char *nametable_path,
                const int output_ratio, const int empty_index, const bool fill_chr_with_empty_tiles) {
    // -------------------------
    // -- LOAD PNG FILE AND DATA

    FILE *fp = fopen(png_path, "rb");
    if (!fp) {
        fprintf(stderr, "!!> Error: File open failed for %s\n", png_path);
        return 1;
    }

    // Init libpng
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, nullptr, nullptr);
    if (!png) {
        fprintf(stderr, "!!> Error: unable to create the structure PNG\n");
        fclose(fp);
        return 1;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        fprintf(stderr, "!!> Error: unable to create the structure info PNG\n");
        png_destroy_read_struct(&png, nullptr, nullptr);
        fclose(fp);
        return 1;
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    png_uint_32 width = png_get_image_width(png, info);
    png_uint_32 height = png_get_image_height(png, info);
    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth = png_get_bit_depth(png, info);

    printf("\nConversion: %s -> CHR: %s, NAMETABLE: %s\n", png_path, chr_path, nametable_path);
    printf("Input Dimensions: %ux%u pixels\n", width, height);
    printf("Color Depth: %d bits\n", bit_depth);

    // Check size of source file
    if (width % TILE_WIDTH != 0 || height % TILE_HEIGHT != 0) {
        fprintf(stderr, "!!> Error: PNG size not valid (%ux%u). Must be multiple de 8x8\n",
                width, height);
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        return 1;
    }

    // Check colors profile
    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_RGB_ALPHA) {
        fprintf(stderr, "!!> Error: PNG must be indexed color (palette-based), try to fix RGB color to grayscale\n");
        png_set_rgb_to_gray_fixed(png, 1, -1, -1);
    }
    if (bit_depth == 16) {
        fprintf(stderr, "!!> Error: PNG must be 8 bits per pixel, try to fix 16 bits per pixels to 8 bits per pixel\n");
        png_set_strip_16(png);
    }
    if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
        fprintf(stderr, "!!> Error: PNG must be indexed color (palette-based), try to fix grayscale alpha to grayscale\n");
        png_set_strip_alpha(png);
    }
    if (bit_depth < 8) {
        fprintf(stderr, "!!> Error: PNG must be 8 bits per pixel, try to fix %d bits per pixels to 8 bits per pixel\n", bit_depth);
        png_set_packing(png);
    }
    png_read_update_info(png, info);

    // Memory allocation
    png_bytep *row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
    if (!row_pointers) {
        fprintf(stderr, "!!> Error: Memory allocation failed\n");
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        return 1;
    }

    for (png_uint_32 y = 0; y < height; y++) {
        row_pointers[y] = (png_byte *)malloc(png_get_rowbytes(png, info));
        if (!row_pointers[y]) {
            fprintf(stderr, "!!> Error: Memory allocation failed for the line %u\n", y);
            for (png_uint_32 i = 0; i < y; i++) {
                free(row_pointers[i]);
            }
            free(row_pointers);
            png_destroy_read_struct(&png, &info, nullptr);
            fclose(fp);
            return 1;
        }
    }

    png_read_image(png, row_pointers);

    int tiles_width = (int) width / TILE_WIDTH;
    int tiles_height = (int) height / TILE_HEIGHT;
    int total_tiles = tiles_width * tiles_height;

    // -- END of LOAD PNG FILE AND DATA
    // --------------------------------

    NES_Screen nesScreen = {0};
    nesScreen.with_nametable = (nametable_path != nullptr && strlen(nametable_path) > 0);
    nesScreen.source_tiles_width = tiles_width;
    nesScreen.source_tiles_height = tiles_height;
    nesScreen.output_ratio = output_ratio;
    nesScreen.tile_empty_index = empty_index;
    nesScreen.fill_chr_with_empty_tiles = fill_chr_with_empty_tiles;

    if (total_tiles > CHR_MAX) {
        if (nesScreen.with_nametable) {
            printf("!!> Before optimization with the nametable: PNG is too big! A CHR file is limited to %d tiles but the source file has %d tiles.\n",
                    CHR_MAX, total_tiles);
        } else {
            printf("!!> PNG file is too big! A CHR file is limited to %d tiles but the source file has %d tiles.\n",
             CHR_MAX, total_tiles);

            free(row_pointers);
            png_destroy_read_struct(&png, &info, nullptr);
            fclose(fp);
            return EXIT_FAILURE;
        }
    }

    extract_tile_and_nametable(&nesScreen, row_pointers);

    // Free memory, row_pointers isn't used for next step:
    for (png_uint_32 y = 0; y < height; y++) {
        free(row_pointers[y]);
    }
    free(row_pointers);
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);


    if (nesScreen.with_nametable) {
        printf("> %d unique tiles successfully converted with nametable unique index %d\n", nesScreen.tileset_count, nesScreen.nametable_uniqueIndex);
        printf("> write output files:\n\t- CHR: %s\n\t- NAMETABLE: %s\n", chr_path, nametable_path);
    } else {
        printf("> %d tiles successfully converted\n", nesScreen.tileset_count);
        printf("> write output files:\n\t- CHR: %s\n", chr_path);
    }

    return write_nesScreen(&nesScreen, chr_path, nametable_path);
}

void help() {
    fprintf(stderr, "Usage: png2chr -i <input.png> -o <output.chr>\n");
    fprintf(stderr, "\t[-n <output.nametable>]\n");
    fprintf(stderr, "\t[-r <output.size ratio: 16 or 32, default=32>]\n");
    fprintf(stderr, "\t[-e <empty-tile.index: auto or index number, default=0>]\n");
    fprintf(stderr, "\t[-f <fill-chr to match 256 tiles: true or false, default=true>]\n");
    fprintf(stderr, "\t[--help]\n");
    fprintf(stderr, "\nRequirements:\n");
    fprintf(stderr, "  - PNG must be indexed color (palette-based)\n");
    fprintf(stderr, "  - Dimensions must be multiples of 8x8 pixels\n");
    fprintf(stderr, "  - Palette colors: 0-3 (2 bits per pixel)\n");
}

int main(int argc, char *argv[]) {

    // Test required arguments :
    char *chr_path = nullptr;
    char *png_path = nullptr;
    char *nametable_path = nullptr;
    int output_ratio = 32;
    int empty_index = 0;
    bool fill_chr_with_empty_tiles = true;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            help();
            return 0;
        }

        if (strcmp(argv[i], "-i") == 0) {
            i++;
            if (i >= argc) {
                fprintf(stderr, "!!> Error: Missing -i <input.png>\n");
                help();
                return 1;
            }
            png_path = argv[i];
        }
        if (strcmp(argv[i], "-o") == 0) {
            i++;
            if (i >= argc) {
                fprintf(stderr, "!!> Error: Missing -o <output.chr>\n");
                help();
                return 1;
            }
            chr_path = argv[i];
        }
        if (strcmp(argv[i], "-n") == 0) {
            i++;
            if (i >= argc) {
                fprintf(stderr, "!!> Error: Missing <output.nametable>\n");
                help();
                return 1;
            }
            nametable_path = argv[i];
        }
        if (strcmp(argv[i], "-r") == 0) {
            i++;
            if (i >= argc) {
                fprintf(stderr, "!!> Error: Missing <output.size ratio: 16 or 32>\n");
                help();
                return 1;
            }
            if (strcmp(argv[i], "16") != 0 && strcmp(argv[i], "32") != 0) {
                fprintf(stderr, "!!> Error: -n <output.size ratio: 16 or 32>\n");
                help();
                return 1;
            }
            output_ratio = atoi(argv[i]);
        }
        if (strcmp(argv[i], "-e") == 0) {
            i++;
            if (i >= argc) {
                fprintf(stderr, "!!> Error: Missing <empty-tile.index: auto or index number>\n");
                help();
                return 1;
            }
            if (strcmp(argv[i], "auto") != 0) {
                char *endptr;
                empty_index = strtol(argv[i], &endptr, 10);
                if (endptr == argv[i] || *endptr != '\0') {
                    fprintf(stderr, "!!> Error: -e <empty-tile.index: auto or index number>\n");
                    help();
                    return 1;
                }
            } else {
                empty_index = -1;
            }
        }
        if (strcmp(argv[i], "-f") == 0) {
            i++;
            if (i >= argc) {
                fprintf(stderr, "!!> Error: Missing <fill-chr to match 256 tiles: true or false>\n");
            }
            if (strcmp(argv[i], "true") == 0) {
                fill_chr_with_empty_tiles = true;
            } else if (strcmp(argv[i], "false") == 0) {
                fill_chr_with_empty_tiles = false;
            } else {
                fprintf(stderr, "!!> Error: -f <fill-chr to match 256 tiles: true or false>\n");
            }
        }
    }

    if (png_path == nullptr || chr_path == nullptr) {
        fprintf(stderr, "!!> Error: Missing required arguments -i <input.png> -o <output.chr>\n");
        help();
        return 1;
    }

    return png_to_chr(png_path, chr_path, nametable_path, output_ratio, empty_index, fill_chr_with_empty_tiles);
}
