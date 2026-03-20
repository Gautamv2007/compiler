# GVR Compiler

> A custom-designed, statically-typed programming language implemented in C, featuring a modular frontend pipeline and direct compilation to 32-bit x86 Linux Assembly.

---

## Overview

This project demonstrates the design of a complete compiler from scratch, taking high-level, Python-inspired syntax and translating it down to bare-metal machine code without the use of virtual machines or intermediate interpreters.

The compiler supports:
- Strict static typing
- Dynamic memory allocation for strings
- A Python-style variadic `print` function
- Seamless interactive system I/O

The complete architecture was developed in C and utilizes the GNU Toolchain for assembling and linking.

---

## Highlights

✔ Compiled natively to x86 Assembly  
✔ Static typing enforcement (`int`, `str`)  
✔ Dynamic Bump Allocator for RAM management  
✔ Python-style variadic `print()`  
✔ Interactive `input()` and type-casting `to_int()`  
✔ Modular Lexer, Parser, and AST pipeline  
✔ Native Linux Kernel system calls (`int 0x80`)  
✔ Character Literal support (`'A'`, `'\n'`) 
✔ Array Indexing & Mutation (`msg[i] = 'X'`)
✔ Null-Safe Semantic Type Checking
✔ `for` loop control structures

---

## System Specifications

| Component            | Configuration                     |
|----------------------|-----------------------------------|
| Host Language        | C                                 |
| Target Architecture  | 32-bit x86 Linux                  |
| Target Assembly      | AT&T Syntax                       |
| Supported Data Types | Integer (`int`), String (`str`)   |
| Memory Management    | Custom Bump Allocator (1KB Buffer)|
| I/O Interface        | Direct Linux Syscalls             |
| Program Flow         | Sequential, Branches, Loops       |

---

The compiler consists of the following modular phases:

- **Lexical Analyzer (Lexer)** – Tokenizes raw source code  
- **Parser** – Validates grammar and constructs the Abstract Syntax Tree (AST)  
- **Semantic Analyzer (Visitor)** – Traverses the AST, tracking variable scopes and types  
- **Code Generator** – Translates AST nodes into x86 assembly instructions  
- **Assembler (`as`)** – Converts assembly into object code  
- **Linker (`ld`)** – Stitches object code into an ELF executable  

---

### 4. Expansion of "Project Structure"
Since your project has grown, adding descriptions for the internal header files makes it look like a professional API.

```markdown
### 📂 Internal Logic Mapping
- `src/visitor.c`: The heart of the **Semantic Analyzer**. It walks the AST to find type mismatches before assembly is even generated.
- `src/as_frontend.c`: The **Code Generator**. Maps AST nodes to x86 opcodes (`movl`, `pushl`, `call`).
- `src/builtins.c`: Contains the **Assembly Templates** for the runtime library.

---

## 🚀 Recent Feature Showcase: String Manipulation

GVR now supports direct character manipulation within strings.

```python
// Initializing a string
msg:str = "hello";

// Modifying an index with a character literal
msg[0] = 'J'; 

// Using the new 'for' loop syntax
i:int = 0;
for (i = 0; i < 5; i = i + 1) {
    print("Character at ", i, " is: ", msg[i]);
}
// Output: Jello
```

## 🛠 Technical Deep Dive: Freestanding Architecture

Unlike typical compilers that link against the C Standard Library (`libc`), the **GVR Compiler** is entirely freestanding. 

### Zero-Dependency Runtime
The generated binaries do not require `printf`, `malloc`, or `exit`. Instead, the compiler injects raw x86 assembly "built-ins" into the output file:
- **Custom Memory Management:** A global 1KB buffer acts as a static heap. The `Bump Allocator` increments a pointer to manage string memory.
- **Direct Kernel Communication:** I/O is handled via `int 0x80` interrupts. 
  - `EAX = 4`: `sys_write`
  - `EAX = 3`: `sys_read`
  - `EAX = 1`: `sys_exit`

### The Semantic Safety Net
The compiler performs a **Two-Pass Semantic Analysis**:
1. **Scope Resolution:** Ensures variables are declared before use.
2. **Type Consistency:** Specifically handles "Type 5" (Char) interactions, allowing characters to be assigned to string indices while blocking unsafe `int` to `str` assignments.

## Syntax & Formats

### Variable Declaration & Assignment
```typescript
name:str = "Gautam";
age:int = 18;
```

### Interactive I/O
```
print("Enter your name: ");
msg:str = input();

print("Enter your age: ");
age:int = to_int(input());
```

### Control Flow
```
if (age > 17) {
    print(msg, ", Congratulations you are eligible to vote!\n");
} else {
    print(msg, ", You still need to wait for ", (18 - age), " years.\n");
}
```

## Built-in Functions & Operations

### Standard I/O
- ```print(arg1, arg2, ...)``` – Variadic printing supporting mixed data types.
- ```input()``` – Reads input from standard input into a dynamically allocated buffer.

## Data Conversion
- ```to_int(string)``` – Converts ASCII string input into integer values.

## Control Flow
- ```if / else``` statements
- ```whil``` loops

###  System-Level Integration

## Assembly Macros
- sys_read – Reads from file descriptor 0 (stdin).
- sys_write – Writes to file descriptor 1 (stdout).
- sys_exit – Terminates the program with status 0.

## Backend Design Engine
The compiler backend integrates several optimized components:

- Bump Allocator – Efficiently stores variable-length inputs sequentially in memory without overwrites.
- String-to-Integer Converter – Implements ASCII parsing using arithmetic logic for base-10 integer generation.
- Variadic Call Handler – Dynamically dispatches arguments to builtin_print_int or builtin_print_str based on AST type information.

### Project Structure
```text
├── examples
│   ├── main.gv
│   └── test.gv
├── Makefile
├── README.md
└── src
    ├── as_frontend.c
    ├── AST.c
    ├── builtins.c
    ├── include
    │   ├── as_frontend.h
    │   ├── AST.h
    │   ├── builtins.h
    │   ├── io.h
    │   ├── lexer.h
    │   ├── list.h
    │   ├── macros.h
    │   ├── parser.h
    │   ├── tac.h
    │   ├── token.h
    │   ├── types.h
    │   └── visitor.h
    ├── io.c
    ├── lexer.c
    ├── list.c
    ├── main.c
    ├── parser.c
    ├── tac.c
    ├── token.c
    ├── types.c
    └── visitor.c
```

## Module Description

| Module           | Description                    |
|------------------|--------------------------------|
| lexer.c          | Tokenizes input source         |
| parser.c         | Builds AST from tokens         |
| visitor.c        | Performs semantic analysis     |
| as_frontend.c    | Generates assembly code        |
| ast.c / ast.h    | Defines AST structures         |

## Tools Used
- GCC → C Compiler (Requires gcc-multilib for 32-bit output on 64-bit systems)
- GNU Make → Build automation
- GNU Binutils (as, ld) → Assembly and linking


### Quick Start

## 1. Build the Compiler
```bash
make
```

## 2. Compile a Script
```bash
./gvr.out examples/test.gv
```

## 3. Run the output
```bash
./a.out
```

### 🗓 Roadmap / Future Enhancements
- [x] `for` loop syntax implementation
- [x] Array indexing and mutation
- [x] Character data type (`char`)
- [x] User-defined functions with stack-frame isolation
- [ ] Floating-point arithmetic using X87 FPU instructions
- [x] Boolean logic operators (`&&`, `||`)
- [x] Optimization pass (Constant Folding)

### Author
Gautam V (Student)

### Notes
This project is intended for educational purposes and demonstrates the end-to-end pipeline of a compiler:

Lexing → Parsing → AST → Code Generation → Execution
