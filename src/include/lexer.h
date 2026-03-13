#ifndef TAC_LEXER_H
#define TAC_LEXER_H

#include "token.h"
#include <stdio.h>

typedef struct LEXER_STRUCT
{
  char* src;            // The source code string
  size_t src_size;      // Length of source code
  char c;               // Current character being looked at
  unsigned int i;       // Current index in source
} lexer_T;

lexer_T* init_lexer(char* src);

void lexer_advance(lexer_T* lexer);

void lexer_skip_whitespace(lexer_T* lexer);

token_T* lexer_next_token(lexer_T* lexer);

token_T* lexer_parse_id(lexer_T* lexer);

token_T* lexer_parse_int(lexer_T* lexer);

char* lexer_get_char_as_str(lexer_T* lexer);

#endif