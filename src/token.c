#include "include/token.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

token_T* init_token(char* value, int type)
{
  token_T* token = calloc(1, sizeof(struct TOKEN_STRUCT));
  token->value = value;
  token->type = type;
  return token;
}

const char* token_type_to_str(int type)
{
  switch (type)
  {
    case TOKEN_ID: return "TOKEN_ID";
    case TOKEN_EQUALS: return "TOKEN_EQUALS";
    case TOKEN_LPAREN: return "TOKEN_LPAREN";
    case TOKEN_RPAREN: return "TOKEN_RPAREN";
    case TOKEN_LBRACE: return "TOKEN_LBRACE";
    case TOKEN_RBRACE: return "TOKEN_RBRACE";
    case TOKEN_LBRACKET: return "TOKEN_LBRACKET";
    case TOKEN_RBRACKET: return "TOKEN_RBRACKET";
    case TOKEN_COLON: return "TOKEN_COLON";
    case TOKEN_COMMA: return "TOKEN_COMMA";
    case TOKEN_LT: return "TOKEN_LT";
    case TOKEN_GT: return "TOKEN_GT";
    case TOKEN_ARROW_RIGHT: return "TOKEN_ARROW_RIGHT";
    case TOKEN_INT: return "TOKEN_INT";
    case TOKEN_SEMI: return "TOKEN_SEMI";
    case TOKEN_PLUS: return "TOKEN_PLUS";
    case TOKEN_MINUS: return "TOKEN_MINUS";
    case TOKEN_MUL: return "TOKEN_MUL";
    case TOKEN_DIV: return "TOKEN_DIV";
    case TOKEN_IF: return "TOKEN_IF";
    case TOKEN_ELSE: return "TOKEN_ELSE";
    case TOKEN_WHILE: return "TOKEN_WHILE";
    case TOKEN_RETURN: return "TOKEN_RETURN";
    case TOKEN_EQUALS_EQUALS: return "TOKEN_EQUALS_EQUALS";
    case TOKEN_EOF: return "TOKEN_EOF";
  }
  return "UNKNOWN_TOKEN";
}

char* token_to_str(token_T* token)
{
  const char* type_str = token_type_to_str(token->type);
  const char* template = "<type=`%s`, int_type=`%d`, value=`%s`>";
  // Check for null value to avoid segfaults on EOF
  const char* val = token->value ? token->value : "NULL";

  char* str = calloc(strlen(type_str) + strlen(template) + strlen(val) + 16, sizeof(char));
  sprintf(str, template, type_str, token->type, val);
  return str;
}