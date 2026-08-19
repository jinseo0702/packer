.section .rodata
message64:
	.ascii "static64:ok\n"
.set message64_len, . - message64

.section .text
.globl _start
_start:
	mov $1, %rax
	mov $1, %rdi
	lea message64(%rip), %rsi
	mov $message64_len, %rdx
	syscall
	mov $60, %rax
	mov $5, %rdi
	syscall
