#include "include/parser.h"
#include "include/AST.h"
#include "include/types.h"
#include "include/lexer.h"
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
    printf("[Parser]: Unexpected token: `%s`, was expecting: `%s`\n", token_to_str(parser->token), token_type_to_str(type));
    exit(1);
  }
  parser->token = lexer_next_token(parser->lexer);
  return parser->token;
}

// Forward declarations
AST_T* parser_parse_factor(parser_T* parser);
AST_T* parser_parse_multiplicative(parser_T* parser);
AST_T* parser_parse_additive(parser_T* parser);
AST_T* parser_parse_comparison(parser_T* parser);

// 1. Factors: The smallest units (numbers, variables, or parenthesized expressions)
AST_T* parser_parse_factor(parser_T* parser) {
    switch (parser->token->type) {
        case TOKEN_ID:      return parser_parse_id(parser);
        case TOKEN_INT:     return parser_parse_int(parser);
        case TOKEN_LPAREN: {
            parser_eat(parser, TOKEN_LPAREN);
            
            // Case 1: Empty function definition, e.g., `main = () -> { ... }`
            if (parser->token->type == TOKEN_RPAREN) {
                parser_eat(parser, TOKEN_RPAREN);
                
                if (parser->token->type == TOKEN_ARROW_RIGHT) {
                    parser_eat(parser, TOKEN_ARROW_RIGHT);
                    AST_T* ast = init_ast(AST_FUNCTION);
                    ast->children = init_list(sizeof(struct AST_STRUCT*)); // Empty arguments
                    ast->value = parser_parse_compound(parser);
                    return ast;
                }
                printf("[Parser]: Unexpected empty parentheses\n");
                exit(1);
            }
            
            // Case 2: Parenthesized math `(5 + 2)` OR function with args `(x) -> { ... }`
            AST_T* node = parser_parse_expr(parser);
            parser_eat(parser, TOKEN_RPAREN);
            
            if (parser->token->type == TOKEN_ARROW_RIGHT) {
                parser_eat(parser, TOKEN_ARROW_RIGHT);
                AST_T* ast = init_ast(AST_FUNCTION);
                ast->children = init_list(sizeof(struct AST_STRUCT*));
                list_push(ast->children, node);
                ast->value = parser_parse_compound(parser);
                return ast;
            }
            
            return node;
        }
        default: 
            printf("[Parser]: Unexpected token in factor `%s`\n", token_to_str(parser->token)); 
            exit(1);
    }
}

// 2. Multiplicative: Handles * and /
AST_T* parser_parse_multiplicative(parser_T* parser) {
    AST_T* left = parser_parse_factor(parser);

    while (parser->token->type == TOKEN_MUL || parser->token->type == TOKEN_DIV) {
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

// 3. Additive: Handles + and -
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

// 4. Comparison: Handles <, >, ==
AST_T* parser_parse_comparison(parser_T* parser) {
    AST_T* left = parser_parse_additive(parser);

    while (parser->token->type == TOKEN_LT || parser->token->type == TOKEN_GT || parser->token->type == TOKEN_EQUALS_EQUALS) {
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


// This is now the entry point for an expression
AST_T* parser_parse_expr(parser_T* parser) {
    
    // Check for "while"
    if (parser->token->type == TOKEN_WHILE) {
        parser_eat(parser, TOKEN_WHILE);
        AST_T* ast = init_ast(AST_WHILE);
        parser_eat(parser, TOKEN_LPAREN);
        ast->value = parser_parse_expr(parser); // The condition
        parser_eat(parser, TOKEN_RPAREN);
        ast->left = parser_parse_block(parser); // The body
        return ast;
    }

    // Check for "return"
    if (parser->token->type == TOKEN_RETURN) {
        AST_T* ast = init_ast(AST_CALL); // Repurposing CALL for return
        ast->name = calloc(7, sizeof(char));
        strcpy(ast->name, "return");
        parser_eat(parser, TOKEN_RETURN);
        ast->value = init_ast(AST_COMPOUND);
        list_push(ast->value->children, parser_parse_expr(parser));
        return ast;
    }
    
    // Top precedence is comparison (which delegates down to math)
    return parser_parse_comparison(parser);
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
  
  // Assignment: x = 5
  if(parser->token->type == TOKEN_EQUALS)
  {
    parser_eat(parser, TOKEN_EQUALS);
    AST_T* ast = init_ast(AST_ASSIGNMENT);
    ast->name = value;
    ast->value = parser_parse_expr(parser);
    return ast; 
  }

  // Variable or Function Call
  AST_T* ast = init_ast(AST_VARIABLE);
  ast->name = value;

  // If it's a function call (like print(x))
  if (parser->token->type == TOKEN_LPAREN)
  {
    ast->type = AST_CALL;
    AST_T* args_node = parser_parse_list(parser);
      
      // 1. Give the backend the compound node (how you originally had it)
    ast->value = args_node;
      
      // 2. ALSO hand the arguments directly to the call node's children list!
    ast->children = args_node->children; 
  }
  // If it's an array access
  else if (parser->token->type == TOKEN_LBRACKET)
  {
      ast->type = AST_ACCESS;
      ast->value = parser_parse_list(parser);
  }


  //comment this after testing
  // if (ast->type == AST_CALL) {
  //     printf("[Tracker] Parser created CALL '%s' at %p | value: %p | args: %zu\n", 
  //            ast->name, (void*)ast, (void*)ast->value, ast->children ? ast->children->size : 0);
  // }

  return ast;
}

AST_T* parser_parse_block(parser_T* parser)
{
  parser_eat(parser, TOKEN_LBRACE);
  AST_T* ast = init_ast(AST_COMPOUND);

  while(parser->token->type != TOKEN_RBRACE)
  {
    list_push(ast->children, parser_parse_expr(parser));
    
    // Eat trailing semicolons inside blocks
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

  // Parse arguments only if the list isn't empty
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

  // Check for type signatures (e.g., args: int) or arrow functions
  if (parser->token->type == TOKEN_COLON)
  {
    parser_eat(parser, TOKEN_COLON);
    // Simplified for now: just eat the type identifier
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