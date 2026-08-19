.section .rodata
message32:
	.ascii "static32:ok\n"
.set message32_len, . - message32

.section .text
.globl _start
_start:
	mov $4, %eax
	mov $1, %ebx
	mov $message32, %ecx
	mov $message32_len, %edx
	int $0x80
	mov $1, %eax
	mov $6, %ebx
	int $0x80
