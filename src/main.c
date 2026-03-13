#include "include/tac.h"
#include <stdio.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Please specify an input file.\nUsage: tac.out <filename.tac>\n");
        return 1;
    }

    tac_compile_file(argv[1]);

    return 0;
}