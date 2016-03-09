;;; da1k8 - Like da1k7 but with optional bare-bones FM effect and a very crude
;;; delay effect. There is overhead, so might want to use da1k7 instead of
;;; this, if FM and delay is not wanted.. maybe the costs are small enough
;;; against the versatility of having just one version? must think about this,
;;; maybe...
;;; 
;;; See da1k7 for comments. This is just a "branch" with FM and delay.

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
;;;   The code will be "inlined", not called as a subroutine. Assume nothing
;;;   pre-initialized - this could be the very first code in the program.
;;; SYNTH_AUDIO_CALLBACK_CODE - must contain a function named
;;;   "audio_callb_sdl" that SDL2 will call to fill audio buf.
;;;   May contain helper also helper functions, if necessary.

	
%include "general_macros.asm"

%macro	SYNTH_REQUIRED_DEFINES 0
	;; Let's make the RNG an optional feature.. can lose those 19-23 bytes if required.
	%define SYN_FEAT_NOISE
	;; OK, the pitch envelope is also optional now. Only a couple of bytes.
	%define SYN_FEAT_PITCH_ENVELOPE
	%define SYN_FEAT_FM
	%define SYN_AINT_NO_STEREO

	%define	SYN_END_SEQ	0x75 ; ends both layer and song (invalid as time/note, sry.)
	
	;; Tempo as a tick length - could use a value that resembles some necessary opcode.
	%define	SYN_TICKLEN	0x1200

	;; What we would like the callback userdata to be
	%define	SYN_USERDATA_INIT	syn_prerendered_content

	;; Song length (note that gdb will start slowly if very large bss is used):
	%define SYN_SONG_LENGTH_IN_FRAMES	48000*14 ; minimum
	%define SYN_SONG_LENGTH_PAD_TO		0x400000 ; something tolerable?
%endmacro

%macro	SYNTH_CONSTANT_DATA 0

syn_constants:			; <--- this is a "base addr"; could vary the location..
syn_c0freq:
;;; 	dd	8.175798915643707      ; C freq (MIDI note 0; standard 440Hz tuning)
;;; 	dd	0.00017032914407591056 ; C freq / sr
;;; 	dd	0.001070209575442236   ; C freq / sr * 2 * pi

;;;  	dd	0.0010700226	; a "nearby-value" with compressible repr.
;;;  	dd	0.0010681152
;;;    	dd	9.765625E-4
;;;   	dd	0.001034632	; "nearby-value" almost same bytes as in freqr.
 	dd	0.001034631	; would be 0x3a879c75
;;; 	dd	0.0010833442	; 0x3a8dff00

syn_freqr:
;;; 	dd	1.0594630943592953  ; freq. ratio btw notes (shouldn't tweak much)
	dd	1.0594622	; would be 0x3f879c75 (but maybe a bit??)

%ifdef SYN_FEAT_NOISE
syn_rdiv:
;;;  	dd	0x80000000	; proper integer divisor for random number
;;;   	dd	0x3f879c7d	; another option for rand divisor
	;; (which just happens to be identical to the bit representation
	;; of the frequency ratio above.. randoms will be approximately 
	;; in the range [-2,2], though, instead of [-1,1]).
;;;    	dd	0x81000001	; another option for rand divisor; quiet range
   	dd	0x879c75c3	; Whatever.. just make these repeat the previous 

syn_rand:
	dd	0x879c75c3	; random seed. Just avoid bad values, and it's OK.
%endif
syn_ticklen:
	dd	SYN_TICKLEN


	times 12 	db 0	; Adjust rbp-relative address to match one in main.

syn_sequence:
;;; Instrument format for synth version "da1k7":
;;; 3 bits significant; others are unused and can be set arbitrarily
;;; aaaa abcd : d==Pitch envelope (BD)  c==Mute tone  b==Noise on (SD)  a==unused
;;; Precede by note length as number of ticks (one byte).
;;; 
;;; Tuning is "somewhere near and/or around MIDI note values.."
;;; 
;;; Pattern data. Format as a sequence of bytes:
;;; instrLen instrMix timeDelta noteDelta timeDelta noteDelta ...
syn_pattern:	

%if 1
	db	0x02, 0x01	; instrument info; length & mix
	db	0, 20		; low-ish note for BD
%macro PAT1	0
	db	4, 20
	db	4, 20
	db	4, 20
	db	4, 20
	db	4, 20
	db	4, 20
	db	4, 20
	db	2, 20
	db	2, 20
%endmacro
	
	PAT1
	PAT1
	PAT1
	PAT1
	PAT1
	PAT1

;;; 	db	4, SYN_REPEAT, 8, 5 ; Repeat 8 times from -5 bytes back
;;; 	db	0, -20		; mark end of layer by resetting note to 0
	db	SYN_END_SEQ
	

	db	0x01, 0x08
	db	32+4, 67	        ; higher for "pitched SD"
%macro PAT2	0
	db	8, 67
	db	8, 67
	db	8, 67
	db	3, 67
	db	5, 67
%endmacro
	PAT2
	PAT2
	PAT2
	PAT2
	PAT2
	
;;; 	db	0, SYN_REPEAT, 33, 6
;;; 	db	0, -67	; mark end of layer
	db	SYN_END_SEQ
	
%else	
	db	0x03, 0x02
	db	0+2, 37
%macro PAT3	0
	db	4, 37
	db	4, 37
	db	3, 37
	db	2, 37
	db	3, 34
	db	3, 34
	db	2, 34
	db	3, 34
	db	3, 34
	db	2, 34
	db	3, 37
%endmacro

	PAT3
	PAT3
	PAT3
	PAT3
	PAT3
	PAT3
%endif
	
	db	0x03, 0x02
	db	0, 37
	db	4, 49
	db	4, 49
%macro PAT4 0
	db	4, 47
	db	6, 37
	db	2, 49
	db	4, 51
	db	2, 47
	db	2, 54
	db	2, 37
	db	4, 37

	db	2, 49
	db	4, 49
%endmacro
	
	PAT4
	PAT4
	PAT4
	PAT4
	PAT4
	db	SYN_END_SEQ

	db	0x03, 0x02
	db	0, 68
	db	4, 80
	db	4, 92
	db	4, 90
	db	4, 92
	db	4, 90
	db	4, 92
	db	4, 90
	db	4, 102
	db	4, 80
	db	4, 92
	db	4, 90
	db	4, 92
	db	4, 90
	db	4, 92
	db	4, 90
	db	4, 100
	db	4, 80
	db	4, 92
	db	4, 90
	db	4, 92
	db	4, 90
	db	4, 92
	db	4, 90
	
	db	SYN_END_SEQ
	db	SYN_END_SEQ	; end song


%endmacro

%define SYN_ADDR(reg,base,var) reg + ((var) - base)

%macro	SYNTH_BSS_DATA 0

;;; Can adjust the bss start location to get nicer absolute addresses
TIMES (0x10000-8) + $$ - $	resb	1

synth_frames_done:	resq	1	; Global frame counter (read by main)
syn_prerendered_content:	resb	2*4*(SYN_SONG_LENGTH_IN_FRAMES)
ALIGNB	SYN_SONG_LENGTH_PAD_TO	
%endmacro
	
%macro	SYNTH_AUDIO_CALLBACK_CODE 0
;;; We get userdata in rdi, stream address in rsi, buffer length in rdx.
;;; Userdata points to frame counter, after which there is the pre-rendered content.
;;; (no need to align stack; we call no-one; we don't touch registers.)
	
audio_callb_sdl:
	;; As of now, I can't make this any shorter in any way. End of imagination.
	shr	edx,2
	mov	ecx,edx
	
	mov	eax, [rdi - (syn_prerendered_content - synth_frames_done)]
	add	[rdi - (syn_prerendered_content - synth_frames_done)], edx

 	lea	edi, [rdi + rax * 4]
  	xchg	rsi,rdi		; TODO: How likely is the stream addr above 32 bits!?
	
	rep	movsd		; One channel mono output. No loop needed, just rep.

	ret
%endmacro
	
	
;;; --------------------------------------------------------------------------
;;; Synth init. Agreement: Will be the first code in the whole program. Assume
;;; nothing; no need to preserve anything.
;;; --------------------------------------------------------------------------

%macro	SYNTH_INITIALIZATION_CODE 0

;;; Actual init code.
	
%%start_of_synth_init:

;;; Which one to choose? Likely depends on whether push dword repeats later or not:
;;;  	push	syn_prerendered_content
;;;  	pop	rdi
  	mov	edi,syn_prerendered_content ; shorter uncompressed opcodes
	
;;; Different options here, too. Best one likely depends on surrounding code:
;;; 	push	syn_constants
;;; 	pop	rbp
;;;  	lea	ebp,[syn_constants]
	mov	ebp,syn_constants
	
;;;  	mov	esi,syn_sequence
  	lea	esi,[SYN_ADDR(rbp,syn_constants,syn_sequence)]
	

;;; ----------------- Pre-rendering code; inlined in main
;;; Expect: rbp points to syn_constants; rdi to output buffer.
;;; rsi to beginning of sequence data. WE MESS UP RBX AND STACK!
synth_render_song:
	
	
%%songloop:
	GPR_LD0	eax
	lodsb
	cmp	al,SYN_END_SEQ
	je	%%end_ini
	
	mul	dword [SYN_ADDR(rbp,syn_constants,syn_ticklen)]

	lodsb			; include instrument mix byte
	mov	ebx,eax
  	GPR_LD0	ecx		; start mixing from frame 0.
;;;   	mov	ecx,0
;;; 	GPR_LDB	rcx,0
	
%%read_delta_and_note:
	GPR_LD0	eax
	lodsb
	cmp	al,SYN_END_SEQ
	je	%%songloop
	mul	dword [SYN_ADDR(rbp,syn_constants,syn_ticklen)]
	add	ecx,eax

	lodsb
;;; 	cmp	al,SYN_END_SEQ	; Could adjust location of jumps, but won't help..
	
;;; A primitive delay effect..
	push	rcx
	push	1
 	call	%%mix_note
	add	rcx,3*SYN_TICKLEN
	push	6
 	call	%%mix_note
	add	rcx,3*SYN_TICKLEN
	push	10
 	call	%%mix_note
	pop	rcx
	pop	rcx
	pop	rcx
	pop	rcx
	jmp	%%read_delta_and_note ; Expect ZF set always after mix_note!!


;;; ------------ produce one note.
	;; Expect rdi == output buffer start rcx == output start frame
	;; [rsp+8] == note length in frames BL == instrument bits
	;; AL == note
	;; 
	;; [Old was: Expect BH==tone length; must be at least 1. BL==note.]
%%mix_note:

%%compute_frequency:
	push	rax
	push	rdi
	push	rcx
	;; frequency re-compute. Stack contents in a Forth'ish comment:
	fld	dword [SYN_ADDR(rbp,syn_constants,syn_c0freq)] ; (c0note)
%%freqloop:
	fmul	dword [SYN_ADDR(rbp,syn_constants,syn_freqr)]
	dec	al
	jnz	%%freqloop
	;; Fp stack now has (note)

;;;   	lea	rdi,[rdi + rcx * 4]
   	lea	edi,[rdi + rcx * 4]
	
;;; 	add	ecx,ecx
;;; 	add	ecx,ecx
;;; 	add	edi,ecx
	
	;; Prepare stack with two copies of note length (in frames):
	push	rbx
	push	rbx



	;; Loop always starts with the following stack layout: (note)
%%l1:
	;; Downward sloping envelope (1->0). Necessary. Will be in fpstack:
	fild	dword [rsp]	; (note framesleft)
	fidiv	dword [rsp+8]	; (note [framesleft/length =: env])

	fld	st1	   	; (note env note)
	
	;; Multiply pitch by 1+2*env^3 if instrument is like that:
	ror	bl,1
%ifdef SYN_FEAT_PITCH_ENVELOPE
	jnc	%%no_pitch_env
	
	fadd	st0		; (note env (2*note))
	fmul	st1		; (note env (2*env*note))
	fmul	st1		; (note env (2*env^2*note))
	fmul	st1		; (note env (2*env^3*note))
	fadd	st2		; (note env [note+(2*env^3*note) =: finnote]) .. "bassdrummy"
%%no_pitch_env:
%endif
	
	;; Multiply frequency by frames_done to get current phase:

	fimul	dword [rsp]	; (note env phase*finnote)

%ifdef SYN_FEAT_FM
	ror	bl,1
	jnc	%%no_fm
	fld	st0		; (note env phase*finnote phase*finnote)
	fadd	st0
	fsin			; (note env phase*finnote sin(phase*finnote))
	fadd	st0
	fmul	st2		; (note env phase*finnote env*sin(phase*finnote))
	faddp			; (note env phase*finnote+env*sin(phase*finnote))
	;; Revert to zero if the instrument is like that:
%%no_fm:
	fsin			; (note env sin(phase*finnote))
	ror	bl,1
%else
	fsin			; (note env sin(phase*finnote))
	ror	bl,2
%endif

	jnc	%%no_zero_pitch
	fsub	st0		; (note env [(sin(phase*finnote) | 0.0) =: aud] )
%%no_zero_pitch:

	;; Add noise if the instrument is like that:
	;; Random generator seems rather lengthy (30+ bytes compressed..):
	;; 27 total when using register r13 to store seed (nasty xchg needed)
	;; OK.. "only" 24 when using rpb-relative addresses.. depends on the
	;; other bytes in the compressed stream, so might be +/- some. Final
	;; tweaking will likely help always. With nicer const values looks like
	;; 19 bytes only, so maybe it's not so very bad anymore.. optional anyway:
%ifdef SYN_FEAT_NOISE
;;;    	mov	eax,0x41a7	; multiplier 16807 == 0x41a7
;;; 	mov	eax,0x00010dcd	; 69069
;;; 	mov	eax,0x00010003	; 65539 (randu)
;;;    	mov	eax,0x73cbd0ff	; .. or could just repeat suitable bytes..
	mov	eax,0x879c75c3
 	ror	bl,1
 	jnc	%%no_noise	; conditional compresses better here in the middle..
	mul	dword [SYN_ADDR(rbp,syn_constants,syn_rand)]
	mov	dword [SYN_ADDR(rbp,syn_constants,syn_rand)], eax
	fild	dword [SYN_ADDR(rbp,syn_constants,syn_rand)]	; (note env aud cur_randseed)
	fidiv	dword [SYN_ADDR(rbp,syn_constants,syn_rdiv)]	; (note env aud cur_randf)
	faddp	st1					    	; (note env aud+randf)
%%no_noise:
	;; Multiply with the slope envelope. Necessary:
	ror	bl,1
	fmulp	st1  						; (note env*(aud+randf))
%else
	fmulp	st1
	ror	bl,2
%endif
	rol	bl,5

	fidiv	dword [rsp+48]

	;; Mix to current pos., and advance
	fadd	dword [rdi]					; (note env*(aud+randf)+old)
	fstp	dword [rdi]					; (note)
	scasd
	
	dec	dword [rsp]
	jnz	%%l1

	;; rid of stack space
	pop	rdi
	pop	rdi
	
	;; actual pops
	pop	rcx
	pop	rdi
	pop	rax
 	ret
	
%%end_ini:

	
%endmacro
