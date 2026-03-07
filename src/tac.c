#include "include/tac.h"
#include "include/lexer.h"
#include "include/parser.h"
#include "include/io.h"
#include <stdlib.h>

void tac_compile(char* src)
{
  lexer_T* lexer = init_lexer(src);
  parser_T* parser = init_parser(lexer);
  AST_T* root = parser_parse(parser);

  printf("%ld\n", root->children->size);

  token_T* tok = 0;

  while((tok = lexer_next_token(lexer))->type != TOKEN_EOF)
  {
    printf("%s\n", token_to_str(tok));
  } 
}

void tac_compile_file(const char* filename)
{
  char* src = tac_read_file(filename);
  tac_compile(src);
  free(src);
}
