;;; da1k2 - Frequencies without powf(); use x87 only; constant pool
	
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
	%define SYN_AINT_NO_STEREO
%endmacro

%macro	SYNTH_CONSTANT_DATA 0
syn_constants:
syn_c0freq:
;;; 	dd	8.175798915643707      ; C freq / sr
;;; 	dd	0.00017032914407591056 ; C freq / sr
 	dd	0.001034631	; 0x3a879c75; close to MICI note 0 freq / sr * 2 * pi

syn_freqr:
;;; 	dd	1.0594630943592953  ; freq. ratio between notes
	dd	1.0594622	; 0x3f879c75; close to 1.0594630943592953
%endmacro



%macro	SYNTH_BSS_DATA 0
synth_frames_done:
	resq	1
syn_note2f:
	resd	256		; Note-frequency look-up table
%endmacro
	


%macro	SYNTH_INITIALIZATION_CODE 0
	lea	edi,[syn_note2f]
	mov	ecx,255
	fld	dword [syn_c0freq]
%%loop:
	fst	dword [rdi]
	scasd
	fmul	dword [syn_freqr]
	loop	%%loop
 	fstp	dword[rdi]
%endmacro
	


%macro	SYNTH_AUDIO_CALLBACK_CODE 0
;;; We get userdata in rdi, stream address in rsi, buffer length in rdx
audio_callb_sdl:
	push	rbx		; Use this here as ptr
	;; (no need to align stack; we call no-one here; use red zone)

;;;  	lea	ebx, [synth_frames_done]
 	mov	ebx, synth_frames_done

aud_buf_loop:
	mov	eax, [rbx]

	shr	eax,11		; make a funny mask
	and	eax,0x3c	; test note to play.
	add	eax,29		; Middle "A" is note 69 as in MIDI

	;; Intend to play sin(2pi*frequency*framecount/srate)
	lea	edi,[syn_note2f]
	fld	dword [rdi + 4*rax] ; 2pi*frequency from look-up
	fimul	dword [rbx]	    ; times phase
	fsin
	;;  SDL2 Supports 32-bit float audio directly.. yiei!
	fstp	dword [rsi]
	add	esi,4
	
	add	dword [rbx],1
	
	sub	edx,4
	jnz	aud_buf_loop

	pop	rbx
	ret
%endmacro
