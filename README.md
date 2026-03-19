GVR Compiler

A custom-designed, statically-typed programming language implemented in C, featuring a modular frontend pipeline and direct compilation to 32-bit x86 Linux assembly.

Overview

This project demonstrates the design of a complete compiler from scratch, translating high-level, Python-inspired syntax into low-level machine code without using virtual machines or intermediate interpreters.

The compiler supports:

Strict static typing

Dynamic memory allocation for strings

A Python-style variadic print function

Interactive system I/O

The architecture is implemented in C and uses the GNU toolchain for assembling and linking.

Highlights

Compiles natively to x86 assembly

Static typing (int, str)

Custom bump allocator for memory management

Variadic print() function

Interactive input() and to_int() conversion

Modular lexer, parser, and AST pipeline

Direct Linux system calls using int 0x80

System Specifications
Component	Configuration
Host Language	C
Target Architecture	32-bit x86 Linux
Target Assembly	AT&T Syntax
Supported Data Types	Integer (int), String (str)
Memory Management	Bump Allocator (1 KB buffer)
I/O Interface	Linux Syscalls
Program Flow	Sequential, branches, loops
Architecture

The compiler consists of the following modular phases:

Lexical Analyzer (Lexer) – Tokenizes source code

Parser – Validates syntax and builds the AST

Semantic Analyzer (Visitor) – Handles scope and type checking

Code Generator – Produces x86 assembly

Assembler (as) – Generates object code

Linker (ld) – Produces the final executable

Syntax
Variable Declaration and Assignment
name: str = "Gautam";
age: int = 18;
Built-in Functions
Standard I/O

print(arg1, arg2, ...)
Variadic function supporting mixed data types

input()
Reads input into a dynamically allocated buffer

Data Conversion

to_int(string)
Converts ASCII string to integer

Control Flow

if / else statements

while loops

System-Level Integration
Assembly Macros

sys_read – Reads from stdin (file descriptor 0)

sys_write – Writes to stdout (file descriptor 1)

sys_exit – Terminates program

Backend Design

The backend includes:

Bump Allocator
Sequential memory allocation without deallocation

String-to-Integer Converter
Parses ASCII input into base-10 integers

Variadic Call Handler
Dispatches arguments to appropriate print handlers based on type

Project Structure
GVR-Compiler/
├── src/
│   ├── lexer.c
│   ├── parser.c
│   ├── visitor.c
│   ├── as_frontend.c
│   └── main.c
├── include/
│   ├── token.h
│   ├── ast.h
│   └── ...
├── examples/
│   └── test.gv
├── Makefile
└── README.md
Module Description
Module	Description
lexer.c	Tokenizes input source
parser.c	Builds AST from tokens
visitor.c	Performs semantic analysis
as_frontend.c	Generates assembly code
ast.c / ast.h	Defines AST structures
Tools Used

GCC (requires gcc-multilib for 32-bit builds)

GNU Make

GNU Binutils (as, ld)

Quick Start
1. Build the Compiler
make
2. Compile a Script
./gvr.out examples/test.gv
3. Run the Output
./a.out
Future Enhancements

Floating-point support (float)

User-defined functions

Array support

for loop syntax

Standard library and module system

Author

Gautam V

Interests

Compiler construction

Systems programming

Low-level architecture

Notes

This project demonstrates a complete compiler pipeline:

Lexing → Parsing → AST → Code Generation → Execution