.section .text
.globl _start
.globl global_text32
.local local_text32
.weak weak_defined32
.weak weak_undefined32

_start:
	call global_text32
	mov $1, %eax
	xor %ebx, %ebx
	int $0x80

global_text32:
	call local_text32
	ret

local_text32:
	ret

weak_defined32:
	ret

.section .rodata
.globl global_rodata32
global_rodata32:
	.long 17

.section .data
.globl global_data32
global_data32:
	.long 42

.section .bss
.globl global_bss32
.lcomm global_bss32, 4
