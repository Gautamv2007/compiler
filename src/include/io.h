#ifndef TAC_IO_H
#define TAC_IO_H

/**
 * Reads the entire content of a file into a dynamically allocated string.
 */
char* tac_read_file(const char* filename);

/**
 * Writes a string to a file, creating or overwriting it.
 */
void tac_write_file(const char* filename, char* outbuffer);

#endif