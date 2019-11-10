section .data

section .text
global _aprint
extern _GetStdHandle@4
extern _WriteFile@20
extern _SetConsoleTextAttribute@8

_aprint:

    mov     ecx,[esp+4]
    mov     eax,ecx
    call    slen
    mov     edx,eax
    mov     esi,[esp+8]

    ; hStdOut = GetstdHandle( STD_OUTPUT_HANDLE)
    push    -11
    call    _GetStdHandle@4
    mov     ebx, eax   

    pusha
    push    esi
    push    ebx
    call    _SetConsoleTextAttribute@8
    popa

    ; WriteFile( hstdOut, message, length(message), &bytes, 0);
    push    0
    push    0
    push    edx
    push    ecx
    push    ebx
    call    _WriteFile@20
    
    push    7   ;default white
    push    ebx
    call    _SetConsoleTextAttribute@8
  
    ret


slen:	;calculate the length of string stored in eax @param eax @ret eax
	push ebx
	mov ebx,eax
nextchar:
	cmp byte[eax],0
	jz finished
	inc eax
	jmp nextchar

finished:
	sub eax,ebx
	pop ebx
	ret	