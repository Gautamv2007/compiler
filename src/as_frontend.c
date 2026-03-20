#include "include/as_frontend.h"
#include "include/AST.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// --- CONSTANT PROPAGATION NOTEPAD ---
char* tracked_vars[100];
int tracked_vals[100];
int tracked_count = 0;
// ------------------------------------

static int label_count = 0;
static int current_local_offset = -4; // Start at -4 for the first local variable

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
    char* op_s = calloc(512, sizeof(char));

    if (strcmp(ast->op, "&&") == 0) {
        sprintf(op_s, 
            "cmp $0, %%ebx\n"      // Check if Left (%ebx) is 0
            "setne %%bl\n"         // Set %bl to 1 if NOT equal to 0
            "movzbl %%bl, %%ebx\n" // Zero-extend %bl to the full %ebx register

            "cmp $0, %%eax\n"      // Check if Right (%eax) is 0
            "setne %%al\n"         // Set %al to 1 if NOT equal to 0
            "movzbl %%al, %%eax\n" // Zero-extend %al to the full %eax register

            "andl %%ebx, %%eax\n"  // Now do the AND. Result (0 or 1) is in %eax!
        );
    } 
    else if (strcmp(ast->op, "||") == 0) {
        sprintf(op_s, 
            "cmp $0, %%ebx\n"
            "setne %%bl\n"
            "movzbl %%bl, %%ebx\n"

            "cmp $0, %%eax\n"
            "setne %%al\n"
            "movzbl %%al, %%eax\n"

            "orl %%ebx, %%eax\n"   // Now do the OR. Result (0 or 1) is in %eax!
        );
    }
    else if (strcmp(ast->op, "+") == 0) {
        strcat(s, "addl %ebx, %eax\n");
    } 
    else if (strcmp(ast->op, "-") == 0) {
        strcat(s, "subl %ebx, %eax\n");
    } 
    else if (strcmp(ast->op, "*") == 0) {
        strcat(s, "imull %ebx, %eax\n");
    }
    else if (strcmp(ast->op, "/") == 0) {
      // --- THE REAL Anti-Div-By-Zero Guardrail ---
      // We check the actual integer value instead of the name pointer!
      if (ast->right->type == AST_INT && ast->right->int_value == 0) 
      {
          printf("\n[Math Error]: Division by literal zero detected!\n");
          printf("  -> You are trying to divide by '0'.\n");
          exit(1);
      }

      // Check 2: Did they use a variable that we KNOW is zero? (e.g., 10 / y)
      if (ast->right->type == AST_VARIABLE && ast->right->name != NULL) 
      {
          // Look up the variable in our notepad
          for (int i = 0; i < tracked_count; i++) {
              if (strcmp(tracked_vars[i], ast->right->name) == 0) {
                  // We found it! Is the value zero?
                  if (tracked_vals[i] == 0) {
                      printf("\n[Math Error]: Division by zero detected via variable '%s'!\n", ast->right->name);
                      printf("  -> My Constant Propagation engine tracked '%s' and knows it currently holds '0'.\n", ast->right->name);
                      exit(1);
                  }
                  break; // We found the variable, no need to keep searching
              }
          }
      }
      // -------------------------------------------

      // Generate assembly for left and right sides
      char* left_val = as_f(ast->left, list);
      char* right_val = as_f(ast->right, list);

      // --- NEW: Run-Time Guardrail ---
      // We use a static counter so every division gets a unique label!
      static int div_count = 0; 
      div_count++;

      // Assembly template:
      const char* template = 
          "%s"                   // 1. Evaluate left side (puts result in %eax)
          "pushl %%eax\n"        // 2. Save dividend on the stack
          "%s"                   // 3. Evaluate right side (puts result in %eax)
          "movl %%eax, %%ebx\n"  // 4. Move right side to %ebx (the divisor)
          "popl %%eax\n"         // 5. Restore left side to %eax (the dividend)
          
          // RUN-TIME SAFETY CHECK
          "cmpl $0, %%ebx\n"               // Is the divisor zero?
          "jne .L_safe_div_%d\n"           // If NOT zero, jump down to the safe math
          "movl $1, %%eax\n"               // If IS zero, prepare sys_exit (1)
          "movl $1, %%ebx\n"               // Exit code 1 (clean crash)
          "int $0x80\n"                    // Call Linux to kill the program
          ".L_safe_div_%d:\n"              // <-- The safe jump target!
          
          // The actual division
          "cdq\n"                // Sign-extend %eax into %edx (required for idivl)
          "idivl %%ebx\n";       // Divide %edx:%eax by %ebx. Result goes in %eax!

      // Calculate memory needed for the string (adding room for our %d integers)
      s = realloc(s, (strlen(template) + strlen(left_val) + strlen(right_val) + 64) * sizeof(char));
      
      // Inject the values AND our unique division counter!
      sprintf(s, template, left_val, right_val, div_count, div_count);
      
      free(left_val);
      free(right_val);
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
    else if (strcmp(ast->op, "<=") == 0) 
    {
        char* left_val = as_f(ast->left, list);
        char* right_val = as_f(ast->right, list);

        const char* template = 
            "%s"                   // 1. Evaluate left side
            "pushl %%eax\n"        // 2. Save left side on stack
            "%s"                   // 3. Evaluate right side
            "movl %%eax, %%ebx\n"  // 4. Move right side to %ebx
            "popl %%eax\n"         // 5. Pop left side back into %eax
            
            "cmpl %%ebx, %%eax\n"  // 6. Compare %eax (left) with %ebx (right)
            "setle %%al\n"         // 7. Set %al to 1 if Less or Equal, else 0
            "movzbl %%al, %%eax\n";// 8. Zero out the rest of %eax so it's a clean 1 or 0
            
        s = realloc(s, (strlen(template) + strlen(left_val) + strlen(right_val) + 1) * sizeof(char));
        sprintf(s, template, left_val, right_val);
        free(left_val); free(right_val);
    }

    // --- GREATER THAN OR EQUAL TO (>=) ---
    else if (strcmp(ast->op, ">=") == 0) 
    {
        char* left_val = as_f(ast->left, list);
        char* right_val = as_f(ast->right, list);

        const char* template = 
            "%s"
            "pushl %%eax\n"
            "%s"
            "movl %%eax, %%ebx\n"
            "popl %%eax\n"
            
            "cmpl %%ebx, %%eax\n"  
            "setge %%al\n"         // <-- ONLY DIFFERENCE: 'setge' (Set if Greater or Equal)
            "movzbl %%al, %%eax\n";
          
        s = realloc(s, (strlen(template) + strlen(left_val) + strlen(right_val) + 1) * sizeof(char));
        sprintf(s, template, left_val, right_val);
        free(left_val); free(right_val);
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


char* as_f_for(AST_T* ast, list_T* list) {
    static int for_label_count = 0;
    int label = for_label_count++;
    
    // --- NEW OFFSET CALCULATION ---
    int offset = 0;
    int found = 0;
    
    // Loop through the backend's variable list to find 'i'
    for (int i = 0; i < list->size; i++) {
        AST_T* v = (AST_T*)list->items[i];
        if (v->name && strcmp(v->name, ast->name) == 0) {
            // Found it! Calculate x86 stack offset: -4, -8, -12, etc.
            offset = (i + 1) * -4; 
            found = 1;
            break;
        }
    }

    if (!found) {
        printf("[Compiler Error]: For-loop variable '%s' is not defined!\n", ast->name);
        exit(1);
    }
    
    // // Debug print to verify it worked!
    // printf("[DEBUG BACKEND]: Loop variable '%s' is safely at memory offset: %d\n", ast->name, offset);
    // // ------------------------------

    // 2. Generate assembly for the pieces
    char* start_s = as_f(ast->left, list);       
    char* end_s = as_f(ast->right, list);        
    char* increment_s = as_f(ast->value, list);  
    char* body_s = as_f_compound(ast, list);     

    // 3. Determine if we are counting UP or DOWN
    // We default to jge (Jump if Greater or Equal) for positive increments
    char* jump_instruction = "jge"; 
    
    // If the increment is a negative integer (e.g., -1), switch to jle (Jump if Less or Equal)
    if (ast->value && ast->value->type == AST_INT && ast->value->int_value < 0) {
        jump_instruction = "jle";
    }

    // 4. Allocate memory and stitch it together
    char* s = calloc(strlen(start_s) + strlen(end_s) + strlen(body_s) + strlen(increment_s) + 1024, sizeof(char));

    sprintf(s, 
        // --- INITIALIZATION ---
        "%s"                            
        "movl %%eax, %d(%%ebp)\n"       

        // --- LOOP START LABEL ---
        ".L_FOR_START_%d:\n"

        // --- CONDITION CHECK ---
        "%s"                            
        "cmpl %%eax, %d(%%ebp)\n"       
        "%s .L_FOR_END_%d\n"            // <-- NEW: Injects 'jge' or 'jle' dynamically!

        // --- LOOP BODY ---
        "%s"                            

        // --- INCREMENT ---
        "%s"                            
        "addl %%eax, %d(%%ebp)\n"       // <-- IF LOOP IS STUCK, 'offset' IS LIKELY WRONG HERE

        // --- REPEAT ---
        "jmp .L_FOR_START_%d\n"         

        // --- LOOP EXIT LABEL ---
        ".L_FOR_END_%d:\n",             

        // Variables mapped exactly to the %s and %d slots above:
        start_s, offset, 
        label, 
        end_s, offset, 
        jump_instruction, label,        // The dynamic jump!
        body_s, 
        increment_s, offset, 
        label, 
        label
    );

    free(start_s); free(end_s); free(increment_s); free(body_s);
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

// printf("[DEBUG Backend]: Address: %p | '%s' Opcode received is: %d\n", (void*)ast, ast->name, ast->int_value);  
  if (ast->value->type == AST_FUNCTION)
  {
    // ... [Your existing function definition logic stays exactly the same!] ...
    current_local_offset = -4; 
    AST_T* as_val = ast->value;

    if (as_val->children) {
        for(unsigned int i = 0; i < as_val->children->size; i++)
        {
          AST_T* farg = (AST_T*) as_val->children->items[i];
          if (!var_lookup(list, farg->name)) {
              AST_T* arg_variable = init_ast(AST_VARIABLE);
              arg_variable->name = farg->name;
              arg_variable->int_value = 8 + (i * 4); 
              arg_variable->data_type = farg->data_type; 
              list_push(list, arg_variable);
          }
        }
    }

    hoist_local_variables(ast->value, list);
    int stack_space_needed = (-current_local_offset) - 4; 

    const char* template = ".globl %s\n"
                           "%s:\n"
                           "pushl %%ebp\n"
                           "movl %%esp, %%ebp\n"
                           "subl $%d, %%esp\n"; 
    s = realloc(s, (strlen(template) + (strlen(ast->name) * 2) + 64) * sizeof(char));
    sprintf(s, template, ast->name, ast->name, stack_space_needed);


    // --- MISSING RETURN GUARDRAIL ---
    AST_T* func_body = as_val->value; // This is the AST_COMPOUND block (the "{ ... }")

    if (func_body && func_body->children && func_body->children->size > 0) 
    {
        // 1. Grab the very last statement in the function body
        AST_T* last_stmt = (AST_T*) func_body->children->items[func_body->children->size - 1];

        // 2. Check for return in two ways:
        //    - As a generic statement (return;)
        //    - As a function-style call (return 0;)
        int is_return_stmt = (last_stmt->type == AST_STATEMENT && 
                              last_stmt->name != NULL && 
                              strcmp(last_stmt->name, "return") == 0);

        int is_return_call = (last_stmt->type == AST_CALL && 
                              last_stmt->name != NULL && 
                              strcmp(last_stmt->name, "return") == 0);

        int has_return = is_return_stmt || is_return_call;

        // 3. Validation Logic
        if (!has_return) {
            // Check if it's a "void" function
            AST_T* first_arg = (as_val->children && as_val->children->size > 0) ? 
                               (AST_T*)as_val->children->items[0] : NULL;

            int is_void = (first_arg && first_arg->name && strcmp(first_arg->name, "void") == 0);

            if (!is_void) {
                // DEBUG: If you're still getting the error, uncomment this to see why:
                // printf("[DEBUG]: Last node type was %d, name: %s\n", last_stmt->type, last_stmt->name);
                
                printf("\n[Compiler Error]: Missing return value in function '%s'!\n", ast->name);
                exit(1);
            }
        }
    }
    // --------------------------------

    // 4. Generate Function Body
        char* as_val_val = as_f(as_val->value, list);
        
        // --- NEW: THE VOID SAFETY NET ---
        // This safely closes the stack frame and returns, preventing Segfaults
        // if the user drops off the edge of a void function.
        const char* void_epilogue = "\n"
                                    "movl %ebp, %esp\n"
                                    "popl %ebp\n"
                                    "ret\n";
                                    
        // Allocate space for the body AND our new safety net
        s = realloc(s, (strlen(s) + strlen(as_val_val) + strlen(void_epilogue) + 1) * sizeof(char));
        
        strcat(s, as_val_val);
        strcat(s, void_epilogue); // Inject the safety net at the very bottom!
        // --------------------------------
        
        free(as_val_val);
    }
    else 
    {
        AST_T* existing_var = var_lookup(list, ast->name);
        
        if (!existing_var) {
            printf("Compiler Error: Variable '%s' was not found or hoisted!\n", ast->name);
            exit(1); 
        }

        // --- UPDATED: CONSTANT PROPAGATION NOTEPAD LOGIC ---
        // We only track the value if they are assigning a literal integer!
        if (ast->value != NULL && ast->value->type == AST_INT) 
        {
            int found = 0;
            for (int i = 0; i < tracked_count; i++) {
                if (strcmp(tracked_vars[i], ast->name) == 0) {
                    
                    int op_code = ast->int_value; // Grab our opcode!

                    // Do the actual math in the notepad!
                    if (op_code == 0 || op_code == 1) { // 1 is '='
                        tracked_vals[i] = ast->value->int_value; 
                    } 
                    else if (op_code == 2) { // 2 is '+='
                        tracked_vals[i] += ast->value->int_value;
                    } 
                    else if (op_code == 3) { // 3 is '-='
                        tracked_vals[i] -= ast->value->int_value;
                    } 
                    else if (op_code == 4) { // 4 is '*='
                        tracked_vals[i] *= ast->value->int_value;
                    } 
                    else if (op_code == 5) { // 5 is '/='
                        if (ast->value->int_value != 0) { 
                            tracked_vals[i] /= ast->value->int_value;
                        }
                    }

                    found = 1;
                    break;
                }
            }
            // 2. If it is a new variable, add it to the notepad
            if (!found) {
                tracked_vars[tracked_count] = ast->name;
                tracked_vals[tracked_count] = ast->value->int_value;
                tracked_count++;
            }
        }
        // -----------------------------------------------

        char* val_s = as_f(ast->value, list);
        const char* template;

        // --- NEW: BACKEND ASSEMBLY GENERATION FOR COMPOUND ASSIGNMENTS ---
        // Standard Assignment (=)
        int op_code = ast->int_value; // Remember, we stored the operator code in int_value during parsing!

        // printf("[DEBUG Backend]: Compiling assignment for '%s'. Operator is: '%s'\n", ast->name, ast->op ? ast->op : "NULL");
        if (op_code == 0 || op_code == 1) {
            template = "%s"                            
                    "movl %%eax, %d(%%ebp)\n";      
        } 
        // 2: Plus-Equals (+=)
        else if (op_code == 2) {
            template = "%s"                            
                    "addl %%eax, %d(%%ebp)\n";      
        }
        // 3: Minus-Equals (-=)
        else if (op_code == 3) {
            template = "%s"                            
                    "subl %%eax, %d(%%ebp)\n";      
        }
        // 4: Times-Equals (*=)
        else if (op_code == 4) {
            template = "%s"                            
                    "imull %d(%%ebp), %%eax\n"      
                    "movl %%eax, %d(%%ebp)\n";      
        }
        // 5: Divide-Equals (/=)
        else if (op_code == 5) {
            template = "%s"                            
                    "movl %%eax, %%ebx\n"           
                    "movl %d(%%ebp), %%eax\n"       
                    "cdq\n"                         
                    "idivl %%ebx\n"                 
                    "movl %%eax, %d(%%ebp)\n";      
        }

        else if (op_code == 6) {
            int found = 0;
            for (int i = 0; i < tracked_count; i++) {
                if (strcmp(tracked_vars[i], ast->name) == 0) {
                    if (ast->value->int_value != 0) {
                        // UPDATE the existing value at index i
                        tracked_vals[i] %= ast->value->int_value;
                    }
                    found = 1;
                    break;
                }
            }
            template = "%s"                            
                    "movl %%eax, %%ebx\n"           
                    "movl %d(%%ebp), %%eax\n"       
                    "cdq\n"                         
                    "idivl %%ebx\n"                 
                    "movl %%edx, %d(%%ebp)\n";      
        }

        s = realloc(s, (strlen(template) + strlen(val_s) + 64) * sizeof(char));
        sprintf(s, template, val_s, existing_var->int_value, existing_var->int_value);
        // -----------------------------------------------------------------

        free(val_s);
    }
    return s;
}


char* as_f_variable(AST_T* ast, list_T* list) 
{
  AST_T* var = var_lookup(list, ast->name);

  if(!var) {
    printf("[AS Frontend]: `%s` is not defined.\n", ast->name);
    exit(1);
  }

  // CRITICAL: Ensure we use the parentheses () around %ebp
  // This tells the CPU: "Go to this memory address and get the VALUE."
  const char* template = "    movl %d(%%ebp), %%eax\n"; 
  char* s = calloc(strlen(template) + 32, sizeof(char));
  
  // var->int_value should be -4, -8, etc.
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
// --- NEW: Print Integer (No Newline, with negative handling) ---
"builtin_print_int:\n"
"    pushl %ebp\n"
"    movl %esp, %ebp\n"
"    subl $16, %esp\n"            
"    movl 8(%ebp), %eax\n"        
"    movl %eax, %ecx\n"           // NEW: Save a copy of the original number to check the sign later
"    testl %eax, %eax\n"          // NEW: Is the number negative?
"    jns .L_is_positive\n"        // NEW: If positive (or zero), jump over the negation
"    negl %eax\n"                 // NEW: Flip the bits to make it a positive number!
".L_is_positive:\n"
"    movl $10, %esi\n"
"    leal 16(%esp), %edi\n"       // Start at the very end of the 16-byte buffer
"    decl %edi\n"                 
".L_convert_loop:\n"
"    xorl %edx, %edx\n"
"    divl %esi\n"                 
"    addb $48, %dl\n"             
"    movb %dl, (%edi)\n"
"    decl %edi\n"
"    testl %eax, %eax\n"
"    jnz .L_convert_loop\n"
"    testl %ecx, %ecx\n"          // NEW: Was the ORIGINAL number negative?
"    jns .L_skip_minus\n"         // NEW: If it was positive, skip adding the minus sign
"    movb $45, (%edi)\n"          // NEW: Put a '-' (ASCII 45) in the buffer right before the digits!
"    decl %edi\n"                 // NEW: Step back one more byte so 'incl %edi' works correctly
".L_skip_minus:\n"
"    incl %edi\n"                 
"    movl $4, %eax\n"             
"    movl $1, %ebx\n"             
"    movl %edi, %ecx\n"           // (This safely overwrites our sign copy, we don't need it anymore)
"    leal 16(%esp), %edx\n"       
"    subl %edi, %edx\n"           // Because we added the '-', this length calculation automatically includes it!
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
"    pushl %esi\n"                        
"    movl current_input_ptr, %esi\n"      
"    movl %esi, %ecx\n"                   

// --- Phase 1: Skip leading whitespace (spaces, tabs, newlines) ---
".L_skip_ws:\n"
"    movl $3, %eax\n"                     // sys_read
"    movl $0, %ebx\n"                     // stdin
"    movl $1, %edx\n"                     // READ EXACTLY 1 BYTE
"    int $0x80\n"
"    cmpl $1, %eax\n"                     
"    jne .L_read_done\n"                  // EOF or Error
"    cmpb $32, (%ecx)\n"                  // Is the char <= 32? (Space is 32, \n is 10, \t is 9)
"    jle .L_skip_ws\n"                    // If it's whitespace, loop and overwrite it!

// --- Phase 2: Read the actual word/number ---
"    incl %ecx\n"                         // We found a real character! Move pointer forward.
".L_read_word:\n"
"    movl $3, %eax\n"                     
"    movl $0, %ebx\n"                     
"    movl $1, %edx\n"                     
"    int $0x80\n"
"    cmpl $1, %eax\n"                     
"    jne .L_read_done\n"                  // EOF
"    cmpb $32, (%ecx)\n"                  // Is it whitespace? (Space/Newline)
"    jle .L_read_done\n"                  // YES! We reached the end of the word. Break loop.
"    incl %ecx\n"                         // NO! It's part of the word. Keep it and move forward.
"    jmp .L_read_word\n"                  

".L_read_done:\n"
"    movb $0, (%ecx)\n"                   // Null-terminate the string
"    incl %ecx\n"                         
"    movl %ecx, current_input_ptr\n"      // Save pointer for next time
"    movl %esi, %eax\n"                   // Return the string address
"    popl %esi\n"
"    popl %ebp\n"
"    ret\n"

// Add this right below your current "builtin_input" block
".globl input_line\n"
"builtin_input_line:\n"
"    pushl %ebp\n"
"    movl %esp, %ebp\n"
"    pushl %esi\n"
"    movl current_input_ptr, %esi\n"      
"    movl %esi, %ecx\n"                   
".L_line_read_loop:\n"
"    movl $3, %eax\n"                     // sys_read
"    movl $0, %ebx\n"                     // stdin
"    movl $1, %edx\n"                     // read 1 byte
"    int $0x80\n"
"    cmpl $1, %eax\n"                     
"    jne .L_line_read_done\n"             // EOF or Error
"    cmpb $10, (%ecx)\n"                  // Is it exactly a Newline (\n)?
"    je .L_line_read_done\n"              // YES! Stop reading.
"    incl %ecx\n"                         // NO! It's a space/letter. Keep it.
"    jmp .L_line_read_loop\n"             
".L_line_read_done:\n"
"    movb $0, (%ecx)\n"                   // Null-terminate
"    incl %ecx\n"                         
"    movl %ecx, current_input_ptr\n"      // Save pointer
"    movl %esi, %eax\n"                   // Return string address
"    popl %esi\n"
"    popl %ebp\n"
"    ret\n"

// --- Convert String to Int ---
"builtin_to_int:\n"
"    pushl %ebp\n"
"    movl %esp, %ebp\n"
"    pushl %esi\n"               // Save %esi
"    pushl %ebx\n"               // Save %ebx
"    movl 8(%ebp), %esi\n"       // Get the string pointer from the stack
"    xorl %eax, %eax\n"          // %eax will be our final result. Set to 0.
"    xorl %ebx, %ebx\n"          // %ebx will hold the current character. Set to 0.
"    xorl %ecx, %ecx\n"          // %ecx will be our 'is negative' flag. Set to 0.

// 1. Check if the string starts with a minus sign '-'
"    movb (%esi), %bl\n"         // Read the first character
"    cmpb $45, %bl\n"            // Is it '-' (ASCII 45)?
"    jne .L_to_int_loop\n"       // If not, jump straight to the loop
"    movl $1, %ecx\n"            // It IS negative! Set our flag to 1
"    incl %esi\n"                // Move the pointer past the '-' character

// 2. The Conversion Loop
".L_to_int_loop:\n"
"    movb (%esi), %bl\n"         // Read the next character
"    testb %bl, %bl\n"           // Is it the null terminator (0)?
"    jz .L_to_int_done\n"        // If yes, we are done!
"    cmpb $10, %bl\n"            // Is it a newline (\n)? (From input_line)
"    je .L_to_int_done\n"        // If yes, we are done!

"    cmpb $48, %bl\n"            // Is it less than '0'?
"    jl .L_to_int_skip\n"        // Invalid char, skip it
"    cmpb $57, %bl\n"            // Is it greater than '9'?
"    jg .L_to_int_skip\n"        // Invalid char, skip it

// It is a valid number! Math time: (Result * 10) + (char - '0')
"    subb $48, %bl\n"            // Convert ASCII character to real number ('5' -> 5)
"    imull $10, %eax\n"          // Multiply current total by 10
"    addl %ebx, %eax\n"          // Add the new digit

".L_to_int_skip:\n"
"    incl %esi\n"                // Move pointer to the next character
"    jmp .L_to_int_loop\n"       // Loop back around

// 3. Finalize and Return
".L_to_int_done:\n"
"    testl %ecx, %ecx\n"         // Check our negative flag
"    jz .L_to_int_exit\n"        // If it's 0, just exit
"    negl %eax\n"                // If it's 1, negate %eax to make it a negative number!

".L_to_int_exit:\n"
"    popl %ebx\n"                // Restore registers
"    popl %esi\n"
"    movl %ebp, %esp\n"
"    popl %ebp\n"
"    ret\n";                      // Return the integer in %eax!

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
        for (int i = 0; i < args->size; i++) {
            AST_T* arg = (AST_T*) args->items[i];
            char* val_s = as_f(arg, list);
            
            int is_string = 0; 
            
            // 1. Detect if it's a literal string "hello"
            if (arg->type == AST_STRING) {
                is_string = 1; 
            } 
            // 2. Detect if it's a direct function call like print(input())
            else if (arg->type == AST_CALL && arg->name && strcmp(arg->name, "input") == 0) {
                is_string = 1;
            }
            // 3. Detect if it's a variable
            else if (arg->type == AST_VARIABLE) {
                AST_T* var = var_lookup(list, arg->name);
                if (var) {
                    // Check if defined as string (Type 2) OR if it holds a string pointer
                    if (var->data_type == 2 || var->type == AST_STRING) {
                        is_string = 1;
                    }
                }
            }
            
            // Choose the right builtin
            const char* func_to_call = is_string ? "builtin_print_str" : "builtin_print_int";

            const char* template = "%s" 
                                   "pushl %%eax\n"    
                                   "call %s\n"
                                   "addl $4, %%esp\n"; 
            
            s = realloc(s, (strlen(s) + strlen(template) + strlen(val_s) + 64) * sizeof(char));
            char* temp = calloc(strlen(template) + strlen(val_s) + 128, sizeof(char));
            
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
  else if (strcmp(ast->name, "input_line") == 0) {
    const char* template = "call builtin_input_line\n";
    
    s = realloc(s, (strlen(s) + strlen(template) + 1) * sizeof(char));
    strcat(s, template);
  }
  // Handle Python-style int(string)
  else if (strcmp(ast->name, "to_int") == 0)
  {
    AST_T* first_arg = args && args->size > 0 ? (AST_T*) args->items[0] : (ast->value ? ast->value : NULL);

    // --- NEW: The Anti-Segfault Guardrail ---
    // If the user literally typed to_int(123), stop the compiler right now!
    if (first_arg != NULL && first_arg->type == AST_INT) {
        printf("\n[Semantic Error]: Invalid argument for 'to_int()'.\n");
        printf("  -> You passed an integer literal, but it expects a string.\n");
        printf("  -> Hint: Use quotes! Did you mean \"123\" instead of 123?\n");
        exit(1); 
    }
    // ----------------------------------------

    // If it is a string, generate the assembly normally!
    char* val_s = as_f(first_arg, list);
    const char* template = "%s" 
                           "pushl %%eax\n"    
                           "call builtin_to_int\n"
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
  // For now, redirect access to variable lookup to get the raw value
  return as_f_variable(ast, list);
}

//I am using this function to find all local variables and assign them stack offsets before codegen. This way, when we encounter an assignment to a new variable anywhere in the function, we can assign it a unique stack offset and store that in the AST node. Then, when we access that variable later, we can generate the correct assembly to read/write from that stack offset.
void hoist_local_variables(AST_T* ast, list_T* list) {
    if (!ast) return;

    // 1. If we find an assignment, register the variable!
    if (ast->type == AST_ASSIGNMENT && ast->name) {
        if (!var_lookup(list, ast->name)) {
            AST_T* local_var = init_ast(AST_VARIABLE);
            local_var->name = ast->name;
            local_var->int_value = current_local_offset; 
            current_local_offset -= 4; 
            
            // --- NEW: Smart Type Inference ---
            local_var->data_type = ast->data_type; // Default to what the user asked for
            
            // Override: If assigning from input(), force it to be a String (Type 2)
            if (ast->value && ast->value->type == AST_CALL && ast->value->name) {
                if (strcmp(ast->value->name, "input") == 0 || strcmp(ast->value->name, "input_line") == 0) {
                    local_var->data_type = 2; // 2 = String
                }
            }
            // Optional: If assigning directly to a literal string (msg = "hello")
            else if (ast->value && ast->value->type == AST_STRING) {
                local_var->data_type = 2;
            }
            // ---------------------------------
            
            list_push(list, local_var);
        }
    }

    // 2. Traverse down all possible AST branches to find hidden assignments
    if (ast->value) hoist_local_variables(ast->value, list);
    
    // (If your AST_T struct uses left/right for binary trees or loop conditions)
    if (ast->left) hoist_local_variables(ast->left, list);
    if (ast->right) hoist_local_variables(ast->right, list);

    // 3. Traverse down into 'children' (Block statements)
    if (ast->children) {
        for (unsigned int i = 0; i < ast->children->size; i++) {
            hoist_local_variables((AST_T*)ast->children->items[i], list);
        }
    }
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
    case AST_FOR:        return as_f_for(ast, list);
    case AST_IF:         return as_f_if(ast, list);

    case AST_STRING:     return as_f_string(ast, list); 
    default: { printf("[As frontend]: No implementation for type `%d`\n", ast->type); exit(1); }
  }
}

