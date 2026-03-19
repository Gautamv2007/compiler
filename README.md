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