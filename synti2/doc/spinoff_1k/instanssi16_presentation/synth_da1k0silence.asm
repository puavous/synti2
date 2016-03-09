;;; Dummy synth that outputs silence - just for benchmarking the size of other synths
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
aud_buf_loop:
	mov	qword [rsi], 0
	inc	dword [synth_frames_done]
	add	rsi,8
	sub	edx,8
	jnz	aud_buf_loop
	ret
	
%endmacro
