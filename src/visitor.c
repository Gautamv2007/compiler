#include "include/visitor.h"
#include "include/builtins.h"
#include "include/AST.h"
#include <stdio.h>
#include <string.h>

static AST_T* var_lookup(list_T* list, const char* name)
{
  for(int i = 0; i < (int) list->size;i++)
  {
    AST_T* child_ast = (AST_T*) list->items[i];

    if (child_ast->type != AST_VARIABLE || !child_ast->name)
      continue;

    if (strcmp(child_ast->name, name) == 0)
    {
      return child_ast;
    }
  }

  return 0;
}




visitor_T* init_visitor()
{
  visitor_T* visitor = calloc(1, sizeof(struct VISITOR_STRUCT));
  visitor->object = init_ast(AST_COMPOUND);

  builtins_init(visitor->object->children);

  return visitor;
}

AST_T* visitor_visit(visitor_T* visitor, AST_T* node, list_T* list)
{
  if (!node) return NULL;

  switch (node->type)
  {
    case AST_COMPOUND:   return visitor_visit_compound(visitor, node, list);
    case AST_ASSIGNMENT: return visitor_visit_assignment(visitor, node, list);
    case AST_VARIABLE:   return visitor_visit_variable(visitor, node, list);
    case AST_CALL:       return visitor_visit_call(visitor, node, list);
    case AST_INT:        return visitor_visit_int(visitor, node, list);
    case AST_ACCESS:     return visitor_visit_access(visitor, node, list);
    case AST_FUNCTION:   return visitor_visit_function(visitor, node, list);
    
    // --- New Visitors ---
    case AST_BINOP:      return visitor_visit_binop(visitor, node, list);
    case AST_WHILE:      return visitor_visit_while(visitor, node, list);
    
    default: { printf("[Visitor]: Don't know how to handle AST of type `%d`\n", node->type); exit(1); }
  }
}

AST_T* visitor_visit_binop(visitor_T* visitor, AST_T* node, list_T* list)
{
  AST_T* binop = init_ast(AST_BINOP);
  binop->op = node->op;
  binop->left = visitor_visit(visitor, node->left, list);
  binop->right = visitor_visit(visitor, node->right, list);
  return binop;
}

AST_T* visitor_visit_while(visitor_T* visitor, AST_T* node, list_T* list)
{
  AST_T* while_node = init_ast(AST_WHILE);
  // Condition
  while_node->value = visitor_visit(visitor, node->value, list);
  // Body (usually stored in left for your structure)
  while_node->left = visitor_visit(visitor, node->left, list);
  return while_node;
}

AST_T* visitor_visit_compound(visitor_T* visitor, AST_T* node, list_T* list)
{
  AST_T* compound = init_ast(AST_COMPOUND); 

  for(unsigned int i = 0;i < node->children->size;i++)
  {
    AST_T* x = visitor_visit(visitor, (AST_T*) node->children->items[i], list);
    list_push(compound->children, x);
  }

  return compound;
}

AST_T* visitor_visit_assignment(visitor_T* visitor, AST_T* node, list_T* list)
{
  AST_T* new_var = init_ast(AST_ASSIGNMENT);
  new_var->name = node->name;
  new_var->value = visitor_visit(visitor, node->value, list);
  list_push(list, new_var);

  return new_var;
}

AST_T* visitor_visit_variable(visitor_T* visitor, AST_T* node, list_T* list)
{
  AST_T* var = var_lookup(visitor->object->children, node->name);

  if(var)
    return var;

  return node;
}

AST_T* visitor_visit_function(visitor_T* visitor, AST_T* node, list_T* list)
{
  AST_T* func = init_ast(AST_FUNCTION);
  
  // DEEP COPY: Instead of overwriting func->children, we push into it!
  for (size_t i = 0; i < node->children->size; i++) 
  {
      AST_T* old_arg = (AST_T*) node->children->items[i];
      AST_T* new_arg = visitor_visit(visitor, old_arg, list);
      list_push(func->children, new_arg); 
  }
  
  func->value = visitor_visit(visitor, node->value, list);
  return func;
}

AST_T* visitor_visit_call(visitor_T* visitor, AST_T* node, list_T* list)
{
  AST_T* var = var_lookup(visitor->object->children, node->name);

  if (var)
  {
    if (var->fptr)
    {
      AST_T* x = var->fptr(visitor, var, node->value->children);
      return x;
    }
  }
  return node;
}
AST_T* visitor_visit_int(visitor_T* visitor, AST_T* node, list_T* list)
{
  return node;
}
AST_T* visitor_visit_access(visitor_T* visitor, AST_T* node, list_T* list)
{
  return node;
}

