#ifndef TAC_AST_H
#define TAC_AST_H
#include "list.h"

struct VISITOR_STRUCT;

typedef struct AST_STRUCT
{
  enum 
  {
    AST_COMPOUND,
    AST_FUNCTION,
    AST_CALL,
    AST_ASSIGNMENT,
    AST_DEFINITION_TYPE,
    AST_VARIABLE,
    AST_STRING,
    AST_STATEMENT,
    AST_ACCESS,
    AST_INT,
    AST_BINOP,      // New: For +, -, *, /
    AST_IF,         // New: For conditionals
    AST_WHILE,      // New: For loops
    AST_NOOP,
  } type;
  
  list_T* children; 
  char* name;
  char* string_value;
  char* op;         // New: To store the operator (e.g., "+", "<")
  
  struct AST_STRUCT* left;  // New: For Binary Ops (left side)
  struct AST_STRUCT* right; // New: For Binary Ops (right side)
  struct AST_STRUCT* value; // For assignments/returns
  
  int int_value;
  int data_type;

  // Function pointer for the visitor to execute logic specific to this node
  struct AST_STRUCT* (*fptr)(struct VISITOR_STRUCT* visitor, struct AST_STRUCT* node, list_T* list);
} AST_T;

AST_T* init_ast(int type);
#endif