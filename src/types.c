#include "include/types.h"
#include <string.h>

int typename_to_int(const char* name)
{
    if (strcmp(name, "int") == 0) return 1;
    if (strcmp(name, "string") == 0) return 2;
    if (strcmp(name, "void") == 0) return 3;
    
    return 0; // Unknown type
}