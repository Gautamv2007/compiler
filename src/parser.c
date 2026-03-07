#include "include/parser.h"
#include "include/AST.h"
#include "include/types.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

parser_T* init_parser(lexer_T* lexer)
{
  parser_T* parser = calloc(1, sizeof(struct PARSER_STRUCT)); 
  parser->lexer = lexer;
  parser->token = lexer_next_token(lexer);

  return parser;
}

token_T* parser_eat(parser_T* parser, int type)
{
  if (parser->token->type != type)
  {
    printf("[Parser]: Unexpected token: `%s`\n, was expecting: `%s`\n", token_to_str(parser->token), token_type_to_str(type));
    exit(1);
  }

  parser->token = lexer_next_token(parser->lexer);
  return parser->token;
}

AST_T* parser_parse(parser_T* parser)
{
  return parser_parse_compound(parser);
}

AST_T* parser_parse_id(parser_T* parser)
{
  char* value = calloc(strlen(parser->token->value) + 1, sizeof(char));
  strcpy(value, parser->token->value);
  parser_eat(parser, TOKEN_ID);
  
  if(parser->token->type == TOKEN_EQUALS)
  {
    parser_eat(parser, TOKEN_EQUALS);
    AST_T* ast = init_ast(AST_ASSIGNMENT);
    ast->name = value;
    ast->value = parser_parse_expr(parser);
    return ast; 
  }

  AST_T* ast = init_ast(AST_VARIABLE);
  ast->name = value;

  if (parser->token->type == TOKEN_COLON)
  {
    parser_eat(parser, TOKEN_COLON);

    while(parser->token->type == TOKEN_ID)
    {
      ast->data_type = typename_to_int(parser->token->value);
      parser_eat(parser, TOKEN_ID);

      if(parser->token->type == TOKEN_LT)
      {
        parser_eat(parser, TOKEN_LT);
        ast->data_type += typename_to_int(parser->token->value);
        parser_eat(parser, TOKEN_ID);
        parser_eat(parser, TOKEN_GT);
      }
    }
  }
  else
  {
    if (parser->token->type == TOKEN_LPAREN)
    {
      ast->type = AST_CALL;
      ast->children = parser_parse_list(parser);
    }
  }

  return ast;
}

AST_T* parser_parse_block(parser_T* parser)
{
  parser_eat(parser, TOKEN_LBRACE);
  
  AST_T* ast = init_ast(AST_COMPOUND);

  while(parser->token->type != TOKEN_RBRACE)
  {
    list_push(ast->children, parser_parse_expr(parser));
  }

  parser_eat(parser, TOKEN_RBRACE);

  return ast;
}

AST_T* parser_parse_list(parser_T* parser)
{
  parser_eat(parser, TOKEN_LPAREN);
  AST_T* ast = init_ast(AST_COMPOUND);

  list_push(ast->children, parser_parse_expr(parser));

  while(parser->token->type == TOKEN_COMMA)
  {
    parser_eat(parser, TOKEN_COMMA);
    list_push(ast->children, parser_parse_expr(parser));
  }

  parser_eat(parser, TOKEN_RPAREN);

  if (parser->token->type == TOKEN_COLON)
  {
    parser_eat(parser, TOKEN_COLON);

    while(parser->token->type == TOKEN_ID)
    {
      ast->data_type = typename_to_int(parser->token->value);
      parser_eat(parser, TOKEN_ID);

      if(parser->token->type == TOKEN_LT)
      {
        parser_eat(parser, TOKEN_LT);
        ast->data_type += typename_to_int(parser->token->value);
        parser_eat(parser, TOKEN_ID);
        parser_eat(parser, TOKEN_GT);
      }
    }
  }

  if (parser->token->type == TOKEN_ARROW_RIGHT)
  {
    parser_eat(parser, TOKEN_ARROW_RIGHT);
    ast->type = AST_FUNCTION;
    ast->value = parser_parse_block(parser);
  }

  return ast;
}

AST_T* parser_parse_expr(parser_T* parser)
{
  switch (parser->token->type)
  {
    case TOKEN_ID:  return parser_parse_id(parser);
    case TOKEN_LPAREN: return parser_parse_list(parser);
    case TOKEN_INT: return parser_parse_list(parser);
    default: { printf("[Parser]: Unexpected token `%s`\n", token_to_str(parser->token)); exit(1); }
  }
}

AST_T* parser_parse_compound(parser_T* parser)
{
  AST_T* compound = init_ast(AST_COMPOUND); 
  while(parser->token->type != TOKEN_EOF)
  {
    list_push(compound->children, parser_parse_expr(parser));
  }

  return compound;
}
