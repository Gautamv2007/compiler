.section .text
.globl _start
_start:
pushl 0(%esp)
pushl 4(%esp)
call main
addl $4, %esp
movl %eax, %ebx
movl $1, %eax
int $0x80

.globl main
main:
pushl %ebp
movl %esp, %ebp
movl $10, %eax
pushl %eax
movl $5, %eax
pushl %eax
movl $10, %eax
pushl %eax
movl $5, %eax
movl %eax, %ebx
popl %eax
cdq
idivl %ebx
movl %edx, %eax
pushl %eax
movl -12(%ebp), %eax
pushl %eax
call builtin_print_int
addl $4, %esp
movl $0, %eax
movl %ebp, %esp
popl %ebp
ret
builtin_print_int:
    pushl %ebp
    movl %esp, %ebp
    subl $16, %esp
    movl 8(%ebp), %eax
    movl $10, %esi
    leal 15(%esp), %edi
    movb $10, (%edi)
    decl %edi
.L_convert_loop:
    xorl %edx, %edx
    divl %esi
    addb $48, %dl
    movb %dl, (%edi)
    decl %edi
    testl %eax, %eax
    jnz .L_convert_loop
    incl %edi
    movl $4, %eax
    movl $1, %ebx
    movl %edi, %ecx
    leal 16(%esp), %edx
    subl %edi, %edx
    int $0x80
    addl $16, %esp
    popl %ebp
    ret
