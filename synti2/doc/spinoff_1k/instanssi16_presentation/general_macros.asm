;;; Some NASM macros that emit either very short or gzip-compressible
;;; code instead of the "orthodox" ways to do operations. NOTE: these
;;; use the stack for operations, so red zone between rsp-1 and rsp-8
;;; cannot be used while these tricks are in use.
%ifndef GENERAL_MACROS_INCLUDED
%define GENERAL_MACROS_INCLUDED

;;; Code to move from register to another
%macro GPR_MOV 2
	push	%2
	pop	%1
%endmacro

;;; Code to load an immediate byte to a register (works for other
;;; sizes, too, but this is most useful with bytes - opcode&data for
;;; push is then 2 bytes total)
	
%macro GPR_LDB 2
	push	%2
	pop	%1
%endmacro

;;; Code to set all bits of a GPR to zero.
;;; The xor version seems to win, but remember to use the dword registers.
;;; I think I did check it in the manual that the upper 32 bits will be
;;; zeroed in rax when a value is loaded to eax, etc.. so no problem here.
%macro GPR_LD0 1
;;;  	mov	%1,0
	xor	%1,%1
%endmacro


%endif

