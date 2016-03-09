;;; da1k5 - back to basics, precomputed sequence with minimal footprint
;;; but with possibility of some nice instrument sounds.
;;;
;;; Some overall things to try next for 1k would be:
;;; 
;;; 	(1) precalculate the whole song and let the audio callback do
;;;         just copying. Then the calculation doesn't need to preserve
;;; 	    register contents and all that "ABI crap".. can use RBP and
;;;         RSP as they are set in the main code and no need for those
;;;         frustrating LEAs in the beginning. Also, preservation of
;;;         synth state would not be an issue.. could mix into the middle
;;;         of the output buffer and so on..
;;; 
;;; 	(2) if there is a possibility to sequence a song, it just
;;;         can't be stored in the MIDI-like format that requires conversions
;;;         from tick deltas to frames (and with state also checks when the
;;;         next event is up).
;;;
;;;     (3) it is time to make a tool program to produce sequence data in a
;;; 	    micro format.. starting to be too convoluted for manual conversion..

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

;;; First we'll see how much effect the fundamental minimizations have:
;;; - gzipped (gzip -9 without self-extractor) size before optimizations: 1130
;;; - First version of pre-compute-and-copy: 1107
;;; - Separate arrays for time and note deltas; some micro-optimizations: 1098
;;; - Change the tuning so that the involved float contains 2 zero bytes: 1095

%include "general_macros.asm"

%macro	SYNTH_REQUIRED_DEFINES 0
;;; Used later in this code; byte that ends/rewinds the sequence:
	%define	SYN_END_OF_SEQUENCE_B	0x80
	
;;; Lifetime of a note instance as a number of frames
	%define	SYN_NOTE_LIFETIME	0x4000
	
;;; Tempo as a tick length
	%define	SYN_TICKLEN	0x0b00

;;; What we would like the callback userdata to be
	%define	SYN_USERDATA_INIT	syn_prerendered_content

;;; Song length (note that gdb will start slowly if very large bss is used):
	%define SYN_SONG_LENGTH_IN_FRAMES	48000*14

%endmacro

%macro	SYNTH_CONSTANT_DATA 0
syn_constants:
;;; syn_ticklen:	dd	0x1000		; tick length - now a macro
syn_c0freq:
;;; 	dd	8.175798915643707      ; C freq
;;; 	dd	0.00017032914407591056 ; C freq / sr
;;; 	dd	0.001070209575442236   ; C freq / sr * 2 * pi
	
	
;;; 	dd	0.0010700226	; a "nearby-value" with compressible repr.
;;; 	dd	0.0010681152
	dd	9.765625E-4
	
syn_freqr:
	dd	1.0594630943592953  ; freq. ratio between notes
	dd	0.0		    ; Could have other constants..
	
syn_sequence:
%if 0
	db	0,  45, 0,24
	db	2,  2,
	db	2,  1,
	db	2,  4,

	db	2,  5,
	db	2,  -2,	
	db	2,  -1,
	db	2,  -2,

	db	4,  2,
	db	2,  1,
	db	2,  4,
	db	2,  5,
	db	2,  -2,	
	db	2,  -1,
	db	2,  -2,

	db	2, SYN_END_OF_SEQUENCE_B
%else
syn_ndeltas:
	db	45,24,2,1,4,5,-2,-1,-2,2,1,4,5,-2,-1,-2, SYN_END_OF_SEQUENCE_B
;;; 
syn_tdeltas:
	db	0,0,2,2,2,  2,2,2,2,  4,2,2,2,2,2,2,  2
%endif
	
%endmacro

%define SYN_ADDR(reg,base,var) reg + ((var) - base)

%macro	SYNTH_BSS_DATA 0
syn_bss:
syn_note2f:		resd	128	; Note-frequency look-up table
synth_frames_done:	resq	1	; Global frame counter (read by main)
syn_prerendered_content:	resb	2*4*(SYN_SONG_LENGTH_IN_FRAMES)
%endmacro
	

;;; This code will be inlined, but we should preserve callee-saved registers.
;;; Also, we can assume that rbp points to _base1 near constant data; rbx
;;; points to _base2 near .bss. Dynamic libraries have been linked.

%macro	SYNTH_INITIALIZATION_CODE 0
%%start_of_synth_init:
	push	rbp
  	lea	rbp,[syn_constants] ; 4 bytes..

;;; ---------------------- Fill the frequency look-up table:
	lea	edi,[syn_note2f]
	push	rdi		; store for later popping to rbp
	GPR_LDB	rcx,127		; we don't make note 127.. rsi is left at syn_events.
 	fld	dword [SYN_ADDR(rbp,syn_constants,syn_c0freq)]
%%loop:
	fld	st0		; duplicate, to make code self-similar to render below
	fstp	dword [rdi]
	add	edi,4
 	fmul	dword [SYN_ADDR(rbp,syn_constants,syn_freqr)]
;;; 	dec	cl
;;; 	jnz	%%loop
	loop	%%loop
	fstp	dword[rdi]
;;; 	add	edi,4		; unnecessary, but makes opcodes absolutely self-similar
;;;  	add	edi,4
	;; Could be omitted if song starts with a pause of at least some frames:
 	add	edi,12		; becomes syn_prerendered_content
	

;;; 	mov	edx,2*4*(SYN_SONG_LENGTH_IN_FRAMES) ; buf length - not necessary?
;;; 	lea	rdi,[ABS syn_prerendered_content]
	
	lea	rsi,[SYN_ADDR(rbp,syn_constants,syn_sequence)] ; sequence
	
	pop	rbp		  ; Make this point to syn_bss
   	call	synth_render_song ; call for clarity; could inline.
	
;;; 	mov	dword [synth_frames_done],0

	pop	rbp
	
%endmacro
	


%macro	SYNTH_AUDIO_CALLBACK_CODE 0
;;; We get userdata in rdi, stream address in rsi, buffer length in rdx
;;; Userdata could be pointer to pre-rendered content..
;;; (no need to align stack; we call no-one here; use red zone)
audio_callb_sdl:

	shr	edx,3
;;; 	GPR_MOV	rcx, rdx
	
	mov	eax, [rdi - (syn_prerendered_content - synth_frames_done)]
	add	dword [rdi - (syn_prerendered_content - synth_frames_done)],edx
	
 	lea	edi, [edi + eax * 4]
	xchg	esi,edi

%%audio_copy_loop:
	;; Multiplex mono to stereo
	lodsd
	stosd
	stosd
;;; 	sub	edx,8
;;; 	jnz	%%audio_copy_loop
 	dec	edx
 	jnz	%%audio_copy_loop
;;; 	loop	%%audio_copy_loop

	ret


;;; ----------------- Pre-rendering code; could be inlined
;;; Called right after synth init. Expect: rbp points to syn_bss; rdi to 
;;; output buffer. rsi to beginning of sequence data.
synth_render_song:
	push	rbx
	push	rdi
	push	rsi
	push	4		; repetition count
%define REPET_COUNT	[rsp]
%if 0
 	push	0
%define FRAMES_DONE	dword [rsp+8]
	push	0
%define CURRENT_NOTE	[rsp]
%endif

	GPR_LD0	ebx
	GPR_LD0	edx
%%mixloop:
	;; compute frame on which the next event should start:
%if 1
	;; Some 20 or so bytes in total:
	GPR_LD0	rcx
	mov	cl,[rsi - (syn_ndeltas - syn_tdeltas)]
%%tickloop:
	jrcxz	%%ticks_added
	add	edx,SYN_TICKLEN
	loop	%%tickloop
%%ticks_added:
%else
	;; Some 20+ bytes in total:
	GPR_LD0	rax
	mov	al,[rsi - (syn_ndeltas - syn_tdeltas)]
  	mov	edx, SYN_TICKLEN
   	mul	edx
 	add	FRAMES_DONE,eax
 	mov	edx,FRAMES_DONE
%endif
	
	GPR_LD0	rax
	lodsb
	add	bl,al
	mov	eax,ebx
	jl	%%seq_fini
	
	;; do CURRRENT_NOTE starting at [rdi + FRAMES_D]
  	call	%%mix_note
	jmp	%%mixloop

%%seq_fini:

	;; Could loop to repetitions here..
	dec	byte REPET_COUNT
	jz	%%reps_fini
	mov	bl,0
	mov	rsi,[rsp+8]
	jmp	%%mixloop
%%reps_fini:

	
 	pop	rax
	pop	rax
	
	pop	rdi
	;; Could have after-effects here.. if need be?
	mov	rcx,SYN_SONG_LENGTH_IN_FRAMES
%%effloop:
	fld	dword [rdi]
	fld	dword [rdi]		; cpy of x
	fmul	st1		; x^2
	fmul	st1		; x^3
	fmul	st1		; x^4
	fld1			; 1
	faddp			; x^4+1
	fdivp			; -> should become x/(x^4+1)
	fstp	dword [rdi]
	add	rdi,4
	loop	%%effloop


	pop	rbx
	ret
	

	
%%mix_note:

	push	rdi
 	lea	rdi,[rdi + rdx * 4]
	push	SYN_NOTE_LIFETIME
	push	SYN_NOTE_LIFETIME	
%%l1:
	
 	fld	dword [SYN_ADDR(rbp,syn_bss,syn_note2f) + 4*rax] ; frequency from look-up
	fimul	dword [rsp]
	fsin

	;; Downward sloping envelope. Necessary:
	fild	dword [rsp]
	fidiv	dword [rsp+8]
	fmulp


	fadd	dword [rdi]	; Mix to current pos., and advance
	fstp	dword [rdi]
	add	rdi,4

	dec	dword[rsp]
	jnz	%%l1

	pop	rdi
	pop	rdi
	pop	rdi
	
	ret
	
%endmacro
