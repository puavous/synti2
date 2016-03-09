;;; Macros for troubleshooting code when DEBUG is defined.
;;; When DEBUG is undefined, these macros emit no code.

%macro DEBUG_HELPER_PUSH_ALL 0
	push	rax
	push	rbx
	push	rcx
	push	rdx
	push	rsi
	push	rdi
	push	r8
	push	r9
	push	r10
	push	r11
%endmacro

%macro DEBUG_HELPER_POP_ALL 0
	pop	r11
	pop	r10
	pop	r9
	pop	r8
	pop	rdi
	pop	rsi
	pop	rdx
	pop	rcx
	pop	rbx
	pop	rax
%endmacro

%ifdef DEBUG
%macro	DEBUG_dlerror 0
	DEBUG_HELPER_PUSH_ALL

	call	[dlerror_rel]	; DEBUG: print error after dlopen, dlsym
	mov	rdi,rax
	cmp	rdi,0
	jz	%%done
	call	[printf_p]
	mov	rdi,%%nl
	call	[printf_p]
	jmp	%%done
%%nl:	
	db	10,0
%%done:
	DEBUG_HELPER_POP_ALL
%endmacro

%macro	DEBUG_printline 1
	DEBUG_HELPER_PUSH_ALL
	
	mov	rdi,%1
	call	[printf_p]
	mov	rdi,%%nl
	call	[printf_p]

	jmp	%%done
%%nl:	
	db	10,0
%%done:	
	DEBUG_HELPER_POP_ALL
%endmacro

%macro	DEBUG_printaddress 1
	DEBUG_HELPER_PUSH_ALL

	mov	rdi,%%fmt
	mov	rsi,%1
	call	[printf_p]
	jmp	%%done
%%fmt:	
	db	'address: %p',10,0
%%done:	
	DEBUG_HELPER_POP_ALL
%endmacro


%macro	DEBUG_printmsg	1
	DEBUG_HELPER_PUSH_ALL

	jmp	%%pr
%%mnl:
	db	%1
	db	10,0
%%pr:
	mov	rdi,%%mnl
	call	[printf_p]

	DEBUG_HELPER_POP_ALL
%endmacro

%macro	DEBUG_iniprintf 0
	DEBUG_HELPER_PUSH_ALL
	jmp	%%doinit
	
libc_name:
	db	'libc.so',0
printf_name:
	db	'printf',0
debug_hello:
	db	'Hello world! I am ready for debug prints now.',10
	db	'(printf() has been linked, as you might see:))',10,0
	
%%doinit:
	mov	rdi,libc_name
	mov	esi,0x01
	call	[dlopen_rel]
	
	mov	rdi,rax
	mov	esi,printf_name
 	call	[dlsym_rel]
	mov	[printf_p],rax

	mov	rdi,debug_hello
	call	[printf_p]
	DEBUG_HELPER_POP_ALL
%endmacro

%else
%macro DEBUG_printmsg 1
%endmacro
%macro DEBUG_printaddress 1
%endmacro
%define DEBUG_iniprintf
%define DEBUG_dlerror
%define DEBUG_printline(a)
%endif
