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