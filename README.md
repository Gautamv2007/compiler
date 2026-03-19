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

## Architecture Blocks

*(Note: You can insert a block diagram image of your Compiler Pipeline here once you upload one to GitHub!)*

The compiler consists of the following modular phases:

- **Lexical Analyzer (Lexer)** – Tokenizes raw source code  
- **Parser** – Validates grammar and constructs the Abstract Syntax Tree (AST)  
- **Semantic Analyzer (Visitor)** – Traverses the AST, tracking variable scopes and types  
- **Code Generator** – Translates AST nodes into x86 assembly instructions  
- **Assembler (`as`)** – Converts assembly into object code  
- **Linker (`ld`)** – Stitches object code into an ELF executable  

---

## Syntax & Formats

### Variable Declaration & Assignment
```typescript
name:str = "Gautam";
age:int = 18;


---

## 🛠️ Built-in Functions & Operations

### Standard I/O
- **`print(arg1, arg2, ...)`** – Variadic printing supporting mixed data types.
- **`input()`** – Reads input from standard input into a dynamically allocated buffer.

### Data Conversion
- **`to_int(string)`** – Converts ASCII string input into integer values.

### Control Flow
- **`if` / `else`** statements
- **`while`** loops

---

## ⚙️ System-Level Integration

### Assembly Macros
- **`sys_read`** – Reads from file descriptor 0 (`stdin`).
- **`sys_write`** – Writes to file descriptor 1 (`stdout`).
- **`sys_exit`** – Terminates the program with status 0.

---

## 🧠 Backend Engine Design

The compiler backend integrates several optimized components:

- **Bump Allocator** – Efficiently stores variable-length inputs sequentially in memory without overwrites.
- **String-to-Integer Converter** – Implements ASCII parsing using arithmetic logic for base-10 integer generation.
- **Variadic Call Handler** – Dynamically dispatches arguments to `builtin_print_int` or `builtin_print_str` based on AST type information.

---

## 📂 Project Structure

```text
GVR-Compiler/
├── src/
│   ├── lexer.c        # Tokenization of input
│   ├── parser.c       # Grammar enforcement & AST generation
│   ├── visitor.c      # Symbol table & semantic handling
│   ├── as_frontend.c  # Assembly code generation
│   └── main.c         # Entry point
├── include/
│   ├── token.h        # Token definitions
│   ├── ast.h          # AST structures
│   └── ...
├── examples/
│   └── test.gv        # Sample program
├── Makefile           # Build automation
└── README.md


Markdown
## Module Description

| Module | Description |
|--------|-------------|
| **`lexer.c`** | Breaks the input stream into tokens. |
| **`parser.c`** | Builds the AST based on language grammar. |
| **`visitor.c`** | Handles symbol tables, memory offsets, and types. |
| **`as_frontend.c`** | Generates x86 assembly and built-in macros. |
| **`ast.c` / `ast.h`** | Defines AST node structures. |

---

## Tools Used

- **GCC** → C Compiler *(Requires `gcc-multilib` for 32-bit output on 64-bit systems)*
- **GNU Make** → Build automation
- **GNU Binutils (`as`, `ld`)** → Assembly and linking

---

## 🚀 Quick Start

### 1. Build the Compiler
```bash
make
## 2. Compile a Script
```Bash
./gvr.out examples/test.gv
## 3. Run the Output
```Bash
./a.out
### Future Enhancements
Support for floating-point arithmetic (float)

User-defined functions with parameters

Array support and contiguous memory structures

for loop syntax

Standard library and module system

## Author
Gautam V (Student)

Interests:

Compiler Construction & Language Design

Systems Programming

Low-Level Architecture

##Notes
This project is intended for educational purposes and demonstrates the end-to-end pipeline of a compiler:

Lexing → Parsing → AST → Code Generation → Execution
