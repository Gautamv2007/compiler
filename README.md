# GVR Compiler

A complete compiler implementation with integrated automated testing framework for validating generated code.

## Table of Contents

- [Overview](#overview)
- [Project Structure](#project-structure)
- [Building the Compiler](#building-the-compiler)
- [Language Features](#language-features)
- [Compiler Architecture](#compiler-architecture)
- [Testing Framework](#testing-framework)
- [Usage](#usage)
- [Installation](#installation)
- [Examples](#examples)
- [Contributing](#contributing)

## Overview

GVR is a compiler that translates high-level source code into x86 32-bit assembly language. The project includes a comprehensive automated testing framework that validates compiler output through compilation, assembly, linking, and execution verification.

### Key Features

- **Complete Compilation Pipeline**: Source code → Assembly → Object code → Executable
- **Target Architecture**: x86 32-bit (IA-32)
- **Assembly Format**: AT&T syntax
- **Automated Testing**: End-to-end validation with input/output verification
- **Error Reporting**: Comprehensive diagnostics for debugging
- **Build Automation**: Makefile-based build system

## Project Structure

```
.
├── src/                    # Compiler source code
├── include/                # Header files
├── gvr.out                 # Compiled compiler executable
├── Makefile                # Build configuration
├── run_tests.py            # Automated test runner
├── tests/                  # Test suite
│   ├── *.txt              # Source test files
│   ├── *.expected         # Expected output
│   └── *.input            # Optional input data
└── README.md              # This file
```

## Building the Compiler

### Prerequisites

- **GCC/G++**: C/C++ compiler (gcc-multilib, g++-multilib)
- **Make**: Build automation tool
- **Binutils**: GNU assembler and linker
- **Python 3**: For test automation
- **32-bit Libraries**: Support for IA-32 compilation

### Installation on Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install build-essential gcc-multilib g++-multilib binutils python3
```

### Build Commands

```bash
# Build the compiler
make

# Clean build artifacts
make clean

# Build and run tests
make test

# Rebuild from scratch
make clean && make
```

## Language Features

The GVR language supports the following features:

### Data Types

- **Integers**: Signed 32-bit integers
- **Variables**: Named storage locations
- **Arrays**: Fixed-size contiguous memory blocks

### Control Flow

- **Conditional Statements**: `if`, `else`, `elif `
- **Loops**: `while`, `for`
- **Function Calls**: User-defined functions

### Operations

- **Arithmetic**: `+`, `-`, `*`, `/`, `%`
- **Comparison**: `==`, `!=`, `<`, `>`, `<=`, `>=`
- **Logical**: `&&`, `||`, `!`
- **Assignment**: `=`, `+=`, `-=`, `*=`, `/=`, `%=`

### Input/Output

- **input()**: `read string;`
- **to_int(input())**: `read integer`
- **input_line()**: `read a whole line(useful when you want to input a string with spaces)`
- **print()**: `Very similar to the print function in python`

### Example Program

```
main = () -> {
    n:int = to_int(input()); #In my language we declare the variable name first and then the datatype
    factorial:int = 1; #max value for int = 2,147,483,647 (standard max in 32 bit integer)
    i:int = 1;

    while (i <= n) {
        factorial *= i;
        i += 1;
    }

    print(factorial, "\n"); #We can add pass multiple arguments
    return 0; #We can avoid writing this if we use main:void in the top line
}
```

## Compiler Architecture

### Compilation Stages

1. **Lexical Analysis**: Tokenization of source code
2. **Syntax Analysis**: Parse tree generation
3. **Semantic Analysis**: Type checking and validation
4. **Intermediate Code Generation**: Three-address code (TAC)
5. **Code Optimization**: Constant folding, dead code elimination
6. **Code Generation**: x86 assembly output

### Assembly Generation

The compiler generates AT&T syntax assembly for the x86-32 architecture:

```assembly
.section .text
.globl _start

_start:
    # Program code here
    
    # Exit system call
    movl $1, %eax
    movl $0, %ebx
    int $0x80
```

### System Calls

The generated code uses Linux system calls:

- **sys_exit (1)**: Program termination
- **sys_read (3)**: Read input
- **sys_write (4)**: Write output

## Testing Framework

### Overview

The automated testing framework validates compiler output through a multi-stage pipeline:

1. **Compile**: `./gvr.out source.txt` → Assembly code
2. **Assemble**: `as --32 file.s -o file.o` → Object file
3. **Link**: `ld -m elf_i386 file.o -o file` → Executable
4. **Execute**: Run with optional input (3-second timeout)
5. **Validate**: Compare output against expected results
6. **Cleanup**: Remove temporary files

### Test File Structure

#### Source Files (`.txt`)

Place test programs in `tests/` with `.txt` extension:

```
tests/factorial.txt
tests/fibonacci.txt
tests/arithmetic.txt
```

#### Expected Output (`.expected`)

Define expected output for each test:

**tests/factorial.expected:**
```
120
```

#### Input Data (`.input`) - Optional

Provide input for interactive tests:

**tests/factorial.input:**
```
5
```

### Running Tests

```bash
# Run all tests
python3 run_tests.py

# Example output:
# Starting Compiler Test Suite...
#
# Testing factorial.txt................ [ PASS ]
# Testing fibonacci.txt................ [ PASS ]
# Testing arithmetic.txt............... [ PASS ]
#
# ========================================
# Test Run Complete: 3 Passed, 0 Failed.
# ========================================
```

### Test Configuration

Modify `run_tests.py` to customize:

```python
COMPILER_CMD = "./gvr.out"           # Compiler executable
TEST_DIR = "tests"                   # Test directory
COMPILER_DEFAULT_OUTPUT = "a.s"      # Assembly output file
```

### Adding New Tests

1. Create source file: `tests/mytest.txt`
2. Create expected output: `tests/mytest.expected`
3. (Optional) Create input data: `tests/mytest.input`
4. Run: `python3 run_tests.py`

### Error Diagnostics

The test framework provides detailed error information:

| Error Type | Meaning | Action |
|------------|---------|--------|
| **Compilation Failure** | Compiler rejected source | Check syntax and semantics |
| **Assembly Error** | Invalid assembly syntax | Review generated assembly |
| **Linking Error** | Missing symbols or references | Ensure `_start` is defined |
| **Timeout** | Infinite loop (>3 seconds) | Check loop termination |
| **Output Mismatch** | Incorrect result | Verify logic and formatting |

## Usage

### Compiling a Program

```bash
# Compile source file
./gvr.out program.txt

# Generated output: a.s
```

### Running the program (The binary file is generated from the assembly automatically)

```bash
# Assemble
./a.out
```

### Complete Workflow Example

```bash
# 1. Write program
cat > hello.txt << 'EOF'
print 42;
EOF

# 2. Compile
./gvr.out hello.txt

# 3. Executre
./a.out
```

## Examples

### Example 1: Fibonacci Sequence

**Source (fibonacci.txt):**
```

main:void = () -> {
    n:int = to_int(input());
    a:int = 0;
    b:int = 1;
    i:int = 0;

    while(i < n){
        print(a, " ");
        temp:int = a+b;
        a = b;
        b = temp;
        i += 1;
    }

    return 0;
}
```

**Input (fibonacci.input):**
```
7
```

**Expected Output (fibonacci.expected):**
```
0
1
1
2
3
5
8
```

### Example 2: Arithmetic Operations

**Source (arithmetic.txt):**
```
main = () -> {
    a:int = 10;
    b:int = 2;
    print(a+b, "\n");
    print(a-b, "\n");
    print(a*b, "\n");
    print(a/b, "\n");
    print(a%b, "\n");
    print(-a, "\n");
    
    return 0;
}
```

**Expected Output (arithmetic.expected):**
```
12
8
20
5
0
-10
```

## Best Practices

### Code Organization

- **One Feature Per Test**: Each test should verify a single language feature
- **Descriptive Names**: Use clear filenames like `loop_while.txt`, `operator_addition.txt`
- **Edge Cases**: Include boundary conditions and error scenarios
- **Documentation**: Comment complex test cases

### Testing Strategy

- **Unit Tests**: Individual language features
- **Integration Tests**: Combined features and real programs
- **Regression Tests**: Previously fixed bugs
- **Performance Tests**: Large inputs and stress testing

### Version Control

```bash
# Track all test files
git add tests/*.txt tests/*.expected tests/*.input

# Commit compiler and tests together
git commit -m "Add feature X with tests"
```

## Troubleshooting

### Common Issues

**Problem**: Compilation fails with syntax error
- **Solution**: Review source code syntax, check language specification

**Problem**: Assembly error during `as` stage
- **Solution**: Examine generated assembly in `a.s`, verify instruction syntax

**Problem**: Linking error - undefined reference to `_start`
- **Solution**: Ensure compiler generates proper entry point

**Problem**: Test timeout
- **Solution**: Check for infinite loops in source or generated code

**Problem**: Segmentation fault during execution
- **Solution**: Review stack management and memory access using VALGRIND tool in linux

**Problem**: Output mismatch
- **Solution**: Check whitespace, newlines, and numeric formatting

### Debug Mode

Enable verbose output in the compiler (if implemented):

```bash
./gvr.out --verbose program.txt
```

## Architecture Details

### Register Usage

The compiler follows x86-32 calling conventions:

- **%eax**: Return values, system call numbers
- **%ebx**: System call arguments, base pointer
- **%ecx**: Counter register
- **%edx**: Data register
- **%esp**: Stack pointer
- **%ebp**: Base pointer

### Stack Frame

Function calls use standard stack frames:

```assembly
push %ebp
mov %esp, %ebp
# Function body
mov %ebp, %esp
pop %ebp
ret
```

### Memory Layout

```
High Address
├─── Stack (grows down)
├─── Heap (grows up)
├─── BSS (uninitialized data)
├─── Data (initialized data)
└─── Text (code)
Low Address
```

## Performance

### Optimization Levels

The compiler may support optimization flags:

- **-O0**: No optimization (default)
- **-O1**: Basic optimizations
- **-O2**: Advanced optimizations

### Benchmarks

Example performance metrics:

| Test | Execution Time (My Language) | Execution Time (C) | Execution Time (Python)|
|------|--------------------------|--------------------|------------------------|
| Sorting(15000 numbers) | 0.033s | 0.003s | 0.107s |

## Contributing

### Development Workflow

1. Create feature branch
2. Implement changes
3. Add tests for new features
4. Run test suite: `python3 run_tests.py`
5. Ensure all tests pass
6. Submit pull request

### Code Style

- Follow existing code conventions
- Add comments for complex logic
- Update tests when changing behavior
- Document new features in README

### Testing Requirements

All new features must include:

- Source test file (`.txt`)
- Expected output (`.expected`)
- Input data if needed (`.input`)
- Documentation in comments

## Continuous Integration

### CI/CD Pipeline

```bash
#!/bin/bash
# ci_test.sh

set -e

echo "Building compiler..."
make clean
make

echo "Running tests..."
python3 run_tests.py

if [ $? -eq 0 ]; then
    echo "All tests passed!"
    exit 0
else
    echo "Tests failed!"
    exit 1
fi
```

### GitHub Actions Example

```yaml
name: GVR Compiler CI

on: [push, pull_request]

jobs:
  build-and-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y gcc-multilib g++-multilib
      - name: Build compiler
        run: make
      - name: Run tests
        run: python3 run_tests.py
```

## Advanced Features

### Custom Test Timeouts

Edit `run_tests.py`:

```python
result = subprocess.run(
    [f"./{exe_file}"], 
    timeout=5  # Increase to 5 seconds
)
```

## License

This project is provided for educational purposes.

## Support

For issues or questions:

1. Check test output for error messages
2. Review generated assembly code
3. Verify prerequisites are installed
4. Consult language specification
5. Run tests in verbose mode

## Acknowledgments

Built with standard GNU toolchain:
- GCC - GNU Compiler Collection
- Binutils - GNU Binary Utilities
- GNU Make - Build automation

## References

- x86 Assembly Language Reference
- System V ABI for Intel386
- GNU Assembler Documentation
- GNU Linker Documentation

---

**Version**: 1.0  
**Last Updated**: March 2026  
**Maintainer**: Gautam V (Student)