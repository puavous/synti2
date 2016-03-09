;;; da1k4 - tried to keep length of da1k3 (+130 bytes compressed,
;;; compared to silence), but get some more functionality. Tried some 
;;; different approaches to sequence and sound generation.
;;;
;;; Didn't achieve the size goal - at least yet. Remains at +230. It looks 
;;; like 1k productions will need a less "universal" synth. Sequence
;;; storage and its parsing just makes so many bytes appear from almost 
;;; nowhere. But this might be a good starting point for a 4k synth, 
;;; though.. so I'll keep on optimizing this as far as I can, but for
;;; 1k I'll have to take a long step backwards in the sequencing 
;;; capabilities. Some overall things to try next for 1k would be:
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
;;; TODO: Branch this to a "bloaty 4k synth" project, and try something much 
;;; less spectacular for 1k.
;;; 

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

;;; Ideas in here (from my synti2 synth, basically) were:
;;; - use time deltas and note deltas as the input byte stream
;;; - prepare a linked list of event objects (max 128 bytes, so
;;;   the whole structure can be addressed with small code footprint
;;;   using base+offset)
;;; - sequencer moves the next event object to a linked list of 
;;;   (polyphonic) active notes. Same object re-used here.
;;; - after note stops playing, the object is forgotten.
;;; - still just one instrument, no note-offs.

;;; STATUS: The ideas had to be mostly abandoned. A key question was, if the
;;; list updates could be made very short. (Well, if somebody can make them
;;; short enough, it's not me.. ok.. didn't try weirder tricks like pointing to
;;; elements with rsp and popping content to registers and then pushing back to
;;; another place in a different order. that would be a fun experiment..)

;;; Let's see, how this thing goes, step by step.. "for the chronicles":
;;; - gzipped (gzip -9 without self-extractor) size before optimizations: 1156
;;; - Lifetime reset omitted (since bss is zeros at start): 1156, a so-so situation?
;;; - Use "push 0" to initialize variables in stack instead of red zone: 1149
;;; - Use pops to release stack space, instead of add: 1147
;;; - Use rbp-relative addressing for synth constants in init: 1144
;;; - Use pop&push0 instead of mov [rsp] for resetting note: 1141
;;; - Revert to using array instead of list for event queue: 1130 (->disappointment)
;;; - Go back to just an array of active notes: 1125
;;; - And then I changed the example sequence, so old measures aren't useful.
;;; So.. what gives? ... 30 bytes was very hard, required using a less
;;; versatile design than I wanted, and it quite disappointingly looks as if the
;;; desired features are not even possible to do within the original goal sizes..

;;; But let us see what micro-optimizations came after those:
;;; Start with 1125
;;; - assume data and bss addresses are in 32-bit range, use ESI etc.: 1121
;;; - make tick length a constant instead of stored data: 1117
;;; - absolute addressing for syn_events; stack trick for fidiv: 1115
;;; - remember to use 32-bit move for framecount: 1112
;;; - 32-bit addresses in other places, too: 1111

;;; Then add an envelope. Becomes 1130.
	
%include "general_macros.asm"

%macro	SYNTH_REQUIRED_DEFINES 0
;;; Used later in this code; byte that ends/rewinds the sequence:
	%define	SYN_END_OF_SEQUENCE_B	0x80
	
;;; Lifetime of a note instance as a number of frames
	%define	SYN_NOTE_LIFETIME	0x1000
	
;;; Tempo as a tick length
	%define	SYN_TICKLEN	0x0b00
	
;;; Overlapping structure offsets for the event / active note:
	%define SYN_O_ACTIVE	0	; Is this slot playing at the moment?
	%define SYN_O_TIME	8	; -> becomes time since note-on
	%define SYN_O_NOTE	16	; the note -> remains the same
	%define SYN_O_CUR_FREQ	24	; -> (just some ideas to maybe use)
	%define SYN_O_CUR_AMPL	32	; -> (just some ideas to maybe use)
	%define SYN_O_CUR_ENVS	40	; -> (just some ideas to maybe use)
	%define SYN_O_CUR_SOME1	48	; -> (just some ideas to maybe use)

;;; Sizes of structures (note that gdb will start slowly if very large bss is used):
	%define SYN_EVENT_SIZE	0x40	; event object size.. more than enough as of now
	%define SYN_MAX_POLY	0x20	; polyphony
	
	
%endmacro

%macro	SYNTH_CONSTANT_DATA 0
syn_constants:
;;; syn_ticklen:	dd	0x1000		; tick length - now a macro
syn_c0freq:
;;; 	dd	8.175798915643707      ; C freq
;;; 	dd	0.00017032914407591056 ; C freq / sr
	dd	0.001070209575442236   ; C freq / sr * 2 * pi
syn_freqr:
	dd	1.0594630943592953  ; freq. ratio between notes
	dd	0.0		    ; Could have other constants..
	
syn_sequence:
%if 0
	;; Just testing for polyphony and stuff.. seems to work..
	;; But this byte format is very wasteful.. need something more compact..
	db	0, 69,
	db	2,  5,   0, 5,
	
	db	4, -3,   0, 5,

	db	4, -12,
	db	2,  5,   0, 5,
	
	db	4, -3,   0, 5,
	;;
	db	4, -12,
	db	2,  5,   0, 5,
	
	db	4, -3,   0, 5,

	db	4, -12,
	db	2,  5,   0, 5,
	
	db	4, -3,   0, 5,
	;;
	db	4, -19,
	db	2,  12,   0, 5,
	
	db	4, -3,   0, 5,

	db	4, -19,
	db	2,  12,   0, 5,
	
	db	4, -3,   0, 5,
	
	db	4, -17,
	db	2,  10,   0, 5,
	
	db	4, -3,   0, 5,

	db	4, -17,
	db	2,  12,   0, 5,
	
	db	4, -3,   0, 5,
	
	db	4, SYN_END_OF_SEQUENCE_B
%else
	
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
%endif
	
%endmacro

%define SYN_ADDR(reg,base,var) reg + ((var) - base)

%macro	SYNTH_BSS_DATA 0
syn_bss:
synth_frames_done:	resq	1	; Global frame counter (read by main)
syn_current_note:	resq	1	; Current delta-decoded note
syn_prev_event_frame:	resq	1	; delta-decoded frame of the previous note
syn_next_event_ind:	resq	1	; Index to next byte in sequence input
syn_note2f:		resd	128	; Note-frequency look-up table
syn_events:		resb	(SYN_MAX_POLY * SYN_EVENT_SIZE)	; active events
%endmacro
	

;;; This code will be inlined, but we should preserve callee-saved registers.
;;; Also, we can assume that rbp points to _base1 near constant data; rbx
;;; points to _base2 near .bss. Dynamic libraries have been linked.

%macro	SYNTH_INITIALIZATION_CODE 0
%%start_of_synth_init:
	push	rbp	    ; use rbp for different purpose here..
;;;  	mov	rbp,[rbp + ((ptr_to_syn_constants) - _base1)] ; 4 bytes but requires const
  	lea	rbp,[syn_constants] ; 4 bytes..

;;; ---------------------- Fill the frequency look-up table:
	lea	esi,[syn_note2f]
	GPR_LDB	rcx,127		; we don't make note 127.. rsi is left at syn_events.
 	fld	dword [SYN_ADDR(rbp,syn_constants,syn_c0freq)]
%%loop:
	fld	st0		; duplicate, to make code self-similar to render below
	fstp	dword [rsi]
	add	rsi,4
 	fmul	dword [SYN_ADDR(rbp,syn_constants,syn_freqr)]
	dec	cl
	jnz	%%loop
	fstp	dword[rsi]
	add	rsi,4		; unnecessary, but makes opcodes absolutely self-similar

	pop	rbp
	
%endmacro
	


%macro	SYNTH_AUDIO_CALLBACK_CODE 0
;;; We get userdata in rdi, stream address in rsi, buffer length in rdx
;;; (Userdata not used as of now - use rdi for other stuff)
;;; (no need to align stack; we call no-one here; use red zone)
audio_callb_sdl:
	push	rbp		; Use this as ptr to synth bss
	push	rbx		; Use this as ptr to current event/sound
	push	rdx		; rsp+8: buffer length counter
	push	rsi		; rsp+0: stream address
	lea	rbp, [syn_bss]

aud_buf_loop:
	lea	esi, [syn_sequence] ; NOTE: Must use addresses that fit in 32 bits.
 	add	esi, [SYN_ADDR(rbp,syn_bss,syn_next_event_ind)]
	mov	ecx, dword [SYN_ADDR(rbp,syn_bss,synth_frames_done)]

;;; ------------------- Process sequence data:
	GPR_LD0	rax
	lodsb
;;;  	mul	dword [SYN_ADDR(rbp,syn_bss,syn_ticklen)]
;;;  	shl	rax,12	; would be smallest code, but tempo becomes restricted
 	mov	edx, SYN_TICKLEN
 	mul	edx
	add	rax,[SYN_ADDR(rbp,syn_bss,syn_prev_event_frame)] ; when to play?
	cmp	ecx,eax						 ; is it time now?
	jl	%%nothing_to_trigger				 ; not yet..

;;; ------------------- Trigger note, add to available slot in active notes:
	mov	[SYN_ADDR(rbp,syn_bss,syn_prev_event_frame)],rcx ; leave a mark..

  	lea	rbx,[ABS syn_events]
	
	lodsb
	add	word [SYN_ADDR(rbp,syn_bss,syn_next_event_ind)],2 ; read position upd.
	add	byte [SYN_ADDR(rbp,syn_bss,syn_current_note)],al ; note update
	jg	%%find_slot					 ; FIXME: Correct cond?
	GPR_LD0	rax
	mov	byte [SYN_ADDR(rbp,syn_bss,syn_current_note)],al
	mov	[SYN_ADDR(rbp,syn_bss,syn_next_event_ind)], rax ; rewind
	jmp	%%nothing_to_trigger
	
%%find_slot:
	cmp	byte [rbx + SYN_O_ACTIVE],0
	je	%%found_slot
	add	rbx,SYN_EVENT_SIZE
	jmp	%%find_slot	; NOTE: No limit checks; buffer overflow on bad data!
%%found_slot:
	;; Reset everything. Could fill with zeros first, if need be at some point
	mov	byte [rbx + SYN_O_ACTIVE],1
	mov	dword [rbx + SYN_O_TIME],0
	mov	al, byte [SYN_ADDR(rbp,syn_bss,syn_current_note)]
	mov	byte [rbx + SYN_O_NOTE],al

    	jmp	aud_buf_loop	; why not..
%%nothing_to_trigger:
	
;;; ------------------- Process currently active notes:
%%process_active_notes:

%if 0
;;; Debuggin:
 	fld	dword [SYN_ADDR(rbp,syn_bss,syn_note2f) + 4*57] ; frequency from look-up
	fimul	dword [synth_frames_done]
	fsin
	fidiv	dword [syn_i_2]

	fld	dword [SYN_ADDR(rbp,syn_bss,syn_note2f) + 4*67] ; frequency from look-up
	fimul	dword [synth_frames_done]
	fsin
	fidiv	dword [syn_i_2]	
	faddp
%else
  	fldz 			; load zero. Playing notes add to stacktop.
%endif


	;; rcx-counted loop:
	GPR_LDB	rcx,SYN_MAX_POLY
;;;  	GPR_MOV	rbx,r9	; Nah, these won't make it any better
;;; 	mov	rbx,[syn_events_address]
   	lea	rbx,[ABS syn_events]
	
%%find_active:
	cmp	byte [rbx],0
	je	%%find_active_continue
	
  	call	%%mix_sound_from_rbx ; subroutine just for code clarity; can inline later
%%find_active_continue:
	add	rbx,SYN_EVENT_SIZE
	loop	%%find_active

;;; ------------------- Output what has been added to st0:
	
%%do_mono_out:

	;; A small integer division could be a macro like FIDIV_B rsi,3 (needs the pop):
	push	3
	fidiv	dword [rsp]	; TODO: Volume per note?
	pop	rsi
;;; 	mov	dword [rsp-8],4
;;; 	fidiv	dword [rsp-8]	; TODO: Volume per note?
	;; Squash output.. distorted niceness..
	fld	st0		; cpy of x
	fmul	st1		; x^2
	fmul	st1		; x^3
	fmul	st1		; x^4
	fld1			; 1
	faddp			; x^4+1
	fdivp			; -> should become x/(x^4+1)
	
	;;  SDL2 Supports 32-bit float audio directly.. yiei!
	pop	rsi		; This is stored on stacktop while used for other purposes
	fld	st0		; duplicate stacktop, so following code can self-repeat
	fstp	dword [rsi]
	add	rsi,4
	fstp	dword [rsi]
	add	rsi,4
	push	rsi

 	inc	dword [synth_frames_done]
	
	sub	dword [rsp+8],8
	jnz	aud_buf_loop
	
	pop	rbx		; release stack space
	pop	rbp
	pop	rbx		; actual pops
	pop	rbp
	ret


	;; Subroutine for clarity.. could inline this.
	;; NOTE: This is cold-blooded stuff - don't destroy the register states here..
	
%%mix_sound_from_rbx:
	mov	eax,[rbx + SYN_O_NOTE]
	fld	dword [SYN_ADDR(rbp,syn_bss,syn_note2f) + 4*rax] ; frequency from look-up
	
%if 0
	;; Try a downward sloping pitch envelope.
	;; Would need this for any bass drum sound
	mov	dword [rsp-8], SYN_NOTE_LIFETIME
	fild	dword [rsp-8]
	fisub	dword [rbx + SYN_O_TIME]
	fidiv	dword [rsp-8]
	fld1
	faddp
	fmulp
%endif

	fimul	dword [rbx + SYN_O_TIME]
	fsin
	
	;; Downward sloping envelope. Necessary:
	mov	dword [rsp-8], SYN_NOTE_LIFETIME
	fild	dword [rsp-8]
	fisub	dword [rbx + SYN_O_TIME]
	fidiv	dword [rsp-8]
	fmulp
	
	;; Mix:
	faddp

	;; Add time and deactivate when lifetime is over:
	inc	dword [rbx + SYN_O_TIME]
	;; Deactivate when time is up:
	cmp	dword [rbx + SYN_O_TIME], SYN_NOTE_LIFETIME
	jl	%%skip_deactivate
	mov	byte [rbx + SYN_O_ACTIVE], 0
%%skip_deactivate:
	ret

	
%endmacro
