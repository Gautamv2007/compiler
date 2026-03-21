#include "include/visitor.h"
#include "include/builtins.h"
#include "include/AST.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static AST_T* var_lookup(list_T* list, const char* name)
{
  if (!list) return NULL;
  
  // 1. Loop backwards so block-scoped variables properly shadow outer variables!
  for(int i = (int)list->size - 1; i >= 0; i--)
  {
    AST_T* child_ast = (AST_T*) list->items[i];

    if (!child_ast->name)
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
    case AST_BINOP:      return visitor_visit_binop(visitor, node, list);
    case AST_WHILE:      return visitor_visit_while(visitor, node, list);
    case AST_FOR:        return visitor_visit_for(visitor, node, list);
    case AST_IF:         return visitor_visit_if(visitor, node, list);
    case AST_ARRAY:      return visitor_visit_array(visitor, node, list);
    case AST_ARRAY_ALLOC: return visitor_visit_array_alloc(visitor, node, list);
    case AST_STRING:     return visitor_visit_string(visitor, node, list);
    
    default: { printf("[Visitor]: Don't know how to handle AST of type `%d`\n", node->type); exit(1); }
  }
}

AST_T* visitor_visit_binop(visitor_T* visitor, AST_T* node, list_T* list)
{
  AST_T* binop = init_ast(AST_BINOP);
  binop->op = node->op;
  binop->int_value = node->int_value;
  binop->left = visitor_visit(visitor, node->left, list);
  binop->right = visitor_visit(visitor, node->right, list);
  return binop;
}

// 2. TRUE BLOCK SCOPING: Create an isolated sandbox scope for blocks and loops!
AST_T* visitor_visit_compound(visitor_T* visitor, AST_T* node, list_T* list)
{
  AST_T* compound = init_ast(AST_COMPOUND); 

  list_T* block_scope = init_list(sizeof(AST_T*));
  if (list) {
      for (unsigned int i = 0; i < list->size; i++) list_push(block_scope, list->items[i]);
  }

  for(unsigned int i = 0; i < node->children->size; i++)
  {
    AST_T* x = visitor_visit(visitor, (AST_T*) node->children->items[i], block_scope);
    list_push(compound->children, x);
  }

  return compound; // When this returns, 'block_scope' is erased!
}

AST_T* visitor_visit_for(visitor_T* visitor, AST_T* node, list_T* list)
{
    AST_T* for_node = init_ast(AST_FOR);
    if (node->name) for_node->name = node->name; 

    list_T* block_scope = init_list(sizeof(AST_T*));
    if (list) {
        for (unsigned int i = 0; i < list->size; i++) list_push(block_scope, list->items[i]);
    }

    if (node->left) for_node->left = visitor_visit(visitor, node->left, block_scope);
    if (node->value) for_node->value = visitor_visit(visitor, node->value, block_scope);
    if (node->right) for_node->right = visitor_visit(visitor, node->right, block_scope);

    if (node->children) {
        for (unsigned int i = 0; i < node->children->size; i++) {
            AST_T* child = (AST_T*) node->children->items[i];
            AST_T* visited_child = visitor_visit(visitor, child, block_scope);
            list_push(for_node->children, visited_child);
        }
    }
    
    return for_node; // When this returns, 'i' is officially deleted from memory!
}

AST_T* visitor_visit_while(visitor_T* visitor, AST_T* node, list_T* list)
{
  AST_T* while_node = init_ast(AST_WHILE);
  
  list_T* block_scope = init_list(sizeof(AST_T*));
  if (list) {
      for (unsigned int i = 0; i < list->size; i++) list_push(block_scope, list->items[i]);
  }
  
  if (node->value) while_node->value = visitor_visit(visitor, node->value, block_scope);
  if (node->left) while_node->left = visitor_visit(visitor, node->left, block_scope);
  
  return while_node;
}

AST_T* visitor_visit_if(visitor_T* visitor, AST_T* node, list_T* list)
{
  AST_T* if_node = init_ast(AST_IF);
  
  list_T* block_scope = init_list(sizeof(AST_T*));
  if (list) {
      for (unsigned int i = 0; i < list->size; i++) list_push(block_scope, list->items[i]);
  }
  
  if (node->value) if_node->value = visitor_visit(visitor, node->value, block_scope);
  if (node->left) if_node->left = visitor_visit(visitor, node->left, block_scope);
  if (node->right) if_node->right = visitor_visit(visitor, node->right, block_scope);
  
  return if_node;
}

AST_T* visitor_visit_function(visitor_T* visitor, AST_T* node, list_T* list)
{
  AST_T* func = init_ast(AST_FUNCTION);
  
  list_T* func_scope = init_list(sizeof(AST_T*));
  if (list) {
      for (unsigned int i = 0; i < list->size; i++) list_push(func_scope, list->items[i]);
  }
  
  for (size_t i = 0; i < node->children->size; i++) 
  {
      AST_T* old_arg = (AST_T*) node->children->items[i];
      AST_T* new_arg = visitor_visit(visitor, old_arg, func_scope);
      list_push(func->children, new_arg); 
  }
  
  if (node->value) {
      func->value = visitor_visit(visitor, node->value, func_scope);
  }
  return func;
}

AST_T* visitor_visit_assignment(visitor_T* visitor, AST_T* node, list_T* list)
{
  AST_T* new_var = init_ast(AST_ASSIGNMENT);
  new_var->name = node->name;
  new_var->data_type = node->data_type;
  new_var->int_value = node->int_value;

  visitor->stack_count += 4; 
  new_var->stack_offset = visitor->stack_count; 

  list_push(list, new_var);

  if (node->left) new_var->left = visitor_visit(visitor, node->left, list);
  if (node->right) new_var->right = visitor_visit(visitor, node->right, list);
  if (node->value) new_var->value = visitor_visit(visitor, node->value, list);

  return new_var;
}

AST_T* visitor_visit_variable(visitor_T* visitor, AST_T* node, list_T* list)
{
    AST_T* var = var_lookup(list, node->name);
    
    if (!var) var = var_lookup(visitor->object->children, node->name);

    // 3. ENFORCE SCOPE: If 'i' isn't in the active list, crash compilation!
    if (!var) {
        int line = node->line > 0 ? node->line : 0; 
        fprintf(stderr, "\n[Semantic Error] at line %d: Undefined variable '%s'. It may be out of scope!\n", line, node->name);
        exit(1);
    }

    return node;
}

AST_T* visitor_visit_call(visitor_T* visitor, AST_T* node, list_T* list)
{
    if (node->value) {
        node->value = visitor_visit(visitor, node->value, list);
    }

    if (strcmp(node->name, "print") == 0 || strcmp(node->name, "input") == 0 || strcmp(node->name, "to_int") == 0 || strcmp(node->name, "return") == 0 || strcmp(node->name, "input_line") == 0) {
        return node; 
    }
    
    AST_T* func = var_lookup(list, node->name);
    if (!func) func = var_lookup(visitor->object->children, node->name);

    if (!func) {
        fprintf(stderr, "\n[Semantic Error]: Call to undefined function '%s()'.\n", node->name);
        exit(1);
    }

    return node;
}

AST_T* visitor_visit_int(visitor_T* visitor, AST_T* node, list_T* list) { return node; }
AST_T* visitor_visit_string(visitor_T* visitor, AST_T* node, list_T* list) { return node; }

AST_T* visitor_visit_array(visitor_T* visitor, AST_T* node, list_T* list)
{
    AST_T* arr_node = init_ast(AST_ARRAY);
    if (node->children) {
        for (unsigned int i = 0; i < node->children->size; i++) {
            AST_T* child = (AST_T*) node->children->items[i];
            list_push(arr_node->children, visitor_visit(visitor, child, list));
        }
    }
    return arr_node;
}

AST_T* visitor_visit_array_alloc(visitor_T* visitor, AST_T* node, list_T* list)
{
    AST_T* alloc_node = init_ast(AST_ARRAY_ALLOC);
    if (node->value) alloc_node->value = visitor_visit(visitor, node->value, list);
    if (node->children) {
        alloc_node->children = init_list(sizeof(AST_T*));
        for (unsigned int i = 0; i < node->children->size; i++) {
            AST_T* child = visitor_visit(visitor, (AST_T*)node->children->items[i], list);
            list_push(alloc_node->children, child);
        }
    }
    return alloc_node;
}

AST_T* visitor_visit_access(visitor_T* visitor, AST_T* node, list_T* list)
{
    AST_T* access_node = init_ast(AST_ACCESS);
    access_node->name = node->name;
    if (node->children) {
        access_node->children = init_list(sizeof(AST_T*));
        for (unsigned int i = 0; i < node->children->size; i++) {
            AST_T* child = visitor_visit(visitor, (AST_T*)node->children->items[i], list);
            list_push(access_node->children, child);
        }
    }
    return access_node;
}