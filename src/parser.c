#include "include/parser.h"
#include "include/AST.h"
#include "include/types.h"
#include "include/lexer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//Symbol table for variables (for future use in type checking and code generation)
char* sym_names[1000];
int sym_types[1000];
int sym_count = 0;

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
    printf("[Parser]: Unexpected token: `%s`, was expecting: `%s`\n", token_to_str(parser->token), token_type_to_str(type));
    exit(1);
  }
  parser->token = lexer_next_token(parser->lexer);
  return parser->token;
}

AST_T* parser_parse_factor(parser_T* parser);
AST_T* parser_parse_multiplicative(parser_T* parser);
AST_T* parser_parse_additive(parser_T* parser);
AST_T* parser_parse_comparison(parser_T* parser);

AST_T* parser_parse_factor(parser_T* parser) {
    switch (parser->token->type) {
        case TOKEN_ID:      return parser_parse_id(parser);
        case TOKEN_INT:     return parser_parse_int(parser);

        case TOKEN_MINUS: {
            parser_eat(parser, TOKEN_MINUS);
            
            AST_T* right_node = parser_parse_factor(parser);

            if (right_node->type == AST_INT) {
                right_node->int_value = -right_node->int_value;
                return right_node;
            }

            AST_T* zero_node = init_ast(AST_INT);
            zero_node->int_value = 0;

            AST_T* ast = init_ast(AST_BINOP);
            ast->left = zero_node;
            
            ast->op = calloc(2, sizeof(char));
            strcpy(ast->op, "-");
            
            ast->right = right_node;
            
            return ast;
        }

        case TOKEN_BANG: {
            parser_eat(parser, TOKEN_BANG);
            AST_T* right_node = parser_parse_factor(parser);

            AST_T* zero_node = init_ast(AST_INT);
            zero_node->int_value = 0;

            AST_T* ast = init_ast(AST_BINOP);
            ast->left = right_node;
            
            ast->op = calloc(3, sizeof(char));
            strcpy(ast->op, "==");
            
            ast->right = zero_node;
            
            return ast;
        }

        case TOKEN_LPAREN: {
            AST_T* list_node = parser_parse_list(parser);
            if (list_node->type == AST_FUNCTION) {
                return list_node;
            }
            if (list_node->children->size == 1) {
                AST_T* actual_expr = list_node->children->items[0];
                free(list_node->children);
                free(list_node);
                return actual_expr;
            }
            
            return list_node;
        }
        
        case TOKEN_LBRACE: {
            parser_eat(parser, TOKEN_LBRACE);
            
            AST_T* ast = init_ast(AST_ARRAY);
            ast->children = init_list(sizeof(AST_T*));
            
            while (parser->token->type != TOKEN_RBRACE) {
                list_push(ast->children, parser_parse_expr(parser));
                if (parser->token->type == TOKEN_COMMA) {
                    parser_eat(parser, TOKEN_COMMA);
                }
            }
            
            parser_eat(parser, TOKEN_RBRACE);
            return ast;
        }

        case TOKEN_STRING: {
            AST_T* ast = init_ast(AST_STRING);
            ast->string_value = calloc(strlen(parser->token->value) + 1, sizeof(char));
            strcpy(ast->string_value, parser->token->value);
            parser_eat(parser, TOKEN_STRING);
            return ast;
        }
        default: 
            printf("[Parser]: Unexpected token in factor `%s`\n", token_to_str(parser->token)); 
            exit(1);
    }
}

AST_T* parser_parse_multiplicative(parser_T* parser) {
    AST_T* left = parser_parse_factor(parser);

    while (parser->token->type == TOKEN_MUL || 
           parser->token->type == TOKEN_DIV || 
           parser->token->type == TOKEN_MOD)
    {
        token_T* op_token = parser->token;
        parser_eat(parser, op_token->type);

        AST_T* binop = init_ast(AST_BINOP);
        binop->left = left;
        binop->op = op_token->value;
        binop->right = parser_parse_factor(parser);
        left = binop;
    }
    return left;
}

AST_T* parser_parse_additive(parser_T* parser) {
    AST_T* left = parser_parse_multiplicative(parser);

    while (parser->token->type == TOKEN_PLUS || parser->token->type == TOKEN_MINUS) {
        token_T* op_token = parser->token;
        parser_eat(parser, op_token->type);

        AST_T* binop = init_ast(AST_BINOP);
        binop->left = left;
        binop->op = op_token->value;
        binop->right = parser_parse_multiplicative(parser);
        left = binop;
    }
    return left;
}

AST_T* parser_parse_comparison(parser_T* parser) {
    AST_T* left = parser_parse_additive(parser);
    while (parser->token->type == TOKEN_LT || 
           parser->token->type == TOKEN_GT || 
           parser->token->type == TOKEN_EQUALS_EQUALS ||
           parser->token->type == TOKEN_NOT_EQUALS ||
           parser->token->type == TOKEN_LTE || 
           parser->token->type == TOKEN_GTE) 
    {
        token_T* op_token = parser->token;
        parser_eat(parser, op_token->type);

        AST_T* binop = init_ast(AST_BINOP);
        binop->left = left;
        
        binop->op = op_token->value; 
        
        binop->right = parser_parse_additive(parser);
        left = binop;
    }
    return left;
}


AST_T* parser_parse_expr(parser_T* parser) {
    
    if (parser->token->type == TOKEN_WHILE) {
        parser_eat(parser, TOKEN_WHILE);
        AST_T* ast = init_ast(AST_WHILE);
        parser_eat(parser, TOKEN_LPAREN);
        ast->value = parser_parse_expr(parser);
        parser_eat(parser, TOKEN_RPAREN);
        ast->left = parser_parse_block(parser);
        return ast;
    }

    if (parser->token->type == TOKEN_IF) {
        parser_eat(parser, TOKEN_IF);
        AST_T* ast = init_ast(AST_IF);
        
        parser_eat(parser, TOKEN_LPAREN);
        ast->value = parser_parse_expr(parser); 
        parser_eat(parser, TOKEN_RPAREN);
        
        ast->left = parser_parse_block(parser); 

        if (parser->token->type == TOKEN_ELIF) {
            
            parser->token->type = TOKEN_IF;
            
            ast->right = parser_parse_expr(parser); 
            
        } 
        else if (parser->token->type == TOKEN_ELSE) {
            parser_eat(parser, TOKEN_ELSE);
            ast->right = parser_parse_block(parser);
        } else {
            ast->right = NULL;
        }
        
        return ast;
    }
    if (parser->token->type == TOKEN_FOR) {
        return parser_parse_for(parser, NULL); 
    }

    if (parser->token->type == TOKEN_RETURN) {
        AST_T* ast = init_ast(AST_CALL);
        ast->name = calloc(7, sizeof(char));
        strcpy(ast->name, "return");
        parser_eat(parser, TOKEN_RETURN);
        ast->value = init_ast(AST_COMPOUND);
        list_push(ast->value->children, parser_parse_expr(parser));
        return ast;
    }
    
    return parser_parse_logical(parser);
}

AST_T* parser_parse(parser_T* parser)
{
  return parser_parse_compound(parser);
}

int get_expression_type(AST_T* node) {
    if (node == NULL) return 0; 

    switch (node->type) {
        case AST_INT: 
            return 1;
        case AST_STRING: 
            return 2;
        case AST_CALL:
            if (strcmp(node->name, "to_int") == 0) return 1;
            if (strcmp(node->name, "input") == 0) return 2;
            if (strcmp(node->name, "input_line") == 0) return 2;
            return 0;     
        case AST_BINOP: {
            int left_type = get_expression_type(node->left);
            int right_type = get_expression_type(node->right);
            
            if (left_type != 0 && right_type != 0) {
                if (left_type != right_type) {
                    printf("[Type Error]: Cannot perform '%s' between '%s' and '%s'\n", 
                        node->op,
                        left_type == 1 ? "int" : "string",
                        right_type == 1 ? "int" : "string");
                    exit(1);
                }
            }
            return left_type; 
        }
        case AST_VARIABLE: {
            for (int i = 0; i < sym_count; i++) {
                if (strcmp(sym_names[i], node->name) == 0) {
                    return sym_types[i];
                }
            }
            return 0;
        }
        
        default:
            return 0; 
    }
}

AST_T* parser_parse_id(parser_T* parser)
{
  char* value = calloc(strlen(parser->token->value) + 1, sizeof(char));
  strcpy(value, parser->token->value);
  parser_eat(parser, TOKEN_ID);
  
  int parsed_data_type = 0;

  if (parser->token->type == TOKEN_COLON)
  {
      parser_eat(parser, TOKEN_COLON);
      
      if (strcmp(parser->token->value, "int") == 0) {
          parsed_data_type = 1; 
          parser_eat(parser, parser->token->type); 

          if (parser->token->type == TOKEN_LBRACKET) {
              parser_eat(parser, TOKEN_LBRACKET);
              
              if (parser->token->type == TOKEN_RBRACKET) {
                  parser_eat(parser, TOKEN_RBRACKET);
                  parsed_data_type = 3;
                  
                  while (parser->token->type == TOKEN_LBRACKET) {
                      parser_eat(parser, TOKEN_LBRACKET);
                      parser_eat(parser, TOKEN_RBRACKET);
                  }
              } else {
                  AST_T* alloc_node = init_ast(AST_ARRAY_ALLOC);
                  alloc_node->children = init_list(sizeof(AST_T*));
                  
                  AST_T* dim_expr = parser_parse_expr(parser);
                  parser_eat(parser, TOKEN_RBRACKET);
                  list_push(alloc_node->children, dim_expr);
                  
                  while (parser->token->type == TOKEN_LBRACKET) {
                      parser_eat(parser, TOKEN_LBRACKET);
                      dim_expr = parser_parse_expr(parser);
                      parser_eat(parser, TOKEN_RBRACKET);
                      list_push(alloc_node->children, dim_expr);
                  }
                  AST_T* total_size = (AST_T*)alloc_node->children->items[0];
                  
                  for (unsigned int i = 1; i < alloc_node->children->size; i++) {
                      AST_T* next_dim = (AST_T*)alloc_node->children->items[i];
                      
                      AST_T* mul_node = init_ast(AST_BINOP);
                      mul_node->int_value = 4; 
                      mul_node->name = calloc(2, sizeof(char)); strcpy(mul_node->name, "*");
                      mul_node->op   = calloc(2, sizeof(char)); strcpy(mul_node->op, "*");
                      
                      mul_node->left = total_size;
                      mul_node->right = next_dim;
                      
                      total_size = mul_node;
                  }

                  alloc_node->value = total_size; 

                  AST_T* assign_node = init_ast(AST_ASSIGNMENT);
                  assign_node->name = value;
                  assign_node->data_type = 3; 
                  assign_node->value = alloc_node;
                  
                  return assign_node;
              }
          }
      }
      else if (strcmp(parser->token->value, "string") == 0) { 
          parsed_data_type = 2;  
          parser_eat(parser, parser->token->type); 
      }
      else {
          printf("[Parser Error]: Expected a valid data type like 'int' or 'string' after ':', but got '%s'\n", parser->token->value);
          exit(1);
      }
  }

  AST_T* array_access_node = NULL;
  if (parser->token->type == TOKEN_LBRACKET) {
      array_access_node = init_ast(AST_ACCESS);
      array_access_node->name = value;
      array_access_node->children = init_list(sizeof(AST_T*));
      
      while (parser->token->type == TOKEN_LBRACKET) {
          AST_T* index_wrapper = parser_parse_list(parser);
          list_push(array_access_node->children, index_wrapper);
      }
  }

  if(parser->token->type == TOKEN_EQUALS || 
    parser->token->type == TOKEN_PLUS_EQUALS ||
    parser->token->type == TOKEN_MINUS_EQUALS ||
    parser->token->type == TOKEN_MUL_EQUALS ||
    parser->token->type == TOKEN_DIV_EQUALS ||
    parser->token->type == TOKEN_MOD_EQUALS)
  {
    int op_type = parser->token->type; 
    parser_eat(parser, op_type);
    
    AST_T* ast = init_ast(AST_ASSIGNMENT);
    ast->name = value;
    ast->data_type = parsed_data_type; 
    
    if (array_access_node != NULL) {
        ast->left = array_access_node; 
    }
    
    if (op_type == TOKEN_EQUALS)             ast->int_value = 1;
    else if (op_type == TOKEN_PLUS_EQUALS)   ast->int_value = 2;
    else if (op_type == TOKEN_MINUS_EQUALS)  ast->int_value = 3;
    else if (op_type == TOKEN_MUL_EQUALS)    ast->int_value = 4;
    else if (op_type == TOKEN_DIV_EQUALS)    ast->int_value = 5;
    else if (op_type == TOKEN_MOD_EQUALS)    ast->int_value = 6;

    ast->value = parser_parse_expr(parser); 

    int assigned_type = get_expression_type(ast->value);

    int existing_type = 0;
    for (int i = 0; i < sym_count; i++) {
        if (strcmp(sym_names[i], ast->name) == 0) {
            existing_type = sym_types[i]; 
            break;
        }
    }

    if (existing_type != 0) 
    {
        if (existing_type != 3 && parsed_data_type != 3) {
            if (parsed_data_type != 0 && parsed_data_type != existing_type) {
                printf("\n[Semantic Error]: Variable '%s' already defined.\n", ast->name);
                exit(1);
            }
            if (assigned_type != 0 && assigned_type != existing_type) {
                printf("\n[Semantic Error]: Cannot assign mismatching type to '%s'.\n", ast->name);
                exit(1);
            }
        }
    } 
    else 
    {
        if (parsed_data_type != 0) {
            sym_names[sym_count] = ast->name;
            sym_types[sym_count] = parsed_data_type;
            sym_count++;
        } 
        else if (assigned_type != 0) {
            sym_names[sym_count] = ast->name;
            sym_types[sym_count] = assigned_type;
            sym_count++;
            ast->data_type = assigned_type; 
        }
    }
    return ast;
  }

  if (parsed_data_type != 0) {
      AST_T* ast = init_ast(AST_ASSIGNMENT);
      ast->name = value;
      ast->data_type = parsed_data_type;
      
      AST_T* default_val = init_ast(AST_INT);
      default_val->int_value = 0;
      ast->value = default_val;
      return ast;
  }

  if (array_access_node != NULL) {
      return array_access_node;
  }

  AST_T* ast = init_ast(AST_VARIABLE);
  ast->name = value;

  if (parser->token->type == TOKEN_LPAREN)
  {
    ast->type = AST_CALL;
    AST_T* args_node = parser_parse_list(parser);
      
    ast->value = args_node;
    ast->children = args_node->children; 
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
    
    if(parser->token->type == TOKEN_SEMI)
      parser_eat(parser, TOKEN_SEMI);
  }

  parser_eat(parser, TOKEN_RBRACE);
  return ast;
}

AST_T* parser_parse_list(parser_T* parser)
{
  unsigned int is_bracket = parser->token->type == TOKEN_LBRACKET;
  parser_eat(parser, is_bracket ? TOKEN_LBRACKET : TOKEN_LPAREN);

  AST_T* ast = init_ast(AST_COMPOUND);

  if (parser->token->type != TOKEN_RPAREN && parser->token->type != TOKEN_RBRACKET)
  {
      list_push(ast->children, parser_parse_expr(parser));

      while(parser->token->type == TOKEN_COMMA)
      {
        parser_eat(parser, TOKEN_COMMA);
        list_push(ast->children, parser_parse_expr(parser));
      }
  }

  parser_eat(parser, is_bracket ? TOKEN_RBRACKET : TOKEN_RPAREN);

  if (parser->token->type == TOKEN_COLON)
  {
    parser_eat(parser, TOKEN_COLON);
    if (parser->token->type == TOKEN_ID) {
        ast->data_type = typename_to_int(parser->token->value);
        parser_eat(parser, TOKEN_ID);
    }
  }

  if (parser->token->type == TOKEN_ARROW_RIGHT)
  {
    parser_eat(parser, TOKEN_ARROW_RIGHT);
    ast->type = AST_FUNCTION;
    ast->value = parser_parse_compound(parser);
  }

  return ast;
}

AST_T* parser_parse_int(parser_T* parser)
{
  int int_value = atoi(parser->token->value);
  parser_eat(parser, TOKEN_INT);

  AST_T* ast = init_ast(AST_INT);
  ast->int_value = int_value;

  return ast;
}

AST_T* parser_parse_compound(parser_T* parser)
{
  unsigned int should_close = 0;

  if (parser->token->type == TOKEN_LBRACE)
  {
    parser_eat(parser, TOKEN_LBRACE);
    should_close = 1;
  }

  AST_T* compound = init_ast(AST_COMPOUND);

  while(parser->token->type != TOKEN_EOF && parser->token->type != TOKEN_RBRACE)
  {
    list_push(compound->children, parser_parse_expr(parser));

    if(parser->token->type == TOKEN_SEMI)
      parser_eat(parser, TOKEN_SEMI);
  }

  if (should_close)
    parser_eat(parser, TOKEN_RBRACE);

  return compound;
}


AST_T* parser_parse_for(parser_T* parser, list_T* list) { 
    AST_T* ast = init_ast(AST_FOR); 
    
    parser_eat(parser, TOKEN_FOR);
    parser_eat(parser, TOKEN_LPAREN);

    ast->left = parser_parse_expr(parser); 
    parser_eat(parser, TOKEN_SEMI); 

    ast->value = parser_parse_expr(parser);
    parser_eat(parser, TOKEN_SEMI); 

    ast->right = parser_parse_expr(parser);
    parser_eat(parser, TOKEN_RPAREN);

    AST_T* body_block = parser_parse_block(parser); 
    ast->children = body_block->children; 

    return ast;
}


AST_T* parser_parse_logical(parser_T* parser) {
    AST_T* left = parser_parse_comparison(parser);

    while (parser->token->type == TOKEN_AND || 
           parser->token->type == TOKEN_OR) 
    {
        token_T* op_token = parser->token;
        parser_eat(parser, op_token->type);

        AST_T* binop = init_ast(AST_BINOP);
        binop->left = left;
        binop->op = op_token->value; 
        
        binop->right = parser_parse_comparison(parser);
        
        left = binop;
    }
    return left;
}