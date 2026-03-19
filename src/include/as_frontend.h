#ifndef TAC_AS_FRONTEND_H
#define TAC_AS_FRONTEND_H

#include "AST.h"
#include "list.h"

// Main entry point for the assembly generator
char* as_f_root(AST_T* ast, list_T* list);

// The recursive dispatcher
char* as_f(AST_T* ast, list_T* list);

// Feature-specific generators
char* as_f_compound(AST_T* ast, list_T* list);
char* as_f_assignment(AST_T* ast, list_T* list);
char* as_f_variable(AST_T* ast, list_T* list);
char* as_f_call(AST_T* ast, list_T* list);
char* as_f_int(AST_T* ast, list_T* list);
char* as_f_binop(AST_T* ast, list_T* list); // New: Arithmetic handler
char* as_f_while(AST_T* ast, list_T* list); // New: Loop handler
char* as_f_if(AST_T* ast, list_T* list);    // New: Conditional handler
char* as_f_access(AST_T* ast, list_T* list);
void hoist_local_variables(AST_T* ast, list_T* list); // New: Pre-scan for local variable declarations and stack offset assignment
char* as_f_string(AST_T* ast, list_T* list); // New: String literal handler

#endif