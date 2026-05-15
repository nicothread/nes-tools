#include "reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../lib/stb_image_write.h"

#define TILE_SIZE 8

uint32_t palette[4] = {
    0xFF000000,
    0xFF555555,
    0xFFAAAAAA,
    0xFFFFFFFF
};

void free_screen(const NES_Screen *screen) {
    free(screen->tileset);
    free(screen->output_pixels);
    free(screen->nametable);
}

void convert_chr_nametable_to_png(NES_Screen *screen) {

    for (int ty = 0; ty < screen->rowsNumber; ty += 1) {

        for (int tx = 0; tx < screen->columnsNumber; tx++) {

            // Get tile index from nametable, each nam_index is pointing to a tile of TILE_SIZExTILE_SIZE pixels
            int nam_index = (ty * screen->columnsNumber) + tx;
            uint8_t tile_index = (screen->with_nametable) ? screen->nametable[nam_index] : nam_index;

            // Draw current tile
            for (int row = 0; row < TILE_SIZE; row++) {
                uint8_t plane1 = screen->tileset[tile_index].planes[0][row];
                uint8_t plane2 = screen->tileset[tile_index].planes[1][row];

                // Draw current tile row
                for (int col = 0; col < TILE_SIZE; col++) {
                    // Construct color index (0 to 3) on 2 bits :
                    // shift left bits to right from the left bit to the right to retrieve MSB in right order
                    int bit_shift = 7 - col;
                    uint8_t color_bit1 = (plane1 >> bit_shift) & 0x01; // apply the shift and a mask to get only the shifted bit.
                    uint8_t color_bit2 = (plane2 >> bit_shift) & 0x01; // apply the shift and a mask to get only the shifted bit.
                    uint8_t color_index = color_bit1 | (color_bit2 << 1); // Color index is recreated with correct bits order.

                    // Pixel position
                    int pixel_x = tx * TILE_SIZE + col;
                    int pixel_y = ty * TILE_SIZE + row;
                    int target_index = pixel_y * screen->widthPixels + pixel_x;

                    // Apply color
                    screen->output_pixels[target_index] = palette[color_index];
                }
            }
        }
    }
}

void help() {
    printf("Usage: ./chr2png -c <input.chr> [-n <input.nametable>] -o <output.png> [-r <input.ratio: 16 or 32>]\n");
}


int chr2png(const char *chr_path, char *png_path, const char *nametable_path, const int input_ratio) {
    NES_Screen screen = {0};
    screen.columnsNumber = input_ratio;
    screen.with_nametable = nametable_path != nullptr;

    // Read CHR file
    if (read_chr(&screen, chr_path)) {
        free_screen(&screen);
        return EXIT_FAILURE;
    }

    // Read nametable
    if (screen.with_nametable)
    if (read_nametable(&screen, nametable_path)) {
        free_screen(&screen);
        return EXIT_FAILURE;
    }

    // ScreeWidth is number of tiles by row * Size of a tile in pixels
    screen.widthPixels = screen.columnsNumber * TILE_SIZE;

    // Try to detect screen height output with input tiles/nametable and columnsNumber
    if (!screen.with_nametable) {
        // ScreenHeight is number of tiles in total ((tilesetCount/(TileSize*2 (format 2BPP))) / number tiles per row ) * Size of a tile in pixels
        screen.heightPixels = (screen.tileset_count / screen.columnsNumber) * TILE_SIZE;
        screen.rowsNumber = screen.tileset_count / screen.columnsNumber;
    } else {
        // ScreenHeight is number of indexes in the nametable divide by number of tiles per row * TILE_SIZE
        screen.heightPixels = (screen.nametable_count / screen.columnsNumber) * TILE_SIZE;
        screen.rowsNumber = (screen.nametable_count / screen.columnsNumber);
    }

    printf("Output size: %ux%u\n", screen.widthPixels, screen.heightPixels);

    // Output memory allocation
    screen.output_pixels = malloc(screen.widthPixels * screen.heightPixels * sizeof(uint32_t));
    if (!screen.output_pixels) {
        perror("Memory allocation issue.");
        free_screen(&screen);
        return EXIT_FAILURE;
    }

    convert_chr_nametable_to_png(&screen);

    // Write pixel memory buffer
    const int success = stbi_write_png(png_path, screen.widthPixels, screen.heightPixels, 4, screen.output_pixels, screen.widthPixels * sizeof(uint32_t));
    free_screen(&screen);

    if (success) {
        printf("Successfully converted  %s\n", png_path);
        return EXIT_SUCCESS;
    }
    fprintf(stderr, "An error occurred during PNG file conversion.\n");
    return EXIT_FAILURE;
}

int main(int argc, char **argv) {

    // Test required arguments :
    char *chr_path = nullptr;
    char *png_path = nullptr;
    char *nametable_path = nullptr;
    int input_ratio = 32;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            help();
            return 0;
        }

        if (strcmp(argv[i], "-c") == 0) {
            i++;
            if (i >= argc) {
                fprintf(stderr, "!!> Error: Missing -i <input.chr>\n");
                help();
                return 1;
            }
            chr_path = argv[i];
        }
        if (strcmp(argv[i], "-o") == 0) {
            i++;
            if (i >= argc) {
                fprintf(stderr, "!!> Error: Missing -o <output.png>\n");
                help();
                return 1;
            }
            png_path = argv[i];
        }
        if (strcmp(argv[i], "-n") == 0) {
            i++;
            if (i >= argc) {
                fprintf(stderr, "!!> Error: Missing <input.nametable>\n");
                help();
                return 1;
            }
            nametable_path = argv[i];
        }
        if (strcmp(argv[i], "-r") == 0) {
            i++;
            if (i >= argc) {
                fprintf(stderr, "!!> Error: Missing <input.ratio: 16 or 32>\n");
                help();
                return 1;
            }
            if (strcmp(argv[i], "16") != 0 && strcmp(argv[i], "32") != 0) {
                fprintf(stderr, "!!> Error: -n <input.ratio: 16 or 32>\n");
                help();
                return 1;
            }
            input_ratio = atoi(argv[i]);
        }
    }

    if (png_path == nullptr || chr_path == nullptr) {
        fprintf(stderr, "!!> Error: Missing required arguments -c <input.chr> -o <output.png>\n");
        help();
        return 1;
    }

    return chr2png(chr_path, png_path, nametable_path, input_ratio);
}
