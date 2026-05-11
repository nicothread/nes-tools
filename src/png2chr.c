#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>

#define TILE_WIDTH 8
#define TILE_HEIGHT 8
#define BYTES_PER_TILE 16

typedef struct {
    unsigned char planes[2][8];  // 2 bit-planes de 8 bytes chacun
} Tile;

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
void extract_tile(png_bytep *row_pointers, int tile_x, int tile_y,
                  png_uint_32 png_width, Tile *output_tile) {
    memset(output_tile, 0, sizeof(Tile));

    int start_x = tile_x * TILE_WIDTH;
    int start_y = tile_y * TILE_HEIGHT;

    for (int py = 0; py < TILE_HEIGHT; py++) {
        unsigned char byte0 = 0;
        unsigned char byte1 = 0;

        for (int px = 0; px < TILE_WIDTH; px++) {
            int x = start_x + px;
            int y = start_y + py;

            // Récupère le pixel (assume palette indexée 0-3)
            unsigned char pixel = row_pointers[y][x];
            pixel = pixel & 0x3;  // Garder seulement les 2 bits de couleur

            // Ajoute le bit dans le byte approprié
            byte0 = (byte0 << 1) | (pixel & 1);
            byte1 = (byte1 << 1) | ((pixel >> 1) & 1);
        }

        output_tile->planes[0][py] = byte0;
        output_tile->planes[1][py] = byte1;
    }
}

/**
 * Converts a PNG file to CHR NES format
 */
int png_to_chr(const char *png_path, const char *chr_path) {
    FILE *fp = fopen(png_path, "rb");
    if (!fp) {
        fprintf(stderr, "Error: File open failed for %s\n", png_path);
        return 1;
    }

    // Initialisation libpng
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

    // Validation
    if (width % TILE_WIDTH != 0 || height % TILE_HEIGHT != 0) {
        fprintf(stderr, "Error: PNG size not valid (%ux%u). Must be multiple de 8x8\n",
                width, height);
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return 1;
    }

    // Conversion en indexed color si nécessaire
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

    // Allocation mémoire pour les données
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

    // Lecture des données PNG
    png_read_image(png, row_pointers);

    // Ouverture du fichier CHR de sortie
    FILE *out = fopen(chr_path, "wb");
    if (!out) {
        fprintf(stderr, "Error: creation failed %s\n", chr_path);
        for (png_uint_32 y = 0; y < height; y++) {
            free(row_pointers[y]);
        }
        free(row_pointers);
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return 1;
    }

    // Conversion et écriture des tiles
    int tiles_width = width / TILE_WIDTH;
    int tiles_height = height / TILE_HEIGHT;
    int total_tiles = tiles_width * tiles_height;

    printf("Conversion: %s -> %s\n", png_path, chr_path);
    printf("Dimensions: %ux%u pixels (%dx%d tiles)\n", width, height, tiles_width, tiles_height);

    for (int ty = 0; ty < tiles_height; ty++) {
        for (int tx = 0; tx < tiles_width; tx++) {
            Tile tile;
            extract_tile(row_pointers, tx, ty, width, &tile);

            // Écrit le plan 0 (8 bytes)
            fwrite(tile.planes[0], 1, 8, out);
            // Écrit le plan 1 (8 bytes)
            fwrite(tile.planes[1], 1, 8, out);
        }
    }

    printf("✓ %d tiles successfully converted\n", total_tiles);

    // Nettoyage
    fclose(out);
    fclose(fp);

    for (png_uint_32 y = 0; y < height; y++) {
        free(row_pointers[y]);
    }
    free(row_pointers);
    png_destroy_read_struct(&png, &info, NULL);

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <input.png> <output.chr>\n", argv[0]);
        fprintf(stderr, "\nRequirements:\n");
        fprintf(stderr, "  - PNG must be indexed color (palette-based)\n");
        fprintf(stderr, "  - Dimensions must be multiples of 8x8 pixels\n");
        fprintf(stderr, "  - Palette colors: 0-3 (2 bits per pixel)\n");
        return 1;
    }

    return png_to_chr(argv[1], argv[2]);
}
