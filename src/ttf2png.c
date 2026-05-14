#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#define TILE_WIDTH 8
#define TILE_HEIGHT 8

typedef struct {
    char *chars;
    unsigned long count;
} CharList;

typedef struct {
    unsigned long width;       // amount of character width
    unsigned long height;      // amount of character height
    unsigned long char_width;  // character pixel width
    unsigned long char_height; // character pixel height
    png_uint_32 png_width;
    png_uint_32 png_height;
} GridConfig;

/**
 * Parse command line arguments to extract characters list
 * Format: -chars:ABC... -width:10 -height:5 -char_width:8 -char_height:16 <input.ttf> <output.png>
 */
int parse_args(int argc, char *argv[], char **ttf_path, char **png_path,
               CharList *chars, GridConfig *config) {

    if (argc < 4) {
        fprintf(stderr, "Usage: %s -chars:<characters> -width:<n> -height:<n> -char_width:<n> -char_height:<n> <input.ttf> <output.png>\n", argv[0]);
        fprintf(stderr, "\nExample: %s -chars:ABCDEFG -width:8 -height:2 font.ttf output.png\n", argv[0]);
        return 1;
    }

    chars->chars = nullptr;
    chars->count = 0;
    config->width = 0;
    config->height = 0;
    config->char_width = TILE_WIDTH;
    config->char_height = TILE_HEIGHT;

    for (int i = 1; i < argc; i++) {
        if (strncmp(argv[i], "-chars:", 7) == 0) {
            chars->chars = argv[i] + 7;
            chars->count = strlen(chars->chars);
        } else if (strncmp(argv[i], "-width:", 7) == 0) {
            config->width = strtol(argv[i] + 7, nullptr, 10);
        } else if (strncmp(argv[i], "-height:", 8) == 0) {
            config->height = strtol(argv[i] + 8, nullptr, 10);
        } else if (strncmp(argv[i], "-char_width:", 12) == 0) {
            config->char_width = strtol(argv[i] + 12, nullptr, 10);
        } else if (strncmp(argv[i], "-char_height:", 13) == 0) {
            config->char_height = strtol(argv[i] + 13, nullptr, 10);
        } else if (argv[i][0] != '-') {
            if (*ttf_path == NULL) {
                *ttf_path = argv[i];
            } else {
                *png_path = argv[i];
            }
        }
    }

    if (!chars->chars || chars->count == 0 || config->width <= 0 || config->height <= 0) {
        fprintf(stderr, "!!> Error: Missing or invalid arguments\n");
        return 1;
    }

    if (!(*ttf_path) || !(*png_path)) {
        fprintf(stderr, "!!> Error: Missing input.ttf or output.png\n");
        return 1;
    }

    return 0;
}

/**
 * Create a PNG image with specified dimensions
 */
int create_png_image(const char *output_path, png_uint_32 width, png_uint_32 height,
                     png_bytep **row_pointers) {

    // Allocate row pointers
    *row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
    if (!*row_pointers) {
        fprintf(stderr, "!!> Error: Memory allocation failed for row pointers\n");
        return 1;
    }

    // Allocate each row (indexed color, 1 byte per pixel)
    for (png_uint_32 y = 0; y < height; y++) {
        (*row_pointers)[y] = (png_byte *)calloc(width, sizeof(png_byte));
        if (!(*row_pointers)[y]) {
            fprintf(stderr, "!!> Error: Memory allocation failed for row %u\n", y);
            for (png_uint_32 i = 0; i < y; i++) {
                free((*row_pointers)[i]);
            }
            free(*row_pointers);
            return 1;
        }
    }

    return 0;
}

/**
 * Draw a character bitmap onto the grid at specified position
 */
void draw_char_on_grid(png_bytep *row_pointers, unsigned long grid_x, unsigned long grid_y,
                       FT_Bitmap *bitmap, unsigned long char_width, unsigned long char_height,
                       png_uint_32 png_width, png_uint_32 png_height) {

    unsigned long start_x = grid_x * char_width;
    unsigned long start_y = grid_y * char_height;

    for (int by = 0; by < bitmap->rows && start_y + by < (int)png_height; by++) {
        for (int bx = 0; bx < bitmap->width && start_x + bx < (int)png_width; bx++) {
            unsigned char pixel = bitmap->buffer[by * bitmap->pitch + bx];
            row_pointers[start_y + by][start_x + bx] = pixel>128?1:0;
        }
    }
}

/**
 * Render characters from TTF font to PNG grid
 */
int render_font_to_png(const char *ttf_path, const char *png_path,
                       CharList chars, GridConfig config) {

    FT_Library library;
    FT_Face face;

    // Initialize FreeType
    if (FT_Init_FreeType(&library)) {
        fprintf(stderr, "!!> Error: FreeType initialization failed\n");
        return 1;
    }

    // Load font
    if (FT_New_Face(library, ttf_path, 0, &face)) {
        fprintf(stderr, "!!> Error: Font loading failed for %s\n", ttf_path);
        FT_Done_FreeType(library);
        return 1;
    }

    // Set character size (in pixels * 64)
    FT_Set_Pixel_Sizes(face, 0, config.char_height);

    // Calculate grid dimensions
    config.png_width = config.width * config.char_width;
    config.png_height = config.height * config.char_height;

    // Create PNG image
    png_bytep *row_pointers;
    if (create_png_image(png_path, config.png_width, config.png_height, &row_pointers)) {
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return 1;
    }

    printf("Rendering font to PNG: %s -> %s\n", ttf_path, png_path);
    printf("Grid: %lux%lu (%ux%u pixels) (%lux%lu pixels by tile)\n", config.width, config.height,
           config.png_width, config.png_height, config.char_width, config.char_height);
    printf("Characters: %lu\n", chars.count);

    // Render each character
    for (unsigned long i = 0; i < config.width * config.height; i++) {
        unsigned long grid_x = i % config.width;
        unsigned long grid_y = i / config.width;

        if (i < chars.count) {
            // Render character
            if (FT_Load_Char(face, chars.chars[i], FT_LOAD_RENDER) ||
                FT_Render_Glyph(face->glyph, FT_RENDER_MODE_MONO)) {
                fprintf(stderr, "Warning: Failed to load character '%c'\n", chars.chars[i]);
                continue;
            }

            draw_char_on_grid(row_pointers, grid_x, grid_y,
                            &face->glyph->bitmap, config.char_width, config.char_height,
                            config.png_width, config.png_height);
        }
        // else: leave empty tile (already initialized to 0)
    }

    // Write PNG file
    FILE *fp = fopen(png_path, "wb");
    if (!fp) {
        fprintf(stderr, "!!> Error: Failed to create output file %s\n", png_path);
        for (png_uint_32 y = 0; y < config.png_height; y++) {
            free(row_pointers[y]);
        }
        free(row_pointers);
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return 1;
    }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, nullptr, nullptr);
    if (!png) {
        fclose(fp);
        fprintf(stderr, "!!> Error: PNG write struct creation failed\n");
        for (png_uint_32 y = 0; y < config.png_height; y++) {
            free(row_pointers[y]);
        }
        free(row_pointers);
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return 1;
    }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_write_struct(&png, nullptr);
        fclose(fp);
        fprintf(stderr, "!!> Error: PNG info struct creation failed\n");
        for (png_uint_32 y = 0; y < config.png_height; y++) {
            free(row_pointers[y]);
        }
        free(row_pointers);
        FT_Done_Face(face);
        FT_Done_FreeType(library);
        return 1;
    }

    png_init_io(png, fp);

    // Set PNG properties for indexed color
    png_set_IHDR(png, info, config.png_width, config.png_height, 8,
                 PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    // Create a simple 2-color palette: black (0) and white (1)
    png_color palette[256];
    memset(palette, 0, sizeof(palette));
    palette[0].red = 0;
    palette[0].green = 0;
    palette[0].blue = 0;
    palette[1].red = 255;
    palette[1].green = 255;
    palette[1].blue = 255;

    png_set_PLTE(png, info, palette, 2);

    png_write_info(png, info);
    png_write_image(png, row_pointers);
    png_write_end(png, nullptr);

    printf("PNG image successfully created\n");

    // Cleanup
    fclose(fp);
    png_destroy_write_struct(&png, &info);

    for (png_uint_32 y = 0; y < config.png_height; y++) {
        free(row_pointers[y]);
    }
    free(row_pointers);

    FT_Done_Face(face);
    FT_Done_FreeType(library);

    return 0;
}

int main(int argc, char *argv[]) {
    char *ttf_path = nullptr;
    char *png_path = nullptr;
    CharList chars;
    GridConfig config;

    if (parse_args(argc, argv, &ttf_path, &png_path, &chars, &config)) {
        return 1;
    }

    return render_font_to_png(ttf_path, png_path, chars, config);
}
