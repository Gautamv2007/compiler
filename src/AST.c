#include "include/AST.h"
#include <stdlib.h>

AST_T* init_ast(int type)
{
    AST_T* ast = calloc(1, sizeof(struct AST_STRUCT));
    ast->type = type;

    // Initialize the generic children list for Compound/Function nodes
    ast->children = init_list(sizeof(struct AST_STRUCT*));

    // Initialize new fields to NULL/0
    ast->name = NULL;
    ast->string_value = NULL;
    ast->op = NULL;
    ast->value = NULL;
    ast->left = NULL;
    ast->right = NULL;
    ast->int_value = 0;
    ast->data_type = 0;
    ast->fptr = NULL;

    return ast;
}