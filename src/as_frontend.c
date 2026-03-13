#include "include/as_frontend.h"
#include "include/AST.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int label_count = 0;
static int current_local_offset = -4;

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

char* as_f_compound(AST_T* ast, list_T* list)
{
  char* value = calloc(1, sizeof(char));

  for (int i = 0;i<(int)ast->children->size;i++)
  {
    AST_T* child_ast = (AST_T*) ast->children->items[i];
    char* next_value = as_f(child_ast, list);
    value = realloc(value, (strlen(next_value) + strlen(value) + 1) * sizeof(char));
    strcat(value, next_value);
    free(next_value); // Prevent leak
  }

  return value;
}

char* as_f_binop(AST_T* ast, list_T* list) {
    char* s = calloc(1, sizeof(char));
    
    char* right_s = as_f(ast->right, list);
    char* left_s = as_f(ast->left, list);

    // Both left_s and right_s now leave their results in EAX
    const char* template = 
        "%s"                  // Left side result into EAX
        "pushl %%eax\n"       // Save left side
        "%s"                  // Right side result into EAX
        "movl %%eax, %%ebx\n" // Move right side to EBX
        "popl %%eax\n";       // Restore left side to EAX

    s = realloc(s, (strlen(right_s) + strlen(left_s) + strlen(template) + 512) * sizeof(char));
    sprintf(s, template, left_s, right_s);

    // strcat uses single % because it isn't formatted
    if (strcmp(ast->op, "+") == 0) strcat(s, "addl %ebx, %eax\n");
    else if (strcmp(ast->op, "-") == 0) strcat(s, "subl %ebx, %eax\n");
    else if (strcmp(ast->op, "*") == 0) strcat(s, "imull %ebx, %eax\n");
    
    free(right_s);
    free(left_s);
    return s;
}

char* as_f_while(AST_T* ast, list_T* list) {
    int label = label_count++;
    
    char* condition_s = as_f(ast->value, list); 
    char* body_s = as_f(ast->left, list);      

    char* s = calloc(strlen(condition_s) + strlen(body_s) + 512, sizeof(char));

    sprintf(s, 
        ".L_WHILE_START_%d:\n"
        "%s"                // Evaluate condition (leaves result in EAX)
        "cmpl $0, %%eax\n"  // Is it zero (false)?
        "je .L_WHILE_END_%d\n"
        "%s"                // Body
        "jmp .L_WHILE_START_%d\n"
        ".L_WHILE_END_%d:\n",
        label, condition_s, label, body_s, label, label);

    free(condition_s);
    free(body_s);
    return s;
}

char* as_f_assignment(AST_T* ast, list_T* list)
{ 
  char *s = calloc(1, sizeof(char));
  
  if (ast->value->type == AST_FUNCTION)
  {
    current_local_offset = -4; // Reset offset for the new stack frame!

    const char* template = ".globl %s\n"
                           "%s:\n"
                           "pushl %%ebp\n"
                           "movl %%esp, %%ebp\n";
    s = realloc(s, (strlen(template) + (strlen(ast->name) * 2) + 1) * sizeof(char));
    sprintf(s, template, ast->name, ast->name);

    AST_T* as_val = ast->value;
    for(unsigned int i = 0; i < as_val->children->size; i++)
    {
      AST_T* farg = (AST_T*) as_val->children->items[i];
      AST_T* arg_variable = init_ast(AST_VARIABLE);
      arg_variable->name = farg->name;
      arg_variable->int_value = (4 * as_val->children->size) - (i * 4);
      list_push(list, arg_variable);
    }

    char* as_val_val = as_f(as_val->value, list);
    s = realloc(s, (strlen(s) + strlen(as_val_val) + 1) * sizeof(char));
    strcat(s, as_val_val);
    free(as_val_val);
  }
  else 
  {
    char* val_s = as_f(ast->value, list);
    AST_T* existing_var = var_lookup(list, ast->name);

    if (existing_var) {
        // Reassign existing variable: overwrite the memory directly
        const char* template = "%s"
                               "movl %%eax, %d(%%ebp)\n";
        s = realloc(s, (strlen(template) + strlen(val_s) + 64) * sizeof(char));
        sprintf(s, template, val_s, existing_var->int_value);
    } else {
        // First time declaring variable: Push to stack
        const char* template = "%s"
                               "pushl %%eax\n";
        s = realloc(s, (strlen(template) + strlen(val_s) + 64) * sizeof(char));
        sprintf(s, template, val_s);

        AST_T* local_var = init_ast(AST_VARIABLE);
        local_var->name = ast->name;
        local_var->int_value = current_local_offset;
        current_local_offset -= 4; // Move offset down for the next variable
        
        list_push(list, local_var);
    }
    free(val_s);
  }

  return s;
}

char* as_f_variable(AST_T* ast, list_T* list) 
{
  char* s = calloc(1, sizeof(char));
  AST_T* var = var_lookup(list, ast->name);

  if(!var)
  {
    printf("[AS Frontend]: `%s` is not defined.\n", ast->name);
    exit(1);
  }

  // Changed %esp to %ebp so variable lookups are rock solid
  const char* template = "movl %d(%%ebp), %%eax\n"; 
  s = realloc(s, (strlen(template) + 32) * sizeof(char));
  sprintf(s, template, var->int_value);

  return s;
}

char* as_f_int(AST_T* ast, list_T* list)
{
  // Standardization: Load the integer directly into EAX
  const char* template = "movl $%d, %%eax\n";
  char* s = calloc(strlen(template) + 128, sizeof(char));
  sprintf(s, template, ast->int_value);

  return s;
}

const char* asm_builtins = 
"builtin_print_int:\n"
"    pushl %ebp\n"
"    movl %esp, %ebp\n"
"    subl $16, %esp\n"            
"    movl 8(%ebp), %eax\n"        
"    movl $10, %esi\n"
"    leal 15(%esp), %edi\n"       
"    movb $10, (%edi)\n"          
"    decl %edi\n"
".L_convert_loop:\n"
"    xorl %edx, %edx\n"
"    divl %esi\n"                 
"    addb $48, %dl\n"             
"    movb %dl, (%edi)\n"
"    decl %edi\n"
"    testl %eax, %eax\n"
"    jnz .L_convert_loop\n"
"    incl %edi\n"                 
"    movl $4, %eax\n"             
"    movl $1, %ebx\n"             
"    movl %edi, %ecx\n"           
"    leal 16(%esp), %edx\n"       
"    subl %edi, %edx\n"
"    int $0x80\n"
"    addl $16, %esp\n"
"    popl %ebp\n"
"    ret\n";

char* as_f_call(AST_T* ast, list_T* list)
{
  char* s = calloc(1, sizeof(char));
  AST_T* first_arg = NULL;
  
  if (ast->value != NULL && ast->value->children != NULL && ast->value->children->size > 0) {
      first_arg = (AST_T*) ast->value->children->items[0];
  } else if (ast->children != NULL && ast->children->size > 0) {
      first_arg = (AST_T*) ast->children->items[0];
  } else if (ast->value != NULL) {
      first_arg = ast->value; 
  }

  if (first_arg == NULL) {
      printf("[Backend Error]: '%s' expects an argument but got NULL!\n", ast->name);
      exit(1);
  }

  char* val_s = as_f(first_arg, list);

  if (strcmp(ast->name, "return") == 0)
  {
    const char* template = "%s"
                           "movl %%ebp, %%esp\n"
                           "popl %%ebp\n"
                           "ret\n";
    s = realloc(s, (strlen(template) + strlen(val_s) + 64) * sizeof(char));
    sprintf(s, template, val_s);
  }
  else if (strcmp(ast->name, "print") == 0)
  {
    // val_s evaluates the argument into EAX, so we just push it!
    const char* template = "%s" 
                           "pushl %%eax\n"    
                           "call builtin_print_int\n"
                           "addl $4, %%esp\n"; 
    
    s = realloc(s, (strlen(template) + strlen(val_s) + 64) * sizeof(char));
    sprintf(s, template, val_s);
  }
  
  free(val_s); 
  return s;
}

char* as_f_root(AST_T* ast, list_T* list)
{
  // Fix: Because strcpy is used below, these MUST be a single % 
  const char* section_text = ".section .text\n"
                            ".globl _start\n"
                            "_start:\n"
                            "pushl 0(%esp)\n" // Push argc
                            "pushl 4(%esp)\n" // Push argv
                            "call main\n"
                            "addl $4, %esp\n"
                            "movl %eax, %ebx\n" // Put exit code in EBX
                            "movl $1, %eax\n"    // sys_exit
                            "int $0x80\n\n";

  char* user_code = as_f(ast, list);
  
  size_t total_size = strlen(section_text) + strlen(user_code) + strlen(asm_builtins) + 1;
  char* value = (char*) calloc(total_size, sizeof(char));
  
  strcpy(value, section_text);
  strcat(value, user_code);
  strcat(value, asm_builtins);

  free(user_code);
  return value;
}

char* as_f_access(AST_T* ast, list_T* list)
{
  AST_T* left = var_lookup(list, ast->name);
  char* left_as = as_f(left, list);
  AST_T* first_arg = (AST_T*) (ast->value && ast->value->children->size) ? ast->value->children->items[0] : (void*) 0;
  
  // Left evaluates into EAX, so we read the offset relative to EAX
  const char* template = "%s"
                         "movl %d(%%eax), %%eax\n";

  char* s = calloc(strlen(template) + strlen(left_as) + 128, sizeof(char));
  sprintf(s, template, left_as, (first_arg ? first_arg->int_value : 0) * 4);
 
  free(left_as);
  return s;
}

char* as_f(AST_T* ast, list_T* list) {    
  if (!ast) return calloc(1, sizeof(char));
  
  switch (ast->type) {
    case AST_COMPOUND:   return as_f_compound(ast, list);
    case AST_ASSIGNMENT: return as_f_assignment(ast, list);
    case AST_VARIABLE:   return as_f_variable(ast, list);
    case AST_CALL:       return as_f_call(ast, list);
    case AST_INT:        return as_f_int(ast, list);
    case AST_BINOP:      return as_f_binop(ast, list);
    case AST_WHILE:      return as_f_while(ast, list);
    default: { printf("[As frontend]: No implementation for type `%d`\n", ast->type); exit(1); }
  }
}