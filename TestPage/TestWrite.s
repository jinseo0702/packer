section .text
    global _TestPacker

_TestPacker:
    push rbp
    mov  rbp, rsp
    push rax
    push rdi
    push rsi
    push rdx
    push rcx
    push r11
    push r8
    push r9
    push r10
    mov rax, 1
    mov rdi, 1
    lea rsi, [rel msg]
    mov rdx, 16
    syscall

    pop r10
    pop r9
    pop r8
    pop r11
    pop rcx
    pop rdx
    pop rsi
    pop rdi
    pop rax
    pop rbp
    push rax
    lea rax, [rel anchor]
    add rax, 0x11223344 ;현재 내 기준에서 _start 를 찾는다.
    xchg [rsp], rax
    ret

anchor:
msg:
    db "This is Packer", 10, 0
align 16