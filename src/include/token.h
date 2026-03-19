#ifndef TAC_TOKEN_H
#define TAC_TOKEN_H

typedef struct TOKEN_STRUCT
{
  enum
  {
    TOKEN_ID,
    TOKEN_EQUALS,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_COLON,
    TOKEN_COMMA,
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_LTE,
    TOKEN_GTE,
    TOKEN_ARROW_RIGHT,
    TOKEN_INT,
    TOKEN_STRING,
    TOKEN_KW_INT,
    TOKEN_KW_STR,
    TOKEN_SEMI,
    // New Tokens for Arithmetic and Loops
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MUL,
    TOKEN_DIV,
    TOKEN_MOD,
    TOKEN_PLUS_EQUALS,
    TOKEN_MINUS_EQUALS,
    TOKEN_MUL_EQUALS,
    TOKEN_DIV_EQUALS,
    TOKEN_MOD_EQUALS,
    TOKEN_EQUALS_EQUALS,
    TOKEN_WHILE,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_RETURN,
    TOKEN_EOF
  } type;

  char* value;
} token_T;

token_T* init_token(char* value, int type);

const char* token_type_to_str(int type);

char* token_to_str(token_T* token);

#endif