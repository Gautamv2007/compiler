#include "include/builtins.h"
#include "include/AST.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

char* mkstr(const char* str)
{
  char* outstr = (char*) calloc(strlen(str) + 1, sizeof(char));
  strcpy(outstr, str);
  return outstr;
}

AST_T* fptr_print(visitor_T* visitor, AST_T* node, list_T* list)
{
  // 1. Create the new CALL node for the backend
  AST_T* ast = init_ast(AST_CALL);
  ast->name = mkstr("print");

  // 2. The 'list' parameter contains our raw parser arguments.
  // We need to visit them and deep copy them into our new ast's children list.
  if (list != NULL)
  {
      for (size_t i = 0; i < list->size; i++) 
      {
          AST_T* original_arg = (AST_T*)list->items[i];
          AST_T* visited_arg = visitor_visit(visitor, original_arg, list);
          
          list_push(ast->children, visited_arg);
      }
  }

  // Note: if your backend also explicitly checks ast->value for the first argument, 
  // you might want to set it here like: 
  // if (ast->children->size > 0) ast->value = ast->children->items[0];

  return ast;
}

void builtins_register_fptr(list_T* list, const char* name, AST_T* (*fptr)(visitor_T* visitor, AST_T* node, list_T* list))
{
  AST_T* fptr_var = init_ast(AST_VARIABLE);
  fptr_var->name = mkstr(name);
  fptr_var->fptr = fptr;
  list_push(list, fptr_var);
}

void builtins_init(list_T* list)
{
  builtins_register_fptr(list, "print", fptr_print);
}