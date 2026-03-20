#include "include/lexer.h"
#include "include/macros.h"
#include "include/token.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

lexer_T* init_lexer(char *src)
{
  lexer_T* lexer = calloc(1, sizeof(struct LEXER_STRUCT));
  lexer->src = src;
  lexer->src_size = strlen(src);
  lexer->i = 0;
  lexer->c = src[lexer->i];
  return lexer;
}

void lexer_advance(lexer_T* lexer)
{
  if (lexer->i < lexer->src_size && lexer->c != '\0')
  {
    lexer->i += 1;
    lexer->c = lexer->src[lexer->i];
  }
} 

char lexer_peek(lexer_T* lexer, int offset)
{
  if (lexer->i + offset >= lexer->src_size) return '\0';
  return lexer->src[lexer->i + offset];
}

token_T* lexer_advance_with(lexer_T* lexer, token_T* token)
{
  lexer_advance(lexer);
  return token;
}

token_T* lexer_advance_current(lexer_T* lexer, int type)
{
  char* value = calloc(2, sizeof(char));
  value[0] = lexer->c;
  value[1] = '\0';
  token_T* token = init_token(value, type);
  lexer_advance(lexer);
  return token;
}

void lexer_skip_whitespace(lexer_T *lexer)
{
  while(lexer->c == 13 || lexer->c == 10 || lexer->c == ' ' || lexer->c == '\t')
  lexer_advance(lexer);
}

token_T* lexer_parse_id(lexer_T* lexer)
{
  char *value = calloc(1, sizeof(char));
  while(isalnum(lexer->c) || lexer->c == '_')
  {
    value = realloc(value, (strlen(value) + 2) * sizeof(char));
    strcat(value, (char[]){lexer->c, 0});
    lexer_advance(lexer);
  }

  if (strcmp(value, "while") == 0) return init_token(value, TOKEN_WHILE);
  if (strcmp(value, "for") == 0) return init_token(value, TOKEN_FOR);
  if (strcmp(value, "if") == 0) return init_token(value, TOKEN_IF);
  if (strcmp(value, "elif") == 0) return init_token(value, TOKEN_ELIF);
  if (strcmp(value, "else") == 0) return init_token(value, TOKEN_ELSE);
  if (strcmp(value, "return") == 0) return init_token(value, TOKEN_RETURN);
  if (strcmp(value, "int") == 0) return init_token(value, TOKEN_KW_INT); 
  if (strcmp(value, "str") == 0) return init_token(value, TOKEN_KW_STR);

  return init_token(value, TOKEN_ID);  
}

token_T* lexer_parse_number(lexer_T* lexer)
{
  char *value = calloc(1, sizeof(char));
  while(isdigit(lexer->c))
  {
    value = realloc(value, (strlen(value) + 2) * sizeof(char));
    strcat(value, (char[]){lexer->c, 0});
    lexer_advance(lexer);
  }
  return init_token(value, TOKEN_INT);
}

token_T* lexer_next_token(lexer_T* lexer)
{
  while(lexer->c != '\0')
  {
    lexer_skip_whitespace(lexer);
    if (lexer->c == '\0') break;

    if (isalpha(lexer->c)) return lexer_parse_id(lexer);
    if (isdigit(lexer->c)) return lexer_parse_number(lexer);

    switch(lexer->c)
    {
      case '=': {
        if (lexer_peek(lexer, 1) == '>') {
          lexer_advance(lexer); 
          return lexer_advance_with(lexer, init_token("=>", TOKEN_ARROW_RIGHT));
        }
        if (lexer_peek(lexer, 1) == '=') {
          lexer_advance(lexer); 
          return lexer_advance_with(lexer, init_token("==", TOKEN_EQUALS_EQUALS));
        }
        return lexer_advance_current(lexer, TOKEN_EQUALS);
      }
      case '(': return lexer_advance_current(lexer, TOKEN_LPAREN);
      case ')': return lexer_advance_current(lexer, TOKEN_RPAREN);
      case '{': return lexer_advance_current(lexer, TOKEN_LBRACE);
      case '}': return lexer_advance_current(lexer, TOKEN_RBRACE);
      case '[': return lexer_advance_current(lexer, TOKEN_LBRACKET);
      case ']': return lexer_advance_current(lexer, TOKEN_RBRACKET);
      case '+': {
        if (lexer_peek(lexer, 1) == '=') {
          lexer_advance(lexer); 
          return lexer_advance_with(lexer, init_token("+=", TOKEN_PLUS_EQUALS));
        }
        return lexer_advance_current(lexer, TOKEN_PLUS);
      }
      case '-': {
        if (lexer_peek(lexer, 1) == '=') {
          lexer_advance(lexer);
          return lexer_advance_with(lexer, init_token("-=", TOKEN_MINUS_EQUALS));
        }
          if (lexer_peek(lexer, 1) == '>') {
              lexer_advance(lexer); 
              return lexer_advance_with(lexer, init_token("->", TOKEN_ARROW_RIGHT));
          }
          return lexer_advance_current(lexer, TOKEN_MINUS);
      }
      case '*': {
          if (lexer_peek(lexer, 1) == '=') {
            lexer_advance(lexer); 
            return lexer_advance_with(lexer, init_token("*=", TOKEN_MUL_EQUALS));
          }
          return lexer_advance_current(lexer, TOKEN_MUL);
      }
      case '/': {
          if (lexer_peek(lexer, 1) == '=') {
            lexer_advance(lexer); 
            return lexer_advance_with(lexer, init_token("/=", TOKEN_DIV_EQUALS));
          }
          return lexer_advance_current(lexer, TOKEN_DIV);
      }
      case '%': {
          if (lexer_peek(lexer, 1) == '=') {
            lexer_advance(lexer); 
            return lexer_advance_with(lexer, init_token("%=", TOKEN_MOD_EQUALS));
          }
          return lexer_advance_current(lexer, TOKEN_MOD);
      }
      case '<': {
          if (lexer_peek(lexer, 1) == '=') {
              lexer_advance(lexer);
              return lexer_advance_with(lexer, init_token("<=", TOKEN_LTE));
          }
          return lexer_advance_current(lexer, TOKEN_LT);
      }
      case '>': {
          if (lexer_peek(lexer, 1) == '=') {
              lexer_advance(lexer);
              return lexer_advance_with(lexer, init_token(">=", TOKEN_GTE));
          }
          return lexer_advance_current(lexer, TOKEN_GT);
      }
      case '&':
          if (lexer_peek(lexer, 1) == '&') {
              lexer_advance(lexer);
              lexer_advance(lexer); 
              return init_token("&&", TOKEN_AND); 
          }
          break;

      case '|':
          if (lexer_peek(lexer, 1) == '|') {
              lexer_advance(lexer);
              lexer_advance(lexer); 
              return init_token("||", TOKEN_OR);
          }
          break;
      case '!': {
          if (lexer_peek(lexer, 1) == '=') {
              lexer_advance(lexer);
              lexer_advance(lexer);
              return init_token("!=", TOKEN_NOT_EQUALS);
          }
          lexer_advance(lexer); 
          return init_token("!", TOKEN_BANG); 
      }
      case ':': return lexer_advance_current(lexer, TOKEN_COLON);
      case ',': return lexer_advance_current(lexer, TOKEN_COMMA);
      case ';': return lexer_advance_current(lexer, TOKEN_SEMI);
      case '"': return lexer_collect_string(lexer);
      case '\'': {
          lexer_advance(lexer);      // Skip the opening quote
          char char_val = lexer->c;  // Grab the actual letter (e.g., 'J')
          lexer_advance(lexer);      // Move past the letter
            
          if (lexer->c == '\'') {
              lexer_advance(lexer);  // Skip the closing quote
          }

            // Convert the character into a string so the token can hold it
          char* val_str = calloc(2, sizeof(char));
          val_str[0] = char_val;
          val_str[1] = '\0';
            
          return init_token(val_str, TOKEN_CHAR);
      }
      default: printf("[Lexer]: Unexpected character `%c`\n", lexer->c); exit(1);
    }
  }
  return init_token(0, TOKEN_EOF);
}

char* lexer_get_char_as_str(lexer_T* lexer) {
    char* s = calloc(2, sizeof(char));
    s[0] = lexer->c;
    s[1] = '\0';
    return s;
}

token_T* lexer_collect_string(lexer_T* lexer) {
    lexer_advance(lexer); 

    char* value = calloc(1, sizeof(char));
    value[0] = '\0';

    while (lexer->c != '"' && lexer->c != '\0') {
        char* s = lexer_get_char_as_str(lexer); 
        value = realloc(value, (strlen(value) + strlen(s) + 1) * sizeof(char));
        strcat(value, s);
        free(s);
        lexer_advance(lexer);
    }
    lexer_advance(lexer); 

    return init_token(value, TOKEN_STRING); 
}