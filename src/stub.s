org 0x0

%ifdef ARCH_64
bits 64
	%define CTX_AX      rax
	%define CTX_BX      rbx
	%define CTX_CX      rcx
	%define CTX_DX      rdx
	%define CTX_SI      rsi
	%define CTX_DI      rdi
	%define CTX_SP      rsp
	%define CTX_BP      rbp
	%define PTR_SIZE    8
	%define CODE_SIZE   123
%else
bits 32
	%define CTX_AX      eax
	%define CTX_BX      ebx
	%define CTX_CX      ecx
	%define CTX_DX      edx
	%define CTX_SI      esi
	%define CTX_DI      edi
	%define CTX_SP      esp
	%define CTX_BP      ebp
	%define CODE_SIZE   123
%endif

%ifdef ARCH_64
	%macro PUSH_ALL_REGISTER 0
	bits 64
		push rax
    	push rbx
    	push rcx
    	push rdx
    	push rsi
    	push rdi
    	push rbp
    	push r8
    	push r9
    	push r10
    	push r11
    	push r12
    	push r13
    	push r14
    	push r15
	%endmacro
%else
	%macro PUSH_ALL_REGISTER 0
	bits 32
		PUSHAD
	%endmacro
%endif

%ifdef ARCH_64
	%macro POP_ALL_REGISTER 0
	bits 64
		pop r15
		pop r14
		pop r13
		pop r12
		pop r11
		pop r10
		pop r9
		pop r8
		pop rbp
		pop rdi
		pop rsi
		pop rdx
		pop rcx
		pop rbx
		pop rax
	%endmacro
%else
	%macro POP_ALL_REGISTER 0
	bits 32
		POPAD
	%endmacro
%endif

%macro INIT_RESI 1
	xor %1, %1
%endmacro

; get the virtual address of argument 2
; %1 is destination  register , %2 is caller label
; want label vaddr subtraction callee label and caller label
%macro CALL_POP 2
	call %%next
%%next:
	INIT_RESI %1
	pop %1
	sub %1, (%%next - %2)
%endmacro

; get the Data if stub vaddr ...
; %1 is destination  register
; %2 is shell code size
%macro FIND_DATA 2
	CALL_POP %1, _stub
	add %1, %2
%endmacro

; get the Data and find index
; %1 is destination  register
; %2 index
; %3 element size in bytes
%macro FIND_DATA_INDEX 3
	FIND_DATA %1, CODE_SIZE
	add %1, (%2 * %3)
%endmacro

; Find the address at base plus offset
; %1 is destination  register
; %2 is offset
%macro FIND_OFFSET 2
	FIND_DATA %1, %2
%endmacro

; use mprotec(2)
; %1 start
; %2 len
; %3 prot
%ifdef ARCH_64
	%macro USE_MPROTEC 3
	bits 64
		INIT_RESI CTX_AX
		INIT_RESI CTX_DI
		INIT_RESI CTX_SI
		INIT_RESI CTX_DX
		mov CTX_DI, %1
		mov CTX_SI, %2
		mov CTX_DX, %3
		mov CTX_AX, 10
		syscall
		CHECK_ERROR
	%endmacro
%else
	%macro USE_MPROTEC 3
	bits 32
		INIT_RESI CTX_AX
		INIT_RESI CTX_BX
		INIT_RESI CTX_CX
		INIT_RESI CTX_DX
		mov CTX_DI, %1
		mov CTX_SI, %2
		mov CTX_DX, %3
		mov CTX_AX, 125
		syscall
		CHECK_ERROR
	%endmacro
%endif

;use write(2)
; %1 fd
; %2 buf
; %3 count
%ifdef ARCH_64
	%macro USE_WRITE 3
	bits 64
		INIT_RESI CTX_AX
		INIT_RESI CTX_DI
		INIT_RESI CTX_SI
		INIT_RESI CTX_DX
		mov CTX_DI, %1
		mov CTX_SI, %2
		mov CTX_DX, %3
		mov CTX_AX, 1
		syscall
		CHECK_ERROR
	%endmacro
%else
	%macro USE_WRITE 3
	bits 32
		INIT_RESI CTX_AX
		INIT_RESI CTX_BX
		INIT_RESI CTX_CX
		INIT_RESI CTX_DX
		mov CTX_DI, %1
		mov CTX_SI, %2
		mov CTX_DX, %3
		mov CTX_AX, 4
		syscall
		CHECK_ERROR
	%endmacro
%endif

; exit syscall
%ifdef ARCH_64
	%macro CHECK_ERROR
	bits 64
		test CTX_AX, CTX_AX
		js %%error
		jmp %%done
	%%error:
		INIT_RESI CTX_DI, CTX_DI
		neg CTX_AX
		mov CTX_DI, CTX_AX
		mov CTX_AX, 60
		syscall
	%%done:
	%endmacro
%else
	%macro CHECK_ERROR
	bits 32
		test CTX_AX, CTX_AX
		js %%error
		jmp %%done
	%%error:
		INIT_RESI CTX_BX, CTX_BX
		neg CTX_AX
		mov CTX_BX, CTX_AX
		mov CTX_AX, 1
		syscall
	%%done:
	%endmacro
%endif

; get GET_DATA_VALUE
; %1 record key address
; %2 index
; %3 destination key value
%macro GET_DATA_VALUE 3
	INIT_RESI %1
	INIT_RESI %3
	FIND_DATA_INDEX %1, %2, 8
	mov %3, [%1]
%endmacro

; get META_DATA address **return not value**
; %1 record origin meta_data address 
; %2 record meta_data * index
%macro GET_META_DATA 3
	INIT_RESI %1
	INIT_RESI %2
	FIND_DATA_INDEX %1, 6, 8
	mov %2, (%1 + (8 * %3))
%endmacro

; get value and insert to destination
; %1 source value address
; %2 destination value address
%macro INIT_INSERT_VALUE 2
	INIT_RESI %1
	mov %2, [%1]
%endmacro

_stub:
	INIT_RESI CTX_CX, CTX_CX
	INIT_RESI CTX_AX, CTX_AX
	FIND_DATA_INDEX CTX_CX, 2, 8
	FIND_OFFSET CTX_AX, [CTX_CX]
	push CTX_AX
	PUSH_ALL_REGISTER
	INIT_RESI CTX_BP

_loop:
	GET_DATA_VALUE CTX_AX, 5, 8
	cmp [CTX_BP], CTX_AX
	je _stub_end
	jmp _loop

_decryption:
	GET_META_DATA CTX_AX, CTX_DI, [CTX_BP]
	INIT_INSERT_VALUE CTX_AX, (CTX_DI + 8) ; p_memsz
	INIT_INSERT_VALUE CTX_SI, (CTX_DI + 16) ; p_flags


_stub_end:
	POP_ALL_REGISTER
	ret

align 16

; db(16byte) ....WOODY....\n\0\0
; dq(8byte) real_entry 
; dq(8byte) p_vaddr 
; dq(8byte) key
; dq(8btye) Meta data num
/*
meta data How to
{
dq(8byte) p_vaddr = start offset
dq(8byte) p_memsz = end_offset
dq(8byte) p_falgs = decision mprotec used
}
*/