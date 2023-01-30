.code16
.global _start
_start:
    movw $0x0,%ax
    add $42,%ax

loop:
    hlt
    jmp loop
