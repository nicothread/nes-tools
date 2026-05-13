#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Définition pour inclure l'implémentation de stb_image_write
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define SCREEN_WIDTH  256
#define SCREEN_HEIGHT 240
#define TILE_SIZE     8
#define TILES_X       (SCREEN_WIDTH / TILE_SIZE)   // 32
#define TILES_Y       (SCREEN_HEIGHT / TILE_SIZE)  // 30
#define NAMETABLE_SIZE 960                         // 32 * 30

// Palette par défaut à 4 couleurs (Niveaux de gris : Noir, Gris foncé, Gris clair, Blanc)
uint32_t palette[4] = {
    0xFF000000, // Index 0 : Noir (Opaque AARRGGBB en Little Endian: 0xFF pour Alpha)
    0xFF555555, // Index 1 : Gris Foncé
    0xFFAAAAAA, // Index 2 : Gris Clair
    0xFFFFFFFF  // Index 3 : Blanc
};

typedef struct {
    unsigned char *tileset;
    int tileset_count;
    unsigned char *nametable;
    int nametable_count;
    uint32_t *output_pixels;
    int nametable_width;  // used later
    int nametable_height;  // used later
} NES_Screen;

void free_screen(NES_Screen *screen) {
    free(screen->tileset);
    free(screen->output_pixels);
    free(screen->nametable);
}

int read_nametable(NES_Screen *screen, char* nam_path) {
    FILE *nam_file = fopen(nam_path, "rb");
    if (!nam_file) {
        perror("Failed to open Nametable file");
        return 1;
    }

    fseek(nam_file, 0, SEEK_END);
    long file_size = ftell(nam_file);
    fseek(nam_file, 0, SEEK_SET);

    screen->nametable = malloc(file_size);
    if (!screen->nametable) {
        perror("Memory allocation issue");
        fclose(nam_file);
        return 1;
    }

    screen->nametable_count = (int) fread(screen->nametable, 1, file_size, nam_file);
    fclose(nam_file);
    printf("> Nametable size %d octets.\n", screen->nametable_count);

    return 0;
}

int read_chr(NES_Screen *screen, char *chr_path) {
    FILE *chr_file = fopen(chr_path, "rb");
    if (!chr_file) {
        perror("Failed to open CHR file");
        return 1;
    }

    fseek(chr_file, 0, SEEK_END);
    screen->tileset_count = (int) ftell(chr_file);
    fseek(chr_file, 0, SEEK_SET);

    screen->tileset = malloc(screen->tileset_count);
    if (!screen->tileset) {
        perror("Memory allocation issue");
        fclose(chr_file);
        return 1;
    }

    fread(screen->tileset, 1, screen->tileset_count, chr_file);
    fclose(chr_file);
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <input.chr> <input.nametable> <output.png>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char* chr_path = argv[1];
    char* nam_path = argv[2];
    char* png_path = argv[3];

    NES_Screen screen = {0};

    // Read CHR file
    if (read_chr(&screen, chr_path)) {
        free_screen(&screen);
        return EXIT_FAILURE;
    }

    // Read nametable
    read_nametable(&screen, nam_path);

    // Output memory allocation
    screen.output_pixels = malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
    if (!screen.output_pixels) {
        perror("Memory allocation issue.");
        free_screen(&screen);
        return EXIT_FAILURE;
    }



    // 5. Rendu de l'écran tuile par tuile
    for (int ty = 0; ty < TILES_Y; ty++) {
        for (int tx = 0; tx < TILES_X; tx++) {

            // Récupération de l'index de la tuile courante dans la Nametable
            int nam_index = ty * TILES_X + tx;
            uint8_t tile_index = screen.nametable[nam_index];

            // Calcul du décalage mémoire dans le fichier CHR (16 octets par tuile)
            int chr_offset = tile_index * 16;

            // Vérification que la tuile demandée ne dépasse pas la taille du fichier CHR fourni
            if (chr_offset + 16 > screen.tileset_count) {
                chr_offset = 0; // Sécurité : pointe vers la tuile 0 si hors limites
            }

            // Dessin des 8 lignes de la tuile courante
            for (int row = 0; row < TILE_SIZE; row++) {
                // Le format CHR de la NES utilise 2 plans séparés de 8 octets pour former la couleur
                uint8_t plane1 = screen.tileset[chr_offset + row];
                uint8_t plane2 = screen.tileset[chr_offset + row + 8];

                // Dessin des 8 pixels de la ligne (de gauche à droite)
                for (int col = 0; col < TILE_SIZE; col++) {
                    // Les bits de poids fort correspondent aux pixels de gauche
                    int bit_shift = 7 - col;

                    // Reconstruction de l'index de couleur (0 à 3) sur 2 bits
                    uint8_t color_bit1 = (plane1 >> bit_shift) & 0x01;
                    uint8_t color_bit2 = (plane2 >> bit_shift) & 0x01;
                    uint8_t color_index = color_bit1 | (color_bit2 << 1);

                    // Calcul de la position absolue du pixel dans le tampon d'image final
                    int pixel_x = tx * TILE_SIZE + col;
                    int pixel_y = ty * TILE_SIZE + row;
                    int target_index = pixel_y * SCREEN_WIDTH + pixel_x;

                    // Application de la couleur de la palette
                    screen.output_pixels[target_index] = palette[color_index];
                }
            }
        }
    }

    // 6. Écriture du tampon de pixels au format PNG
    int success = stbi_write_png(argv[3], SCREEN_WIDTH, SCREEN_HEIGHT, 4, screen.output_pixels, SCREEN_WIDTH * sizeof(uint32_t));

    // Libération des ressources
    free_screen(&screen);

    if (success) {
        printf("Conversion réussie ! Image enregistrée sous : %s\n", argv[3]);
        return EXIT_SUCCESS;
    } else {
        fprintf(stderr, "Erreur lors de la génération du fichier PNG.\n");
        return EXIT_FAILURE;
    }
}
