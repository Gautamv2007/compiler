#include "include/as_frontend.h"
#include "include/AST.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int label_count = 0;
static int current_local_offset = -4;

static int string_count = 0;
char* string_data_section = NULL;

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

//For string function 
char* as_f_string(AST_T* ast, list_T* list) {
    int label = string_count++;
    
    // 1. Add string to the global data section (e.g., .L_STR_0: .asciz "Hello World")
    // .asciz automatically adds a null-terminator (\0) to the end of the string!
    const char* data_template = ".L_STR_%d:\n    .asciz \"%s\"\n";
    
    char* new_data = calloc(strlen(data_template) + strlen(ast->string_value) + 32, sizeof(char));
    sprintf(new_data, data_template, label, ast->string_value);
    
    if (!string_data_section) {
        string_data_section = calloc(1, sizeof(char));
    }
    string_data_section = realloc(string_data_section, strlen(string_data_section) + strlen(new_data) + 1);
    strcat(string_data_section, new_data);
    free(new_data);

    // 2. Return the assembly to load the string's memory pointer into EAX
    // Notice the '$' before the label. We want the memory ADDRESS, not the value at the address.
    const char* text_template = "movl $.L_STR_%d, %%eax\n";
    char* s = calloc(strlen(text_template) + 32, sizeof(char));
    sprintf(s, text_template, label);

    return s;
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

    // Math Operators
    if (strcmp(ast->op, "+") == 0) {
        strcat(s, "addl %ebx, %eax\n");
    } 
    else if (strcmp(ast->op, "-") == 0) {
        strcat(s, "subl %ebx, %eax\n");
    } 
    else if (strcmp(ast->op, "*") == 0) {
        strcat(s, "imull %ebx, %eax\n");
    }
    else if (strcmp(ast->op, "/") == 0) {
        strcat(s, "cdq\n"
                  "idivl %ebx\n");
    }
    else if (strcmp(ast->op, "%") == 0){
        strcat(s, "cdq\n"
                  "idivl %ebx\n"
                  "movl %edx, %eax\n"); // Move remainder into EAX
    }
    // Comparison Operators (NEW)
    else if (strcmp(ast->op, "<") == 0) {
        strcat(s, "cmpl %ebx, %eax\n"
                  "setl %al\n"         // Set AL to 1 if Less
                  "movzbl %al, %eax\n"); // Zero-extend AL to EAX
    }
    else if (strcmp(ast->op, ">") == 0) {
        strcat(s, "cmpl %ebx, %eax\n"
                  "setg %al\n"         // Set AL to 1 if Greater
                  "movzbl %al, %eax\n"); 
    }
    else if (strcmp(ast->op, "==") == 0) {
        strcat(s, "cmpl %ebx, %eax\n"
                  "sete %al\n"         // Set AL to 1 if Equal
                  "movzbl %al, %eax\n"); 
    }
    else {
        printf("[Backend Error]: Unknown binary operator '%s'\n", ast->op);
        exit(1);
    }
    
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

char* as_f_if(AST_T* ast, list_T* list) {
    int label = label_count++;
    
    char* condition_s = as_f(ast->value, list); 
    char* if_body_s = as_f(ast->left, list);      
    char* else_body_s = ast->right ? as_f(ast->right, list) : calloc(1, sizeof(char));

    char* s;
    
    // If we have an ELSE block attached
    if (ast->right) {
        s = calloc(strlen(condition_s) + strlen(if_body_s) + strlen(else_body_s) + 512, sizeof(char));
        sprintf(s, 
            "%s"                 // Evaluate condition (leaves 1 or 0 in EAX)
            "cmpl $0, %%eax\n"   // Is it false (0)?
            "je .L_ELSE_%d\n"    // If false, jump to the ELSE label
            "%s"                 // Otherwise, run the IF body
            "jmp .L_IF_END_%d\n" // And jump over the ELSE block
            ".L_ELSE_%d:\n"      // <--- ELSE LABEL
            "%s"                 // Run the ELSE body
            ".L_IF_END_%d:\n",   // <--- END LABEL
            // vvv FIX is here: removed the extra 'label' argument! vvv
            condition_s, label, if_body_s, label, label, else_body_s, label);
    } 
    // If it's just a standard IF block (no else)
    else {
        s = calloc(strlen(condition_s) + strlen(if_body_s) + 512, sizeof(char));
        sprintf(s, 
            "%s"                 // Evaluate condition
            "cmpl $0, %%eax\n"   // Is it false?
            "je .L_IF_END_%d\n"  // If false, jump entirely over the IF body
            "%s"                 // Otherwise, run the IF body
            ".L_IF_END_%d:\n",   // <--- END LABEL
            condition_s, label, if_body_s, label);
    }

    free(condition_s);
    free(if_body_s);
    if (ast->right) free(else_body_s);
    
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

      // --- THE FIX IS HERE ---
      // In x86, the first argument (index 0) is always at 8(%ebp)
      // The second (index 1) is at 12(%ebp), etc.
      arg_variable->int_value = 8 + (i * 4); 
      // -----------------------


      list_push(list, arg_variable);
    }

    char* as_val_val = as_f(as_val->value, list);
    s = realloc(s, (strlen(s) + strlen(as_val_val) + 1) * sizeof(char));
    strcat(s, as_val_val);
    free(as_val_val);
  }
  else 
  {
    AST_T* existing_var = var_lookup(list, ast->name);

    // --- NEW: TYPE CHECKING GUARDRAILS ---
    // 1 is our integer data type from the parser
    if (ast->data_type == 1 && ast->value->type == AST_STRING) {
        printf("[Type Error]: Cannot assign a string to integer variable '%s'\n", ast->name);
        exit(1);
    }

    if (existing_var) {
        // Prevent assigning a string to an existing integer variable
        if (existing_var->data_type == 1 && ast->value->type == AST_STRING) {
             printf("[Type Error]: Variable '%s' is an integer, cannot reassign to string.\n", ast->name);
             exit(1);
        }
    }
    // ---------------------------------------

    char* val_s = as_f(ast->value, list);

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
        
        // --- NEW: SAVE THE DATA TYPE ---
        local_var->data_type = ast->data_type; 
        // -------------------------------

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
// --- NEW: Print String ---
"builtin_print_str:\n"
"    pushl %ebp\n"
"    movl %esp, %ebp\n"
"    movl 8(%ebp), %ecx\n"      // Load string pointer into ECX
"    movl %ecx, %edi\n"         // Copy pointer to EDI for length calculation
".L_strlen_loop:\n"
"    cmpb $0, (%edi)\n"         // Look for the null terminator (\0)
"    je .L_strlen_done\n"
"    incl %edi\n"               // Move to next character
"    jmp .L_strlen_loop\n"
".L_strlen_done:\n"
"    subl %ecx, %edi\n"         // EDI now holds the exact length of the string
"    movl $4, %eax\n"           // sys_write
"    movl $1, %ebx\n"           // stdout
"    movl %edi, %edx\n"         // Length
"    int $0x80\n"
"    popl %ebp\n"
"    ret\n"
// --- NEW: Print Integer (No Newline) ---
"builtin_print_int:\n"
"    pushl %ebp\n"
"    movl %esp, %ebp\n"
"    subl $16, %esp\n"            
"    movl 8(%ebp), %eax\n"        
"    movl $10, %esi\n"
"    leal 16(%esp), %edi\n"       // Start at the very end of the 16-byte buffer
"    decl %edi\n"                 // Step back 1 byte (We no longer write '10' here!)
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
"    subl %edi, %edx\n"           // Calculate the exact length of the digits
"    int $0x80\n"
"    addl $16, %esp\n"
"    popl %ebp\n"
"    ret\n"
// --- NEW: Dynamic Bump Allocator for Input ---
".section .bss\n"
".lcomm global_input_buffer, 1024\n"
".section .data\n"
"current_input_ptr: .long global_input_buffer\n"
".section .text\n"

"builtin_input:\n"
"    pushl %ebp\n"
"    movl %esp, %ebp\n"
"    movl $3, %eax\n"                     // sys_read
"    movl $0, %ebx\n"                     // stdin
"    movl current_input_ptr, %ecx\n"      
"    movl $255, %edx\n"                   
"    int $0x80\n"                         // Read input. EAX holds the number of bytes read.
// --- Strip Newline and Bump Allocator ---
"    movl current_input_ptr, %ebx\n"      // Save the start address in EBX
"    movl %ebx, %ecx\n"                   
"    addl %eax, %ecx\n"                   // Move ECX to the very end of what was typed
"    decl %ecx\n"                         // Step back one character (to look at the last thing typed)
"    cmpb $10, (%ecx)\n"                  // Is it a newline (\n)?
"    jne .L_no_newline\n"                 // If not, jump down
"    movb $0, (%ecx)\n"                   // YES! Overwrite the \n with a null terminator (\0)
"    incl %ecx\n"                         // Move forward 1 byte for the next allocation
"    jmp .L_save_ptr\n"
".L_no_newline:\n"
"    incl %ecx\n"                         // Step forward past the last character
"    movb $0, (%ecx)\n"                   // Add a null terminator
"    incl %ecx\n"                         // Move forward 1 byte for the next allocation
".L_save_ptr:\n"
"    movl %ecx, current_input_ptr\n"      // Save the new free-space pointer!
"    movl %ebx, %eax\n"                   // Return the original string address in EAX
"    popl %ebp\n"
"    ret\n"

// --- Convert String to Int ---
"builtin_int:\n"
"    pushl %ebp\n"
"    movl %esp, %ebp\n"
"    movl 8(%ebp), %esi\n"                // Grab string address
"    xorl %eax, %eax\n"                   
"    xorl %ebx, %ebx\n"                   
".L_int_loop:\n"
"    movb (%esi), %bl\n"                  
"    cmpb $10, %bl\n"                     // Stop at newline
"    je .L_int_done\n"
"    cmpb $0, %bl\n"                      // Stop at null terminator
"    je .L_int_done\n"
"    subb $48, %bl\n"                     
"    imull $10, %eax\n"                   
"    addl %ebx, %eax\n"                   
"    incl %esi\n"                         
"    jmp .L_int_loop\n"
".L_int_done:\n"
"    popl %ebp\n"
"    ret\n";

char* as_f_call(AST_T* ast, list_T* list)
{
  char* s = calloc(1, sizeof(char));

  // Determine where the arguments live (handles varying AST structures)
  list_T* args = NULL;
  if (ast->value && ast->value->children) {
      args = ast->value->children;
  } else if (ast->children) {
      args = ast->children;
  }

  // Handle return
  if (strcmp(ast->name, "return") == 0)
  {
    AST_T* first_arg = args && args->size > 0 ? (AST_T*) args->items[0] : (ast->value ? ast->value : NULL);
    char* val_s = as_f(first_arg, list);
    const char* template = "%s"
                           "movl %%ebp, %%esp\n"
                           "popl %%ebp\n"
                           "ret\n";
    s = realloc(s, (strlen(template) + strlen(val_s) + 64) * sizeof(char));
    sprintf(s, template, val_s);
    free(val_s);
  }
  // Handle Smart Print (Python style!)
  else if (strcmp(ast->name, "print") == 0)
  {
    if (args) {
        // Loop through every argument passed to print(arg1, arg2, arg3)
        for (int i = 0; i < args->size; i++) {
            AST_T* arg = (AST_T*) args->items[i];
            char* val_s = as_f(arg, list);
            
            // 1. Detect the data type!
            int is_string = 0; 
            
            if (arg->type == AST_STRING) {
                is_string = 1; // It's a literal "string"
            } else if (arg->type == AST_VARIABLE) {
                AST_T* var = var_lookup(list, arg->name);
                // Remember we set data_type = 2 for strings in parser.c!
                if (var && var->data_type == 2) { 
                    is_string = 1; 
                }
            }
            
            // 2. Choose the right builtin function
            const char* func_to_call = is_string ? "builtin_print_str" : "builtin_print_int";

            // 3. Generate the assembly for this specific argument
            const char* template = "%s" 
                                   "pushl %%eax\n"    
                                   "call %s\n"
                                   "addl $4, %%esp\n"; 
            
            s = realloc(s, (strlen(s) + strlen(template) + strlen(val_s) + 64) * sizeof(char));
            char* temp = calloc(strlen(template) + strlen(val_s) + 64, sizeof(char));
            
            sprintf(temp, template, val_s, func_to_call);
            strcat(s, temp);
            
            free(temp);
            free(val_s);
        }
    }
  }

  //for scanning input
  // Handle Python-style input()
  else if (strcmp(ast->name, "input") == 0)
  {
    const char* template = "call builtin_input\n";
    s = realloc(s, strlen(s) + strlen(template) + 1);
    strcat(s, template);
  }
  // Handle Python-style int(string)
  else if (strcmp(ast->name, "to_int") == 0)
  {
    AST_T* first_arg = args && args->size > 0 ? (AST_T*) args->items[0] : (ast->value ? ast->value : NULL);
    char* val_s = as_f(first_arg, list);
    const char* template = "%s" 
                           "pushl %%eax\n"    
                           "call builtin_int\n"
                           "addl $4, %%esp\n"; 
    
    s = realloc(s, (strlen(template) + strlen(val_s) + 64) * sizeof(char));
    sprintf(s, template, val_s);
    free(val_s);
  }
  // --- NEW: Handle Custom Functions ---
  else 
  {
    // 1. Evaluate and PUSH arguments in reverse order
    if (args) {
        for (int i = args->size - 1; i >= 0; i--) {
            AST_T* arg = (AST_T*) args->items[i];
            char* arg_s = as_f(arg, list);
            
            const char* push_template = "%s"
                                        "pushl %%eax\n";
            s = realloc(s, strlen(s) + strlen(arg_s) + strlen(push_template) + 1);
            
            char* temp = calloc(strlen(arg_s) + strlen(push_template) + 1, sizeof(char));
            sprintf(temp, push_template, arg_s);
            strcat(s, temp);
            
            free(temp);
            free(arg_s);
        }
    }

    // 2. Call the function and clean up the stack
    const char* call_template = "call %s\n"
                                "addl $%d, %%esp\n"; // Stack cleanup!
                                
    int cleanup_size = args ? (args->size * 4) : 0;
    
    s = realloc(s, strlen(s) + strlen(ast->name) + strlen(call_template) + 32);
    char* temp2 = calloc(strlen(ast->name) + strlen(call_template) + 32, sizeof(char));
    sprintf(temp2, call_template, ast->name, cleanup_size);
    strcat(s, temp2);
    
    free(temp2);
  }
  
  return s;
}

char* as_f_root(AST_T* ast, list_T* list)
{
  string_data_section = calloc(1, sizeof(char)); // Initialize empty
  
  char* user_code = as_f(ast, list);
  
  const char* section_data_header = ".section .data\n";
  const char* section_text_header = ".section .text\n"
                                    ".globl _start\n"
                                    "_start:\n"
                                    "pushl 0(%esp)\n"
                                    "pushl 4(%esp)\n"
                                    "call main\n"
                                    "addl $4, %esp\n"
                                    "movl %eax, %ebx\n"
                                    "movl $1, %eax\n"
                                    "int $0x80\n\n";

  size_t total_size = strlen(section_data_header) + strlen(string_data_section) + 
                      strlen(section_text_header) + strlen(user_code) + strlen(asm_builtins) + 1;
                      
  char* value = (char*) calloc(total_size, sizeof(char));
  
  // Build the file top to bottom:
  strcpy(value, section_data_header);
  strcat(value, string_data_section);
  strcat(value, section_text_header);
  strcat(value, user_code);
  strcat(value, asm_builtins);

  free(user_code);
  free(string_data_section); // Clean up!
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
    case AST_IF:         return as_f_if(ast, list);

    case AST_STRING:     return as_f_string(ast, list); 
    default: { printf("[As frontend]: No implementation for type `%d`\n", ast->type); exit(1); }
  }
}

