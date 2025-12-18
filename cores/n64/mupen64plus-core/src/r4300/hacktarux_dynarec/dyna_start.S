;Mupen64plus - dyna_start.asm
;Mupen64Plus homepage: http://code.google.com/p/mupen64plus
;Copyright (C) 2007 Richard Goedeken (Richard42)
;Copyright (C) 2002 Hacktarux
;
;This program is free software; you can redistribute it and/or modify
;it under the terms of the GNU General Public License as published by
;the Free Software Foundation; either version 2 of the License, or
;(at your option) any later version.
;
;This program is distributed in the hope that it will be useful,
;but WITHOUT ANY WARRANTY; without even the implied warranty of
;MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;GNU General Public License for more details.
;
;You should have received a copy of the GNU General Public License
;along with this program; if not, write to the
;Free Software Foundation, Inc.,
;51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

option epilogue:none
option prologue:none


extern save_rsp: sqword
extern save_rip: sqword
extern reg: qword 
extern return_address: qword 

.CODE
PUBLIC dyna_start
dyna_start proc
   ;we must push an even # of registers to keep stack 16-byte aligned
%ifdef WIN64
    push rdi
    push rsi
%endif
    push rbx                 
    push r12
    push r13
    push r14
    push r15
    push rbp
    mov [save_rsp], rsp
    lea r15, [reg]        ;store the base location of the r4300 registers in r15 for addressing
    call _A1
    jmp _A2
_A1:
    pop  rax
   mov  [save_rip], rax
    sub rsp, 020h
    and rsp, 0FFFFFFFFFFFFFFF0h          ;ensure that stack is 16-byte aligned
    mov rax, rsp
    sub rax, 8h
    mov [return_address], rax
%ifdef WIN64
    call rcx
%else
    call rdi
%endif
_A2:
    mov  rsp, [save_rsp]
    pop  rbp
    pop  r15
    pop  r14
    pop  r13
    pop  r12
    pop  rbx
%ifdef WIN64
    pop rsi
    pop rdi
%endif

    ;mov QWORD [save_rsp], 0
    ;mov QWORD [save_rip], 0

    ret
dyna_start endp

end

