default rel

section .text
global _start
_start:
    test byte [test_value], 0x20
    cmovne rax, [selected_value]
    cmp al, 1
    jne failed

    xor edi, edi
    mov eax, 60
    syscall

failed:
    mov edi, 1
    mov eax, 60
    syscall

section .data
test_value:
    db 0x20
selected_value:
    dq 1
