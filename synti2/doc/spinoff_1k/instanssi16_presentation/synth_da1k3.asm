;;; da1k3 - Like da1k2 but with actual sequencer with predefined
;;; notes (monophonic, supports pauses).

;;; Becomes about 130 bytes larger (compressed) than the "silence"
;;; benchmark synth.
	
;;; Define a couple of parameterless macros that will be expanded
;;; in suitable places of main code (must define each macro, even
;;; if they are empty):
;;; SYNTH_REQUIRED_DEFINES - defines and compiler constants
;;; SYNTH_CONSTANT_DATA - constant data to insert in code segment
;;;   (as of now, put the instrument and song data here, too)
;;; SYNTH_BSS_DATA - structures to reserve in the BSS section. MUST include
;;;   "synth_frames_done: resq 1" which is used in rendering code (as of now)
;;; SYNTH_INITIALIZATION_CODE - This macro will be inserted in the
;;;   initialization part of the main file. Prepare what is necessary.
;;;   The code will be "inlined", not called as a subroutine!
;;; SYNTH_AUDIO_CALLBACK_CODE - must contain a function named
;;;   "audio_callb_sdl" that SDL2 will call to fill audio buf.
;;;   May contain helper also helper functions, if necessary.

%include "general_macros.asm"

%macro	SYNTH_REQUIRED_DEFINES 0
;;; Used later in this code; byte that ends/rewinds the sequence:
	%define	SYN_END_OF_SEQUENCE_B	0x81
	%define SYN_PAUSE		0x80
%endmacro

%macro	SYNTH_CONSTANT_DATA 0
syn_constants:
syn_c0freq:
;;; 	dd	8.175798915643707      ; C freq
;;; 	dd	0.00017032914407591056 ; C freq / sr
	dd	0.001070209575442236   ; C freq / sr * 2 * pi
syn_freqr:
	dd	1.0594630943592953  ; freq. ratio between notes
	dd	0.0		    ; Could have other constants..

syn_sequence:
	db	69, 7, 5, SYN_PAUSE, -12, SYN_PAUSE, 12, SYN_PAUSE, 
	db	-9, 7, 5, SYN_PAUSE, -10, SYN_PAUSE, 12,  SYN_PAUSE,
	db	SYN_END_OF_SEQUENCE_B
%endmacro

%define SYN_ADDR(var) rbp + ((var) - syn_bss)

%macro	SYNTH_BSS_DATA 0
syn_bss:
synth_frames_done:
	resq	1
syn_step:
	resq	1		; Step of the sequencer.
syn_note:
	resq	1		; Current note (delta-coded sequence).
syn_ispause:
	resq	1		; Output 0 when this is up.
syn_note2f:
	resd	128		; Note-frequency look-up table
%endmacro
	

;;; This code will be inlined, but we should preserve callee-saved registers.
;;; Also, we can assume that rbp points to _base1 near constant data; rbx
;;; points to _base2 near .bss. Dynamic libraries have been linked.
	
%macro	SYNTH_INITIALIZATION_CODE 0
	lea	esi,[syn_note2f]
	GPR_LDB	rcx,127
 	fld	dword [rbp + ((syn_c0freq) - _base1)]
%%loop:
	fld	st0		; duplicate, to make code self-similar to render below
	fstp	dword [rsi]
	add	rsi,4
 	fmul	dword [rbp + ((syn_freqr) - _base1)]
	dec	cl
	jnz	%%loop
	fstp	dword[rsi]
	add	rsi,4		; unnecessary, but makes opcodes absolutely self-similar
%endmacro
	


%macro	SYNTH_AUDIO_CALLBACK_CODE 0
;;; We get userdata in rdi, stream address in rsi, buffer length in rdx
;;; (Userdata not used as of now - use rdi for other stuff)
;;; (no need to align stack; we call no-one here; use red zone)
audio_callb_sdl:
	push	rbp		; Use this as ptr to synth bss
	lea	rbp, [syn_bss]

aud_buf_loop:
	test	dword [SYN_ADDR(synth_frames_done)],0x1fff
	jnz	%%no_upd

	
;;; ------------------- Process sequence data only when step index updates:
	;; Which index is up next:
	mov	eax, [SYN_ADDR(syn_step)]
	;; If it is 0 (just rewound), we zero the note before delta:
	cmp	eax,0
	jne	%%no_zero
	mov     byte [SYN_ADDR(syn_note)], 0
%%no_zero:
	
	;; What delta do we have in there:
	lea	rdi, [syn_sequence]
	mov	cl, [rdi+rax]
	
	inc	eax
	cmp	byte [rdi+rax],SYN_END_OF_SEQUENCE_B
	jnz	%%no_rewind
	GPR_LD0	rax		;"rewind" if 0x80 is encountered
%%no_rewind:
	mov	[SYN_ADDR(syn_step)],eax

	mov	byte [SYN_ADDR(syn_ispause)],0
	cmp	cl,SYN_PAUSE
	jne	%%no_pause	;pause (silence) if 0x7f is encountered
	mov	byte [SYN_ADDR(syn_ispause)],1
	jmp	%%keep_old
%%no_pause:
	add	byte [SYN_ADDR(syn_note)], cl
%%keep_old:

%%no_upd:
;;; ------------------- Output note for each frame:
	;; Just load the currently playing note:
 	mov	eax, [SYN_ADDR(syn_note)]

	cmp	byte [SYN_ADDR(syn_ispause)],0
	je	%%no_pause2
	fld0
	jmp	%%do_mono_out
	
%%no_pause2:
	;; Intend to play sin(2pi*frequency*framecount/srate)
	;; We have pre-computed 2*pi*frequency/srate.
	fld	dword [SYN_ADDR(syn_note2f) + 4*rax] ; frequency from look-up
	fimul	dword [SYN_ADDR(synth_frames_done)]
;;; 	fidiv	dword [synth_sr_i]	; included in pre-computation. Could have all there!
	fsin
%%do_mono_out:
	;;  SDL2 Supports 32-bit float audio directly.. yiei!
	
	fld	st0		; duplicate stacktop, so following code can self-repeat
	fstp	dword [rsi]
	add	rsi,4
	fstp	dword [rsi]
	add	rsi,4

 	inc	dword [SYN_ADDR(synth_frames_done)]
	
	sub	edx,8
	jnz	aud_buf_loop

	pop	rbp
	ret
%endmacro
