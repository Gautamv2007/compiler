GVR Compiler
A custom, statically-typed programming language built entirely from scratch in C.

The GVR Compiler features a custom lexer, parser, and Abstract Syntax Tree (AST) that compiles high-level, Python-inspired syntax directly down to raw 32-bit x86 Linux Assembly. No virtual machines, no interpreters—just pure, bare-metal machine code.

Key Features
Compiled to x86 Assembly: Your code is translated into efficient, standalone Linux executables.

Static Typing: Enforced data types (int, str) for safer code and precise memory management.

Smart Print Function: A Python-style, variadic print() that automatically detects and handles multiple data types in a single function call.

Dynamic Bump Allocator: A custom built-in memory allocator that safely stores user input in RAM without overwriting previous data.

Interactive Input: Seamless string and integer reading using input() and to_int().

Control Flow: Fully functional if / else statements and while loops.

Native System Calls: Directly interfaces with the Linux Kernel (int 0x80) for highly optimized I/O operations.

Syntax & Example
GVR blends the strict typing of TypeScript with the readable, user-friendly built-in functions of Python.

Here is a working example of a GVR program (example.gv):

TypeScript
main = () -> {
  print("Enter your name: ");
  msg:str = input();

  print("Enter your age: ");
  age:int = to_int(input());

  if (age > 17) {
    print(msg, ", Congratulations you are eligible to vote!\n");
  }
  else {
    print(msg, ", You still need to wait for ", (18 - age), " years.\n");
  }

  return 0;
}
Architecture Under the Hood
Lexical Analysis: Breaks raw text down into recognizable tokens (Identifiers, Keywords, Operators).

Parsing: Constructs an Abstract Syntax Tree (AST) enforcing the grammar rules of the language.

Semantic Analysis (Visitor): Traverses the AST, tracking variable scopes, calculating memory offsets, and validating types.

Code Generation: Generates raw AT&T syntax x86 assembly, dynamically linking custom built-in assembly macros for memory allocation, string conversion, and system I/O.

Assembling & Linking: Calls GNU as and ld to stitch the assembly into an ELF executable.

How to Build & Run
Prerequisites
A Linux environment (or WSL on Windows).

gcc, make, and binutils (as, ld) installed.

Note: Requires a 32-bit compilation environment (e.g., gcc-multilib on 64-bit systems).

1. Build the Compiler
Clone the repository and run make to compile the C source code into the compiler executable:

Bash
make
2. Compile a Script
Pass a .gv file to the newly built compiler. This will generate the assembly code and produce a final executable (defaulting to a.out):

Bash
./gvr.out examples/test.gv
3. Run Your Program
Execute the compiled binary:

Bash
./a.out
Built with C, Assembly, and a lot of caffeine. ☕
