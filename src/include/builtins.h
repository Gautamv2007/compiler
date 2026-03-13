#ifndef TAC_BUILTINS_H
#define TAC_BUILTINS_H
#include "visitor.h"
#include "AST.h"

AST_T* fptr_print(visitor_T* visitor, AST_T* node, list_T* list);
void builtins_init(list_T* list);
char* mkstr(const char* str);
void builtins_register_fptr(list_T* list, const char* name, AST_T* (*fptr)(visitor_T* visitor, AST_T* node, list_T* list));
#endif
