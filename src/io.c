#include "include/io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* tac_read_file(const char* filename) {
    const char* ext = strrchr(filename, '.');

    if (!ext || (strcmp(ext, ".gv") != 0 && strcmp(ext, ".txt") != 0)) {
        printf("[Error]: Invalid file format '%s'. Only .gv or .txt files are allowed for compilation.\n", filename);
        exit(1);
    }

    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        printf("Could not open file %s\n", filename);
        exit(1);
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* buffer = (char*) calloc(size + 1, sizeof(char));
    if (!buffer) {
        printf("Could not allocate memory for file %s\n", filename);
        exit(1);
    }

    fread(buffer, 1, size, fp);
    fclose(fp);

    return buffer;
}

void tac_write_file(const char* filename, char* outbuffer) {
    FILE* fp = fopen(filename, "w");
    if (!fp) {
        printf("Could not open file %s for writing\n", filename);
        exit(1);
    }

    fputs(outbuffer, fp);
    fclose(fp);
}
