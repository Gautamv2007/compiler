#include "include/parser.h"
#include "include/AST.h"
#include "include/types.h"
#include "include/lexer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// --- NEW: A simple Symbol Table to remember variable types ---
char* sym_names[1000];
int sym_types[1000];
int sym_count = 0;
// -----------------------------------------------------------

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
// 1. Factors: The smallest units (numbers, variables, or parenthesized expressions)
AST_T* parser_parse_factor(parser_T* parser) {
    switch (parser->token->type) {
        case TOKEN_ID:      return parser_parse_id(parser);
        case TOKEN_INT:     return parser_parse_int(parser);

        case TOKEN_MINUS: {
            parser_eat(parser, TOKEN_MINUS);
            
            // Parse whatever comes after the minus sign (number, variable, etc.)
            AST_T* right_node = parser_parse_factor(parser);

            // OPTIMIZATION (Constant Folding): 
            // If it's a raw number, just flip the sign natively.
            // This ensures our for-loop still sees a pure negative integer!
            if (right_node->type == AST_INT) {
                right_node->int_value = -right_node->int_value;
                return right_node;
            }

            // THE COMPILER TRICK: 
            // If it's a variable (like -i) or expression (like -(x+2)), 
            // secretly rewrite it as "0 - i" for the backend.
            AST_T* zero_node = init_ast(AST_INT);
            zero_node->int_value = 0;

            AST_T* ast = init_ast(AST_BINOP);
            ast->left = zero_node;
            
            // Set the operator to "-"
            ast->op = calloc(2, sizeof(char));
            strcpy(ast->op, "-");
            
            ast->right = right_node;
            
            return ast;
        }

        case TOKEN_LPAREN: {
            // Use your existing list parser to handle the parentheses!
            // It will read the inside, and if it sees an arrow `->`, it will 
            // automatically build an AST_FUNCTION for us.
            AST_T* list_node = parser_parse_list(parser);
            
            // If it parsed a function like `(a:int) -> { ... }`, return it!
            if (list_node->type == AST_FUNCTION) {
                return list_node;
            }
            
            // If it was just regular math inside parentheses like `(5 + 2)`,
            // parser_parse_list wraps it in an AST_COMPOUND. We just want the math inside.
            if (list_node->children->size == 1) {
                AST_T* actual_expr = list_node->children->items[0];
                free(list_node->children);
                free(list_node); // Clean up the wrapper
                return actual_expr;
            }
            
            return list_node;
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

// 2. Multiplicative: Handles * and /
// 2. Multiplicative: Handles *, /, and %
AST_T* parser_parse_multiplicative(parser_T* parser) {
    AST_T* left = parser_parse_factor(parser);

    // NEW: Added the modulo token to this loop!
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
    while (parser->token->type == TOKEN_LT || 
           parser->token->type == TOKEN_GT || 
           parser->token->type == TOKEN_EQUALS_EQUALS ||
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

    // --- NEW: Check for "if" and "else" ---
    if (parser->token->type == TOKEN_IF) {
        parser_eat(parser, TOKEN_IF);
        AST_T* ast = init_ast(AST_IF);
        
        // The condition
        parser_eat(parser, TOKEN_LPAREN);
        ast->value = parser_parse_expr(parser); 
        parser_eat(parser, TOKEN_RPAREN);
        
        // The main 'if' body
        ast->left = parser_parse_block(parser); 

        // --- NEW: The 'elif' chain ---
        if (parser->token->type == TOKEN_ELIF) {
            
            // 1. Spoof the token! Change ELIF to IF so the next cycle parses it normally.
            parser->token->type = TOKEN_IF;
            
            // 2. Recursively parse the new 'if' statement and attach it to the false branch.
            // IMPORTANT: Change `parser_parse_expr` to whatever function this code is currently inside!
            ast->right = parser_parse_expr(parser); 
            
        } 
        // --- Existing 'else' body ---
        else if (parser->token->type == TOKEN_ELSE) {
            parser_eat(parser, TOKEN_ELSE);
            ast->right = parser_parse_block(parser);
        } else {
            ast->right = NULL; // Explicitly set to NULL if there is no else/elif
        }
        
        return ast;
    }
    // --------------------------------------
    if (parser->token->type == TOKEN_FOR) {
        // We pass NULL for the list since we removed its dependency inside the function!
        return parser_parse_for(parser, NULL); 
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
// Top precedence is logical (which delegates to comparison, then math)
    return parser_parse_logical(parser);
}

AST_T* parser_parse(parser_T* parser)
{
  return parser_parse_compound(parser);
}

// --- NEW: Recursive Type Checker ---
int get_expression_type(AST_T* node) {
    if (node == NULL) return 0; 

    switch (node->type) {
        case AST_INT: 
            return 1; // 1 = int
            
        case AST_STRING: 
            return 2; // 2 = string
            
        case AST_CALL:
            if (strcmp(node->name, "to_int") == 0) return 1;
            if (strcmp(node->name, "input") == 0) return 2;
            if (strcmp(node->name, "input_line") == 0) return 2;
            return 0; 
            
        case AST_BINOP: {
            // Dig into the math operation
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

        // Add this right above your default: case
        case AST_VARIABLE: {
            for (int i = 0; i < sym_count; i++) {
                if (strcmp(sym_names[i], node->name) == 0) {
                    return sym_types[i]; // Found it! Return the saved type.
                }
            }
            return 0; // Not found in our table, return unknown.
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
  
  int parsed_data_type = 0; // 0 means "Unknown" or "No type specified"

  // NEW: Check for the colon to capture the data type (e.g., :int)
  // NEW: Check for the colon to capture the data type (e.g., :int)
  if (parser->token->type == TOKEN_COLON)
  {
      parser_eat(parser, TOKEN_COLON);
      
      // Check the actual string value to be completely safe from Lexer quirks
      if (strcmp(parser->token->value, "int") == 0) {
          parsed_data_type = 1; // 1 = Integer 
          parser_eat(parser, parser->token->type); // Eat the type token safely
      }
      else if (strcmp(parser->token->value, "string") == 0) { 
          parsed_data_type = 2; // 2 = String 
          parser_eat(parser, parser->token->type); 
      }
      else {
          printf("[Parser Error]: Expected a valid data type like 'int' or 'string' after ':', but got '%s'\n", parser->token->value);
          exit(1);
      }
  }

  // Assignment: x:int = 5  OR  x = 5
  // printf("[DEBUG Parser]: Checking assignment for '%s'. Token Type is: %d\n", value, parser->token->type);
  if(parser->token->type == TOKEN_EQUALS || 
    parser->token->type == TOKEN_PLUS_EQUALS ||
    parser->token->type == TOKEN_MINUS_EQUALS ||
    parser->token->type == TOKEN_MUL_EQUALS ||
    parser->token->type == TOKEN_DIV_EQUALS ||
    parser->token->type == TOKEN_MOD_EQUALS)
  {
    int op_type = parser->token->type; // Remember which operator it is for the AST node
    // token_T* op_token = parser->token;
    
    // 2. Eat the token dynamically so it doesn't crash
    parser_eat(parser, op_type);
    
    AST_T* ast = init_ast(AST_ASSIGNMENT);
    ast->name = value;
    ast->data_type = parsed_data_type; 
    
    
    if (op_type == TOKEN_EQUALS)             ast->int_value = 1;
    else if (op_type == TOKEN_PLUS_EQUALS)   ast->int_value = 2;
    else if (op_type == TOKEN_MINUS_EQUALS)  ast->int_value = 3;
    else if (op_type == TOKEN_MUL_EQUALS)    ast->int_value = 4;
    else if (op_type == TOKEN_DIV_EQUALS)    ast->int_value = 5;
    else if (op_type == TOKEN_MOD_EQUALS)    ast->int_value = 6;


    ast->value = parser_parse_expr(parser); 

    int assigned_type = get_expression_type(ast->value);

    // --- Check if the variable ALREADY exists in our memory ---
    int existing_type = 0;
    for (int i = 0; i < sym_count; i++) {
        if (strcmp(sym_names[i], ast->name) == 0) {
            existing_type = sym_types[i]; // We found it!
            break;
        }
    }

    if (existing_type != 0) 
    {
        // CASE 1: The user tried to re-declare the type (e.g., x:string = "hello")
        if (parsed_data_type != 0 && parsed_data_type != existing_type) {
            printf("\n[Semantic Error]: Variable '%s' already defined as '%s'.\n", 
                   ast->name, existing_type == 1 ? "int" : "string");
            printf("  -> You cannot re-declare it as '%s'!\n", 
                   parsed_data_type == 1 ? "int" : "string");
            exit(1);
        }
        
        // CASE 2: They didn't re-declare the type, but tried to assign the wrong value type (e.g., x = "hello")
        if (assigned_type != 0 && assigned_type != existing_type) {
            printf("\n[Semantic Error]: Cannot assign a '%s' to variable '%s' (which is of type '%s').\n", 
                   assigned_type == 1 ? "int" : "string", 
                   ast->name, 
                   existing_type == 1 ? "int" : "string");
            exit(1);
        }
    } 
    else 
    {
        // IT IS A BRAND NEW VARIABLE!
        if (parsed_data_type != 0) {
            // They explicitly provided a type (x:int = 5)
            if (assigned_type != 0 && parsed_data_type != assigned_type) {
                printf("\n[Semantic Error]: Type Mismatch for variable '%s'\n", ast->name);
                exit(1);
            }
            // Save to Symbol Table
            sym_names[sym_count] = ast->name;
            sym_types[sym_count] = parsed_data_type;
            sym_count++;
        } 
        else if (assigned_type != 0) {
            // BONUS: TYPE INFERENCE! 
            sym_names[sym_count] = ast->name;
            sym_types[sym_count] = assigned_type;
            sym_count++;
            ast->data_type = assigned_type; 
        }
    }
    // printf("[DEBUG Parser]: Address: %p | '%s' Opcode is: %d\n", (void*)ast, ast->name, ast->int_value);
    return ast;
  }

  // If a type was provided but NO equals sign (e.g., `x:int;`), handle it here
  if (parsed_data_type != 0) {
      // For now, we'll treat uninitialized variables as an assignment to 0
      // You can change this later to a dedicated 'Declaration' AST node if you want
      AST_T* ast = init_ast(AST_ASSIGNMENT);
      ast->name = value;
      ast->data_type = parsed_data_type;
      
      AST_T* default_val = init_ast(AST_INT);
      default_val->int_value = 0;
      ast->value = default_val;
      return ast;
  }

  // Variable Access or Function Call
  AST_T* ast = init_ast(AST_VARIABLE);
  ast->name = value;

  // If it's a function call (like print(x))
  if (parser->token->type == TOKEN_LPAREN)
  {
    ast->type = AST_CALL;
    AST_T* args_node = parser_parse_list(parser);
      
    ast->value = args_node;
    ast->children = args_node->children; 
  }
  // If it's an array access
  else if (parser->token->type == TOKEN_LBRACKET)
  {
      ast->type = AST_ACCESS;
      ast->value = parser_parse_list(parser);
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


AST_T* parser_parse_for(parser_T* parser, list_T* list) { 
    AST_T* ast = init_ast(AST_FOR); 
    
    parser_eat(parser, TOKEN_FOR);
    parser_eat(parser, TOKEN_LPAREN);

    // 1. Initialization (e.g., i:int = 0)
    ast->left = parser_parse_expr(parser); 
    parser_eat(parser, TOKEN_SEMI); // <-- CHANGED TO SEMICOLON

    // 2. Condition (e.g., i < 10)
    ast->value = parser_parse_expr(parser);
    parser_eat(parser, TOKEN_SEMI); // <-- CHANGED TO SEMICOLON

    // 3. Increment (e.g., i += 2)
    ast->right = parser_parse_expr(parser);
    parser_eat(parser, TOKEN_RPAREN);

    // 4. The Body { ... } 
    AST_T* body_block = parser_parse_block(parser); 
    ast->children = body_block->children; 

    return ast;
}


// 5. Logical: Handles && and ||
AST_T* parser_parse_logical(parser_T* parser) {
    // 1. First, evaluate any comparisons (e.g., x == 1)
    AST_T* left = parser_parse_comparison(parser);

    // 2. Check if there is an AND or OR joining it to another comparison
    while (parser->token->type == TOKEN_AND || 
           parser->token->type == TOKEN_OR) 
    {
        token_T* op_token = parser->token;
        parser_eat(parser, op_token->type);

        AST_T* binop = init_ast(AST_BINOP);
        binop->left = left;
        binop->op = op_token->value; 
        
        // 3. Parse the right side of the && or ||
        binop->right = parser_parse_comparison(parser);
        
        left = binop;
    }
    return left;
}