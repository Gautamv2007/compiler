#include "include/tac.h"
#include "include/lexer.h"
#include "include/parser.h"
#include "include/io.h"
#include "include/as_frontend.h"
#include <stdlib.h>

void tac_compile(char* src)
{
  lexer_T* lexer = init_lexer(src);
  parser_T* parser = init_parser(lexer);
  AST_T* root = parser_parse(parser);

  char* s = as_f_root(root);
  printf("%s\n", s);
}

void tac_compile_file(const char* filename)
{
  char* src = tac_read_file(filename);
  tac_compile(src);
  free(src);
}
