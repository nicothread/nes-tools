#include "reader.h"
#include <stdio.h>
#include <stdlib.h>

int read_nametable(NES_Screen *screen, const char* nam_path) {
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

int read_chr(NES_Screen *screen, const char *chr_path) {
    FILE *chr_file = fopen(chr_path, "rb");
    if (!chr_file) {
        perror("Failed to open CHR file");
        return 1;
    }

    fseek(chr_file, 0, SEEK_END);
    screen->tileset_bytes_size = ftell(chr_file);
    fseek(chr_file, 0, SEEK_SET);

    screen->tileset = malloc(screen->tileset_bytes_size);
    if (!screen->tileset) {
        perror("Memory allocation issue");
        fclose(chr_file);
        return 1;
    }

    fread(screen->tileset, 1, screen->tileset_bytes_size, chr_file);
    fclose(chr_file);

    // Fix number of tiles
    screen->tileset_count = (int) (screen->tileset_bytes_size / sizeof(Tile));

    return 0;
}