;;; The original beeper... not very useful :)
;;; We get userdata in rdi, stream address in rsi, buffer length in rdx
%macro	SYNTH_REQUIRED_DEFINES 0
%endmacro
%macro	SYNTH_CONSTANT_DATA 0
%endmacro
%macro	SYNTH_BSS_DATA 0
syn_bss:
synth_frames_done:	resq	1	; Global frame counter (read by main)
%endmacro
%macro	SYNTH_AUDIO_CALLBACK_CODE 0
	
audio_callb_sdl:
	push	rbx
	
aud_buf_loop:
	mov	eax, [synth_frames_done]
	mov	ebx,eax
	inc	eax
	mov	[synth_frames_done], eax
	
	
	shr	eax,10		; make a funny mask
	and	eax,0xc8
	
;;; 	mov	ecx,0x7fffffff
 	mov	ecx,0x3e800000	; .25
	and	ebx,eax
	jz	aud2
	mov	ecx,0xbe800000	; -.25
;;; 	mov	ecx,0x80000000
aud2:	
	mov	[rsi],ecx
	add	rsi,4
	mov	[rsi],ecx
	add	rsi,4
	sub	rdx,8
	jnz	aud_buf_loop
	
	pop	rbx
	ret
	
%endmacro
	
%macro	SYNTH_INITIALIZATION_CODE 0
%endmacro
	
