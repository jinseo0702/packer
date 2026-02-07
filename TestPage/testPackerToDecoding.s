section .text
    global _TestPackerToDecoding

%define SYS_MPROTECT 10
%define PROT_RWX     7

%define PT_LOAD      1
%define PT_PHDR      6

%define PF_W         2

%define AT_NULL      0
%define AT_PHDR      3
%define AT_PHNUM     5
%define AT_PAGESZ    6

%define PHDR_P_TYPE    0
%define PHDR_P_FLAGS   4
%define PHDR_P_OFFSET  8
%define PHDR_P_VADDR  16
%define PHDR_P_FILESZ 32
%define PHDR_P_ALIGN  48
%define PHDR_SIZE     56

_TestPackerToDecoding:
stub_begin:
    mov r9, rsp

    push rbp
    mov  rbp, rsp

    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    lea r14, [rel packed_info]

    mov eax, dword [r14 + (packed_magic - packed_info)]
    cmp eax, 0x434F4445
    jne .restore_and_jump

    mov r15, [r14 + (packed_xor_key - packed_info)]

    mov rsi, r9
    add rsi, 8

.skip_argv:
    mov rax, [rsi]
    add rsi, 8
    test rax, rax
    jne .skip_argv

.skip_envp:
    mov rax, [rsi]
    add rsi, 8
    test rax, rax
    jne .skip_envp

    xor r12, r12                    ; AT_PHDR
    xor r10, r10                    ; AT_PHNUM
    mov rbx, 0x1000                 ; AT_PAGESZ default

.auxv_loop:
    mov rax, [rsi]
    mov rdx, [rsi + 8]
    test rax, rax
    jz .auxv_done

    cmp rax, AT_PHDR
    jne .auxv_check_phnum
    mov r12, rdx

.auxv_check_phnum:
    cmp rax, AT_PHNUM
    jne .auxv_check_pagesz
    mov r10, rdx

.auxv_check_pagesz:
    cmp rax, AT_PAGESZ
    jne .auxv_next
    mov rbx, rdx

.auxv_next:
    add rsi, 16
    jmp .auxv_loop

.auxv_done:
    test r12, r12
    jz .restore_and_jump
    test r10, r10
    jz .restore_and_jump

    ; base = AT_PHDR - p_vaddr(PT_PHDR)
    mov r13, r12
    mov rcx, r10
    xor r8, r8

.find_pt_phdr:
    test rcx, rcx
    jz .have_base

    mov eax, dword [r13 + PHDR_P_TYPE]
    cmp eax, PT_PHDR
    jne .find_next

    mov rax, [r13 + PHDR_P_VADDR]
    mov r8, r12
    sub r8, rax
    jmp .have_base

.find_next:
    add r13, PHDR_SIZE
    dec rcx
    jmp .find_pt_phdr

.have_base:
    mov r13, r12                    ; phdr pointer

.seg_loop:
    test r10, r10
    jz .after_decrypt

    mov eax, dword [r13 + PHDR_P_TYPE]
    cmp eax, PT_LOAD
    jne .seg_next

    mov rax, [r13 + PHDR_P_OFFSET]
    test rax, rax
    jz .seg_next

    mov eax, dword [r13 + PHDR_P_FLAGS]
    test eax, PF_W
    jnz .seg_next

    mov r9, [r13 + PHDR_P_VADDR]
    add r9, r8

    mov r12, [r13 + PHDR_P_FILESZ]
    test r12, r12
    jz .seg_next

    mov rdx, [r13 + PHDR_P_ALIGN]
    test rdx, rdx
    jnz .align_nonzero
    mov rdx, rbx
.align_nonzero:
    cmp rdx, rbx
    jae .align_ready
    mov rdx, rbx
.align_ready:

    mov rax, rdx
    dec rax
    not rax                         ; mask

    mov rdi, r9
    and rdi, rax                    ; start

    lea rsi, [r9 + r12]             ; end candidate
    mov rcx, rdx
    dec rcx                         ; align-1
    add rsi, rcx
    and rsi, rax                    ; aligned end
    sub rsi, rdi                    ; length

    mov rax, SYS_MPROTECT
    mov rdx, PROT_RWX
    syscall

    lea r11, [rel stub_begin]
    lea rdx, [rel stub_end]

    mov rdi, r9
    mov rcx, r12

.dec_qword:
    cmp rcx, 8
    jb .dec_tail

    cmp rdi, r11
    jb .do_qword
    cmp rdi, rdx
    jae .do_qword

    mov rax, rdx
    sub rax, rdi
    cmp rax, rcx
    jae .seg_next
    add rdi, rax
    sub rcx, rax
    jmp .dec_qword

.do_qword:
    mov rax, [rdi]
    xor rax, r15
    mov [rdi], rax
    add rdi, 8
    sub rcx, 8
    jmp .dec_qword

.dec_tail:
    test rcx, rcx
    jz .seg_next

.dec_byte:
    cmp rdi, r11
    jb .do_byte
    cmp rdi, rdx
    jae .do_byte

    mov rax, rdx
    sub rax, rdi
    cmp rax, rcx
    jae .seg_next
    add rdi, rax
    sub rcx, rax
    jmp .dec_byte

.do_byte:
    mov al, [rdi]
    xor al, r15b
    mov [rdi], al
    inc rdi
    dec rcx
    jnz .dec_byte

.seg_next:
    add r13, PHDR_SIZE
    dec r10
    jmp .seg_loop

.after_decrypt:
    mov rdi, r14
    mov rax, rbx
    dec rax
    not rax
    and rdi, rax
    mov rsi, rbx
    mov rax, SYS_MPROTECT
    mov rdx, PROT_RWX
    syscall

    lea rdi, [rel packed_info]
    mov rcx, packed_wipe_end - packed_info
    xor eax, eax
    rep stosb

.restore_and_jump:
    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    pop rbp

    lea rax, [rel anchor]
    add rax, [rel original_entry_delta]
    jmp rax

align 16
anchor:

align 16
packed_info:
packed_magic:              dq 0x57504445434F4445
packed_target_phdr_index:  dq 0x1111111122222222
packed_reserved:           dq 0x3333333344444444
packed_xor_key:            dq 0x5555555566666666
packed_wipe_end:
original_entry_delta:      dq 0x7777777788888888
packed_info_end:

stub_end:
