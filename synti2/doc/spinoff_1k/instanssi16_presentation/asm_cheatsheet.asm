;;; Compile and disassemble this to quickly see how many bytes
;;; some operation compiles to..
BITS 64
GLOBAL main
SECTION .text
main:
            mov     eax, 42
            ret
	
	fstp	dword [rdi]
	fstp	dword [edi]
	
;;; 	push	0x0877665544332211

	db	0x7d, 0x9c, 0x87, 0x3a
times 8	nop
	db	0x9c, 0x87, 0x3a, 0x7d
times 8	nop
	db	0x87, 0x3a, 0x7d, 0x9c
times 8	nop
	db	0x3a, 0x7d, 0x9c, 0x87
times 8	nop

	lea	edi,[rbp+8]
	lea	edi,[ebp+8]
	
	call	nxt
nxt:
	call	rax
	call	[rax]
	call	rbx
	call	[rbx]
	call	r12
	call	[r12]
	add	r12,8
	call	[r14]
	add	r14,8
	jmp	rax
	jmp	[rax]
	jmp	r12
	jmp	[r12]
	jmp	rbp
	jmp	[rbp]
	jmp	[rbp+rax]
	jmp	[rbp+r12]
	
	jmp	j2
	jmp	j1
	lodsb
j1:	
	lodsw
	lodsd
	lodsq
j2:
	xchg	rsp,r12
	xchg	r12,rsp
	xchg	al,ah
	
	fld	dword [rsp]
	fld	dword [rsp+8]
	fld	dword [rbp]
	fld	dword [rbp+8]
	

        push	rbp
        call	[rbp-120]
        call	[rbp-64]
        call	[rbp-32]
        call	[rbp-16]
        call	[rbp-0]
        call	[rbp+16	]
        call	[rbp+32	]
        call	[rbp+64]
        call	[rbp+120]
        call	[rbp+128]
        call	[rbp+256]
        call	[rax+120]
        call	[rbx+120]
        call	[rcx+120]
        call	[rdx+120]
        call	[rdi+120]
        call	[rsi+120]
        call	[r8+120]
        call	[r12+120]
	push	rcx
	push	r8
	
	lea	rbp,[rbp+8]
	lea	rbp,[rbp-8]
	add	rbp,8
	sub	rbp,8
	
	lea	rbp,[rbp+800]
	lea	rbp,[rbp-800]
	add	rbp,800
	sub	rbp,800

	lea	ebp,[ebp+800]
	lea	ebp,[ebp-800]
	add	ebp,800
	sub	ebp,800

	add	bx,8
	sub	bx,8
