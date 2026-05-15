#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "reader.h"

#define TILES_X 32
#define TILES_Y 30
#define NAMETABLE_SIZE (TILES_X * TILES_Y) // 960 bytes

// Function to export the nametable as an aligned grid
void export_nametable_grid(const char* nametable_path, const char* output_path, const int tile_per_row) {
    NES_Screen nes_screen = {0};
    read_nametable(&nes_screen, nametable_path);

    int output_size = (TILES_Y * tile_per_row) * 4 + TILES_Y + 1; // 4 == strlen("$FF ") + \n one time per row + last \n;
    int written_size = 0;
    char *output = malloc(output_size);
    if (!output) {
        perror("Failed to allocate memory for nametable grid output");
        return;
    }

    for (int y = 0; y < TILES_Y; y++) {
        int index = 0;
        for (int x = 0; x < tile_per_row; x++) {
            index = (y * TILES_X) + x;
            if (nes_screen.nametable_count <= index) {
                break;
            }
            if ((output_size - written_size - 5) <= 0) {
               output_size += (tile_per_row * 4); // Add one row
                char *temp = realloc(output, output_size);
                if (!temp) {
                    perror("Failed to reallocate memory for nametable grid output");
                    free(output); free(temp);
                    return;
                }
            }
            written_size += snprintf(output + written_size, output_size - written_size, "$%02X ", nes_screen.nametable[index]);
        }
        written_size += snprintf(output + written_size, output_size - written_size,"\n");
        if (nes_screen.nametable_count <= index) {
            break;
        }
    }

    printf(output, written_size);

    // Open output file to save text format of nametable
    if (output_path != nullptr) {
        FILE* fout = fopen(output_path, "w");
        if (fout == NULL) {
            printf("!!> Error: Creation file failed %s\n", output_path);
            free(output);
            return;
        }

        fwrite(output, written_size, 1, fout);
        printf("\n[Success] Nametable text grid saved in file [%s]\n", output_path);
        fclose(fout);
    }

    free(output);
}

void help() {
    fprintf(stderr, "Usage: nametable -i <input.nametable> [-o <output.text>] [-r <input.ratio: 16 or 32>] [--help]\n");
}

int main(int argc, char **argv) {

    // Test required arguments :
    char *nametable_path = nullptr;
    char *ouput_path = nullptr;
    int tile_per_row = 32;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            help();
            return 0;
        }
        if (strcmp(argv[i], "-i") == 0) {
            i++;
            if (i >= argc) {
                fprintf(stderr, "!!> Error: Missing -i <input.nametable>\n");
                help();
                return 1;
            }
            nametable_path = argv[i];
        }
        if (strcmp(argv[i], "-o") == 0) {
            i++;
            if (i >= argc) {
                fprintf(stderr, "!!> Error: Missing -o <output.text>\n");
                help();
                return 1;
            }
            ouput_path = argv[i];
        }
        if (strcmp(argv[i], "-r") == 0) {
            i++;
            if (i >= argc) {
                fprintf(stderr, "!!> Error: Missing -r <input.ratio: 16 or 32>\n");
                help();
                return 1;
            }
            if (strcmp(argv[i], "16") != 0 && strcmp(argv[i], "32") != 0) {
                fprintf(stderr, "!!> Error: Missing <input.ratio: 16 or 32>\n");
                help();
                return 1;
            }
            tile_per_row = atoi(argv[i]);
        }
    }

    if (nametable_path == nullptr) {
        fprintf(stderr, "!!> Error: Missing required arguments\n");
        help();
        return 1;
    }

    export_nametable_grid(nametable_path, ouput_path, tile_per_row);

    return 0;
}