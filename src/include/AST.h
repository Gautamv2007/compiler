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
    AST_ARRAY,
    AST_ARRAY_ALLOC,
    AST_INT,
    AST_BINOP,      //for +, -, *, /
    AST_IF,         //for conditionals
    AST_WHILE,      //for loops
    AST_FOR,
    AST_NOOP,

  } type;
  
  list_T* children; 
  char* name;
  char* string_value;
  char* op;         //for storing the operator (e.g., "+", "<")

  int stack_offset;
  
  struct AST_STRUCT* left;  //for Binary Ops (left side)
  struct AST_STRUCT* right; //for Binary Ops (right side)
  struct AST_STRUCT* value; //for assignments/returns
  
  int int_value;
  int data_type;
  int int_value2; //for future use (e.g., array sizes, second operand in certain operations)

  // Function pointer for the visitor to execute logic specific to this node
  struct AST_STRUCT* (*fptr)(struct VISITOR_STRUCT* visitor, struct AST_STRUCT* node, list_T* list);
} AST_T;

AST_T* init_ast(int type);
#endif