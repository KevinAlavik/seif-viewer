#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>

#include "seif.h"

int main(int argc, char **argv) {
    if(argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "rb");
    if(!file) {
        printf("Could not open file %s\n", argv[1]);
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *buffer = malloc(size);
    fread(buffer, 1, size, file);
    fclose(file);

    SEIF_Header *header = (SEIF_Header *)buffer;
    if(strncmp((char *)header->magic, "SEIF", 4) != 0) {
        printf("Invalid SEIF image! ERROR: Invalid magic\n");
        return 1;
    }

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow(
        "SEIF Image Viewer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        header->meta.width, header->meta.height, 0);

    SDL_Renderer *renderer =
        SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture *texture = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING,
        header->meta.width, header->meta.height);

    printf("[HEADER]:\n");
    printf(" - magic \"%.4s\"\n", header->magic);
    printf(" - flags: 0x%.8x\n", header->flags);
    printf(" - encoding: 0x%.8x\n", header->encoding);
    printf(" - [META]:\n");
    printf("    - signature: %.8s\n", header->meta.signature);
    printf("    - width: %d\n", header->meta.width);
    printf("    - height: %d\n", header->meta.height);
    printf(" - chunk_count: %d\n", header->chunk_count);
    printf(" - chunk_size: %d\n", header->chunk_size);

    uint8_t encoding_size = 0;
    if(header->encoding == SEIF_ENCODING_RGB) {
        encoding_size = 3;
    } else if(header->encoding == SEIF_ENCODING_RGBA ||
              header->encoding == SEIF_ENCODING_ARGB) {
        encoding_size = 4;
    }

    uint8_t **image_data = malloc(header->chunk_count * sizeof(uint8_t *));

    if(image_data == NULL) {
        printf("Could not allocate memory for image data!\n");
        return 1;
    }

    for(int i = 0; i < header->chunk_count; i++) {
        SEIF_ChunkHeader *chunk_header =
            (SEIF_ChunkHeader *)((buffer + sizeof(SEIF_Header)) +
                                 i * header->chunk_size);

        printf(" - [CHUNK %d]:\n", i);
        printf("    - width: %d\n", chunk_header->width);
        printf("    - height: %d\n", chunk_header->height);

        if(header->chunk_count == 1) {
            if(chunk_header->width != header->meta.width &&
               chunk_header->height != header->meta.height) {
                printf("Chunk %d is invalid! ERROR: Chunk size does not match "
                       "the image size!\n",
                       i);
                return 1;
            }
        } else {
            printf("Chunk %d is invalid! ERROR: Multiple chunks are not "
                   "supported yet!\n",
                   i);
            return 1;
        }

        int data_size = header->chunk_size * encoding_size;
        uint8_t *data = malloc(data_size);

        if(data == NULL) {
            printf("Chunk %d is invalid! ERROR: Could not allocate memory for "
                   "data!\n",
                   i);
            return 1;
        }
        memcpy(data,
               (buffer + sizeof(SEIF_Header) + i * sizeof(SEIF_ChunkHeader)),
               data_size);

        image_data[i] = data;
    }

    SDL_RenderClear(renderer);
    for(int i = 0; i < header->chunk_count; i++) {
        uint8_t *data = image_data[i];

        for(int y = 0; y < header->meta.height; y++) {
            for(int x = 0; x < header->meta.width; x++) {
                int index = (y * header->meta.width + x) * encoding_size;

                uint8_t r = 0;
                uint8_t g = 0;
                uint8_t b = 0;
                uint8_t a = 255;

                if(header->encoding == SEIF_ENCODING_RGB) {
                    r = data[index];
                    g = data[index + 1];
                    b = data[index + 2];
                } else if(header->encoding == SEIF_ENCODING_RGBA) {
                    r = data[index];
                    g = data[index + 1];
                    b = data[index + 2];
                    a = data[index + 3];
                } else if(header->encoding == SEIF_ENCODING_ARGB) {
                    r = data[index + 1];
                    g = data[index + 2];
                    b = data[index + 3];
                    a = data[index];
                }

                SDL_SetRenderDrawColor(renderer, r, g, b, a);
                SDL_RenderDrawPoint(renderer, x, y);
            }
        }
    }
    SDL_RenderPresent(renderer);

    bool quit = false;
    SDL_Event event;
    while(!quit) {
        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT) {
                quit = true;
            }
        }
        SDL_Delay(100);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
