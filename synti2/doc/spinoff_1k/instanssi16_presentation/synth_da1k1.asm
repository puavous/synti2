;;; Define a couple of parameterless macros that will be expanded
;;; in suitable places of main code (must define each macro, even
;;; if they are empty):
;;; SYNTH_REQUIRED_DEFINES - defines and compiler constants
;;; SYNTH_CONSTANT_DATA - constant data to insert in code segment
;;;   (as of now, put the instrument and song data here, too)
;;; SYNTH_BSS_DATA - structures to reserve in the BSS section
;;; SYNTH_INITIALIZATION_CODE - This macro will be inserted in the
;;;   initialization part of the main file. Prepare what is necessary.
;;;   The code will be "inlined", not called as a subroutine!
;;; SYNTH_AUDIO_CALLBACK_CODE - must contain a function named
;;;   "audio_callb_sdl" that SDL2 will call to fill audio buf.
;;;   May contain helper also helper functions, if necessary.

%macro	SYNTH_REQUIRED_DEFINES 0
%define USE_FUNCTIONS_FROM_LIBM
%endmacro

%macro	SYNTH_CONSTANT_DATA 0
%endmacro

%macro	SYNTH_BSS_DATA 0
syn_bss:
synth_frames_done:
	resq	1
%endmacro

%macro	SYNTH_INITIALIZATION_CODE 0
%endmacro

%macro	SYNTH_AUDIO_CALLBACK_CODE 0
;;; We get userdata in rdi, stream address in rsi, buffer length in rdx
audio_callb_sdl:
	push	rbx		; Use this as the output counter
	push	rbp		; We'll use rbp as the output ptr
	
	push	12		 ; rsp+16 int(12)
	push	2		 ; rsp+8 int(2)
	push	21		 ; rsp+0 int(440)
	
	mov	rbp,rsi
	mov	rbx,rdx
	
aud_buf_loop:
	mov	eax, [synth_frames_done]
	inc	eax
	mov	[synth_frames_done], eax
	
	shr	eax,11		; make a funny mask
	and	eax,0x3c	; test note to play.
	sub	eax,40		; Middle "A" is note 0 (no need to -69)
	
	cvtsi2ss	xmm1,dword eax
	cvtsi2ss	xmm0,dword [rsp+16]
	divss		xmm1,xmm0	; (note) / 12.0
	cvtsi2ss	xmm0,dword [rsp+8]
	call		[powf_p]	; RBP not usable in this thr.
	movss	dword [rsp-8],xmm0	; Result in xmm0, get a temp

	;; xmm0 is now the frequency to play
	;; Intend to play sin(2pi*frequency*framecount/srate)
	fldpi
	fadd	st0		; 2pi=pi+pi
	fimul	dword [synth_frames_done]
	fidiv	dword [synth_sr_i]
	fmul	dword [rsp-8]
	fimul	dword [rsp]	; times 440.0 for orchestra tuning
	fimul	dword [rsp]	; (or 21*21=441 for a bit sharp..)
	fsin
	;;  SDL2 Supports 32-bit float audio directly.. yiei!
	fst	dword [rbp]
	fstp	dword [rbp+4]
	lea	rbp,[rbp+8]

	sub	ebx,8
	jnz	aud_buf_loop
	
	add	rsp,24
	pop	rbp
	pop	rbx
	ret
%endmacro
