section .data
message:    db  'Hello,World',10h
message_end:

section .text
global _aprint
global _aprintRed
extern _GetStdHandle@4
extern _WriteFile@20
extern _ExitProcess@4
extern _SetConsoleTextAttribute@8

_aprint:

    ; hStdOut = GetstdHandle( STD_OUTPUT_HANDLE)
    push    -11
    call    _GetStdHandle@4
    mov     ebx, eax    

    ; WriteFile( hstdOut, message, length(message), &bytes, 0);
    push    0
    push    0
    push    (message_end - message)
    push    message
    push    ebx
    call    _WriteFile@20
    
    ret

_aprintRed:

    ; hStdOut = GetstdHandle( STD_OUTPUT_HANDLE)
    push    -11
    call    _GetStdHandle@4
    mov     ebx, eax   

    push    4   ;red
    push    ebx
    call    _SetConsoleTextAttribute@8

    ; WriteFile( hstdOut, message, length(message), &bytes, 0);
    push    0
    push    0
    push    (message_end - message)
    push    message
    push    ebx
    call    _WriteFile@20
    
    push    7   ;default white
    push    ebx
    call    _SetConsoleTextAttribute@8
  
    ret