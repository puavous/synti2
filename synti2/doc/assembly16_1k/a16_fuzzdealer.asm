;;; This is for NASM assembler. To be compiled as:
;;;   nasm -f bin -o a16_1k_x86.bin a16_1k_x86.asm 
;;; 
;;; Fuzzdealer - 7th place in Assembly Summer 2016 1k intro compo.
;;;              The only 1k entry this year on Linux!
;;; 
;;; No warranties. Every run is at your own risk!
;;;
;;; Author:  Paavo Nieminen <paavo.j.nieminen@jyu.fi>
;;;          known as "qma" since Stream Nine.
;;; 
;;; License: MIT License with an added wish; see LICENSE.txt
BITS 32

;;; Define ASSEMBLY_FINAL to produce forced FullHD resolution:
;;; %define ASSEMBLY_FINAL

;;; Break the ELF spec where the current Linux doesn't care to check
%define BREAK_THE_SPECIFICATION
	
;;; Misbehave where the assembly compo rules allow:
%define CRASH_AT_END

;;; Picked from the interface versions I happen to have installed currently
;;; Out of luck, if these don't work somewhere :)
	
%define SDL_INIT_TIMER	0x00000001
%define SDL_INIT_AUDIO	0x00000010
%define SDL_INIT_VIDEO	0x00000020
%define SDL_WINDOWPOS_UNDEFINED_MASK	0x1FFF0000
%define SDL_WINDOWPOS_UNDEFINED	0x1FFF0000
%define SDL_WINDOW_FULLSCREEN	0x00000001
%define SDL_WINDOW_OPENGL	0x00000002
%define SDL_KEYDOWN	0x300

;;;  (see SDL_audio.h for specs)
%define AUDIO_S32LSB 0x8020
%define AUDIO_F32LSB 0x8120
	
%define GL_VERTEX_SHADER	0x8b31 
%define GL_FRAGMENT_SHADER	0x8b30
%define GL_DEPTH_BUFFER_BIT 0x100
%define GL_COLOR_BUFFER_BIT 0x4000

%define SDL_WINDOW_FULLSCREEN_DESKTOP ( SDL_WINDOW_FULLSCREEN | 0x00001000 )

;;; From dlfcn.h
%define RTLD_NOW       2
%define RTLD_LAZY      1

;;; Zero bytes can be added in .bss to adjust mem_size to a
;;; value that compresses better. Large numbers can't be used with gdb
;;; debugging, though, because starting the program in gdb seems to take
;;; a very long time when the memory size grows... Not sure what magic it
;;; likes to perform before starting the program. Size tweaks seem to be OK
;;; for the executable otherwise.
%if 1
%define MEM_SIZE_ALIGN_BYTES 		0x1000000
%define MEM_SIZE_ADJUSTMENT_BYTES	0x6020100
%else
%define MEM_SIZE_ALIGN_BYTES 		0x100
%define MEM_SIZE_ADJUSTMENT_BYTES	0x000000
%endif
	
;;; If some 'rumors' are correct, Linux doesn't mind if ELF header gives wrong
;;; size for some sections. 
;;; Quirky hack, for sure, but saves me a byte each:
%ifdef BREAK_THE_SPECIFICATION
file_size_makebelieve    equ file_size + 0x100 - (file_size % 0x100) + 0x100 ; some extra
dynamic_size_makebelieve equ dynamic_size + 0x100 - (dynamic_size % 0x100)
e_ehsize_makebelieve     equ 0x00
reltext_size_makebelieve equ reltext_size + 0x28 + 0x35 ; tweaked - has implications!
%else
file_size_makebelieve equ file_size
dynamic_size_makebelieve equ dynamic_size
e_ehsize_makebelieve equ 0x34 ; (the actual size in elf32 spec.)
reltext_size_makebelieve equ reltext_size
%endif

;;; Hardcoded layout of file - implemented with zero paddings:
%define ZERO_PAD_UPTO(nxt) TIMES nxt + $$ - $	db	0
%define BSS_PAD_UPTO(nxt) TIMES nxt + $$ - $	resb	1
%define STRINGS_START 		0x0040	; ASCII strings (lib & func names, shaders)
%define SHADERSTRINGS_START	0x01b0	; ASCII string(s) for shader(s)
%define HEADERS_START		0x0400	; headers & sections other than ELF&prg hd.
;;; %define RELTEXT_START		0x04c0	; Relocation
%define PROGHEADERS_START	0x0500	; Program headers
%define SYNTH_CONST_START	0x0600 	; Synth constants
%define CODE_START		0x0700	; Actual program text
%define MAIN_DATA_START		0x0902	; Actual constant data for main program


;;; Minimal addressing for calls (3 bytes) is via rbp or rbx
;;; holding a base address less than 128 bytes away from target.
%define ADDR(r,base,a)   r + ((a) - base)
	
;;; Assembly requires forced FullHD(?) in fullscreen
%ifdef ASSEMBLY_FINAL
%define MY_WINDOW_WIDTH 1920
%define MY_WINDOW_HEIGHT 1080
%define MY_WINDOW_SPEC SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN
%else
;;; On my slow development laptop, I use a small window:
%define MY_WINDOW_WIDTH 1920/4
%define MY_WINDOW_HEIGHT 1080/4
%define MY_WINDOW_SPEC SDL_WINDOW_OPENGL
%endif


%ifndef	DISASS			; Define DISASS for easy "objdump -d"
;; Virtual memory load address..
;;	org	0x25490000	; could perhaps encode some data, if hacker wants? :)
;;;	org	0x400000	; The usual value
;;; 	org	0x380000	; Happens to include 0x38 == prg header entry sz
;;;   	org	0xff0000	; 'Rhymes' very well with rest of the deflated world
   	org	0x750000	; Could rhyme even better..?
	
%endif
	
;;; 32-bit ELF header
eh_start:	
		db	0x7F, "ELF"
		db	1 			; 32 bits (EI_CLASS)
		db	1 			; 2's compl. little endian (EI_DATA)
		db	1			; version 1 'current' (EI_VERSION)
		db	0			; sysv (EI_OSABI)
		db	0			; usually 0 (EI_ABIVERSION)
 	        db	0,  0,0,0,0,0,0		; unused '14 (EI_PAD[0..6])
	
		dw	2			; executable (e_type)
		dw	3			; 386 (e_machine)
		dd	1			; version 1 (e_version)
		dd	_start			; start (e_entry)

  		dd	prg_head-eh_start	; offset to prog hdrtab (e_phoff)
  		dd	0			; offset to sec hdrtab (e_shoff)
		dd	0			; flags (e_flags)
		dw	e_ehsize_makebelieve	; this hdr size (e_ehsize) can cheat??
		dw	0x20			; prog hdr entry size (e_phentsize)
		dw	4			; num entries in prog hdrtab (e_phnum)
 		dw	0			; sec hdr entry size (e_shentsize)
		dw	0			; num entries in sec hdrtab (e_shnum)
  		dw      0			; header entry with sec names (e_shstrndx)
	
	ZERO_PAD_UPTO(STRINGS_START)

;;; This is problematic to place.. compressibility of the header seems to vary quite
;;; a lot, depending on where this resides, and whether the length can be a good value.
;;; So far it looks OK here.. zopfli will nicely pack this along with other ASCII data. 
interp:
 		db	'/lib/ld-linux.so.2', 0 ; 19 bytes including \0 
interp_size equ $ - interp	; correct size up to here.

;;; Just can't avoid dlopen() and dlsym() ...? Here are the names (and the library).
;;; Order of strings seems to be the one that compresses best after everything is compiled (?).

dynstr:
dlopen_name equ $ - dynstr
	db	'dlopen', 0
libdl_name equ $ - dynstr
	db	'libdl.so.2', 0
dlsym_name equ $ - dynstr
	db	'dlsym', 0
dynstr_size equ $ - dynstr

	times 0xa	db 0	; could apply padding to make address more compressible
;;; Names of libraries and functions. Will use the above funcs to link the rest.
dli_strs:
;;; 	db	'libSDL2-2.0.so.0.2.1',0
	db	'libSDL2.so',0	
	db	'SDL_GL_SwapWindow',0
	db	'SDL_PollEvent',0
	db	'SDL_Quit',0
	
;;; ------------ sequentially ordered init starts here

	db	'SDL_Init',0
%ifdef USE_DESKTOP_DISPLAY_QUERY
	db	'SDL_GetDesktopDisplayMode',0
%endif
	db	'SDL_CreateWindow',0
	db	'SDL_GL_CreateContext',0
	db	'SDL_OpenAudio',0
	db	'SDL_PauseAudio',0
%ifndef FORGET_CURSOR
	db	'SDL_ShowCursor',0
%endif
	db	0
	db	'libGL.so.1',0
	db	'glCreateProgram',0
	db	'glCreateShader',0
	db	'glShaderSource',0
	db	'glCompileShader',0
	db	'glAttachShader',0
	db	'glLinkProgram',0
	
;;; ------------ sequentially ordered init up to here
	
	db	'glUseProgram',0
	db	'glGetUniformLocation',0
	db	'glUniform4fv',0
	db	'glClear',0
	db	'glRects',0
%ifdef USE_GL_TEXTURES
	db	'glGenTextures',0
	db	'glBindTexture',0
	db	'glTexParameteri',0
	db	'glTexImage2D',0
%endif
%ifdef	USE_GL_MATRICES	
	db	'glMatrixMode',0
	db	'glLoadIdentity',0
%endif
	db	0
%ifdef USE_FUNCTIONS_FROM_GLU
	db	'libGLU.so',0
	db	'gluNewQuadric',0
	db	'gluSphere',0
	db	'gluPerspective',0
	db	0
%endif
%ifdef USE_FUNCTIONS_FROM_LIBM
	db	'libm.so',0
	db	'powf',0
	db	0
%endif
	db	0		; End of dynamic function names.
	
 	ZERO_PAD_UPTO(SHADERSTRINGS_START)	
;;;  -------------------- shader string for this production
minimized_fragment_shader_str:
%if 1
  	db '#version 120',10
	db 'uniform vec4 u;'
	db 'void main(){'
	;;    Build a vec4 with time-varying integral pars; r.t used as iterative output
	db    'vec4 r=vec4(int(u.r/8/4),2,2,2+int(u.r/8));'
 	db    'for(int a=6;--a>0;){'
	db      'vec4 o=1-2*gl_FragCoord/u.tptt;'
  	db      'o.r/=u.p/u.t;' ; cartesian, with correct aspect
	;;      Main "Movement" (could have been stripped down more?):
 	db	'o=sin(a*'
	db               '(o'
	db                 '+2*a*sin(u.r/8+o.grrr*fract(u.r/8/3))'
	db                 '+u.r/8/3/2*a*sin(u.r/8+r.arrr*fract(u.r/8/3))'
	db               ')'
	db            ')'
	db        '/(1+fract(u.r/2*r.r))' ; pulsation
	db      ';'
       	db	'r.t=r.t*.4+sin(dot(o.st,o.st))*(.4-sin(r.a*atan(o.t,o.s)));'
 	db    '}'
    	db    'gl_FragColor='
    	db      'vec4(r.t*r.t*r.t,r.ttt)' ; cyan-ish specular glow
;;;   	db      'vec4(r.t)' ; plain B&W (many bytes smaller)
    	db      '*sin(u.r/8/3/2)' ; cheap-ish fade in (redundant? too much, even?)
   	db      '+u.r/8/4/3*u.r/8/4/3*u.r/8/4/3*u.r/8/4/3'  ; fade-to-white in end
	db    ';'
	db '}'
	db 0
%else
	;; A minimal "red screen" to compare actual size to the unavoidable overhead:
  	db '#version 120',10
	db 'void main(){'
    	db    'gl_FragColor=vec4(1.0,0.0,0.0,1.0);'
	db '}'
	db 10,0			; Check spec: need newline?? no.. no need.
%endif	

 	ZERO_PAD_UPTO(HEADERS_START)

;; Need to have a hash.. can tweak size and numbers
hash:
 		dd	1			; nbucket = 1
;;; s_string:	db	'u',0,0,0		; nchain=0x75 seems to be ok. 'u' str here.
 		dd	3   			; nchain whatever >2 (maybe?)
		dd	2			; start dynsym chain from this ind
 		dd	0, 0, 1			; the chain

;; Array of Elf32_Sym structures:
dynsym:
		dd	0	; u32 st_name
 		dd	0	; u64 st_value
		dd	0	; u64 st_size
		db	0	; uc st_info
		db	0	; uc st_other
		dw	0	; u16 st_shndx
dlopen_sym equ 1
		dd	dlopen_name
		dd	0
		dd	0
		db	0x12			; STB_GLOBAL, STT_FUNC
		db	0
		dw	0
dlsym_sym equ 2
		dd	dlsym_name
		dd	0
interp_size_makebelieve equ $ - interp ; can try to make the size aligned to 0x100
		dd	0
		db	0x12			; STB_GLOBAL, STT_FUNC
		db	0
		dw	0
dynsym_count equ 3
dynsym_size equ $ - dynsym
dynstr_size_makebelieve equ $ - dynstr +0xb ; make a nice size.

;;; 	ZERO_PAD_UPTO(RELTEXT_START)
  	ZERO_PAD_UPTO(HEADERS_START+0x75)

;;; times	0x2d	db	0	; Pad to exact offset hash+0x75
;; The relocation table. FIXME: Suspect, until properly verified.
;; It is array of type Elf32_Rela .. {u32 adr, u16 info, u32 addend}
	
reltext:
		dd	dlopen_rel
		dd	((dlopen_sym << 8) + 1)  ; R_386_32
		dd	0

		dd	dlsym_rel
		dd	((dlsym_sym << 8) + 1)   ; R_386_32
		dd	0
	
reltext_size equ $ - reltext
	
;;; 		dq	0	; allow for tweaked sizes..

;;; ------------------------------------------------------------

	ZERO_PAD_UPTO(PROGHEADERS_START)
	
;; Next, of type Elf32_Phdr:
prg_head:
		dd	3			; PT_INTERP
		dd	interp - $$		; file offset
		dd	interp			; virtual address
;;; s_string equ prg_head+10 ; so far best hack for compression?
		dd	0			; (phys addr.) unused
		dd	interp_size_makebelieve		; filesz
		dd	interp_size_makebelieve	        ; memsz
		dd	7			; flags PF_R
		dd	0			; align (used? how??)

		dd	1		; 1 = PT_LOAD (p_type)
		dd	0		; from file beg. (p_offset)
	        dd	$$		; to this addr (p_vaddr)
		dd	0     		; phys. addr. (p_paddr) (unused)
		dd	file_size_makebelieve ;filesize (p_filesz) 'can be > real?' 
 		dd	mem_size 	; mem size, must allow for data area
		dd	7		; 4 = PF_R (p_flags) 7=RWX
        	dd	0	  	; 'alignment' (p_align) (used? how?)
	
;;; There will be a "Bus error" around second page for memory access
;;; if cheated file size is too much greater than the actual size.
;;; Cheats have limits...

		dd	2			; PT_DYNAMIC
		dd	dynamic - $$		; file offset
		dd	dynamic			; virtual address
		dd	0			; (phys addr.) unused
		dd	dynamic_size_makebelieve		; filesz
		dd	dynamic_size_makebelieve            	; memsz
		dd	7			; flags PF_R | PF_W (..why w?)
		dd	0			; align (used? how??)

	;; Then one more header look-alike, marked unused, to make number of hdrs 4.
		dd	0			; PT_NULL (== unused)
		dd	0
	;; pad. Helpful? ..alignment to get simpler numbers as start of dynamic
	;; (the effect of these might depend on the other contents)
	;; can also affect the block count chosen by zopfli in two ways)
;;; 		dd	0,0,0,0,0,0,0,0,0,0
	
;;; Wow, we don't need to be aligned.. so let us start from an offset with lsb 0x75:
		dd	0,0,0
		db	0


	;; In theory, could have some code like audio callback here? Maybe?

;; The .dynamic section. Indicates the presence and location of the
;; dynamic symbol section (and associated string table and hash table)
;; and the relocation section.

;; Dynamic section entries, of type Elf64_Dyn {u64, u64}

dynamic:
		dd	5,  dynstr		; DT_STRTAB
		dd	1,  libdl_name		; DT_NEEDED
       		dd	7,  reltext		; DT_RELA
		dd	4,  hash		; DT_HASH
		dd	10, dynstr_size_makebelieve		; DT_STRSZ
		dd	6,  dynsym		; DT_SYMTAB
		dd	8,  reltext_size_makebelieve	; DT_RELASZ
		dd	9,  12			; DT_RELAENT = sizeof(Elf32_Rela)
		dd	11, 16			; DT_SYMENT = sizeof(Elf32_Sym)
	
;;; The required DT_NULL content could be part of next section (size modified with +16):
		dd	0,0			; DT_NULL 

dynamic_size equ $ - dynamic

	




;;; Macro to skip zeros in string. Compresses very well; no need for function call
;;; Could increase 32-bit register with shorter uncompressed code, but somehow
;;; doesn't compress as well as when using full 64-bit instructions for %1 and %2.
%macro MAC_skip_str_at_esi 0
%%skip_str_at_esi:
	lodsb
;;;  	cmp	al, byte 0	; zero ends string
;;; 	and	al,al	; another two byte opcode.. but cmp compresses better
 	shr	al, 1	; .. but can also stop with either 0 or 1; same code in synth
	jnz	%%skip_str_at_esi
%endmacro
	
	ZERO_PAD_UPTO(SYNTH_CONST_START)

;;;  ------------------------------------------------------ synth constants
	
syn_constants:
synth_env_state:
	dd	0
syn_seqptr:
	dd	syn_miniseq
;;; Integer parameters for the output:
syn_params:
syn_tiks:	dd	0
syn_steplen:
syn_unused:	dd	0
syn_nvol:	dd	0
syn_dlen:	dd	0
syn_dvol:	dd	0
syn_lsrc:	dd	0
syn_lvol:	dd	0
syn_p7:		dd	0
	
synth_frames_done:
	dd	0
syn_currentf:
	dd	0
syn_c0freq:
syn_basevol:
   	dd	0.004138524	; 0x3b879c75; close to MIDI note 24 freq / sr * 2 * pi	
;;; 	dd	0.016554097	; 0x3c879c75; close to MIDI note 0x30
syn_freqr:
;;; 	dd	1.0594630943592953  ; freq. ratio between notes
	dd	1.0594622	; 0x3f879c75; close to 1.0594630943592953
syn_ticklen:
	dd	0x1770   	; sequencer tick length. 0x1770 is 120bpm \w 4-tick note

%define STEP_LEN(val)  (0<<(1))+1,(val) ,
%define NOTE_VOL(val)  (2<<(1))+1,(val) ,
%define DLAY_LEN(val)  (3<<(1))+1,(val) ,
%define DLAY_VOL(val)  (4<<(1))+1,(val) ,
%define LOOP_SRC(val)  (5<<(1))+1,(val) ,
%define LOOP_VOL(val)  (6<<(1))+1,(val) ,

;;; Let me make these notes as octaves and note names...
%define c 0
%define C 1
%define d 2
%define D 3
%define e 4
%define f 5
%define F 6
%define g 7
%define G 8
%define a 9
%define b 11
%define n(p,o) ((-24 + p+12*o)<<1),
%define pause  0,
	
syn_miniseq:
	;; Played one byte at a time:
	
	db	STEP_LEN(2) 	; This must be set. Others are nicely 0-inited.
	db	DLAY_VOL(0x75)
	db	NOTE_VOL(0x75)  
	db      DLAY_LEN(3) 
	db	LOOP_VOL(0xff)
	db	LOOP_SRC(16)
	
	;; Intro
 	db	n(f,3) 	n(f,3) 	n(e,4)	n(f,4)
 	db	pause 	n(f,3) 	n(e,4)	n(f,4)
	db	pause 	pause   pause   pause
	db	pause 	pause   pause   pause

	db	pause 	pause   pause   pause
	db	pause 	pause   pause   pause
	db	pause 	pause   pause   pause
	db	pause 	pause   pause   pause
	
	db	pause 	pause   pause   pause
	db	pause 	pause   pause   pause
	db	pause 	pause   pause   pause
	db	pause 	pause   pause   pause

	db	pause 	pause   pause   pause
	db	pause 	pause   pause   pause
	db	pause 	pause   pause   pause
	db	pause 	pause   pause   pause
	
	;; Little variance from this point onwards; start longer steps
	db	pause   n(b,4)	pause   pause
	db	STEP_LEN(8)
	db	pause
	db	pause
	db	pause
	
	db	pause
	db	pause
	db	pause
	db	pause
	

	;; Bass finally in.
	db	n(f,2) pause pause pause
	db	pause  pause pause pause
	db	pause  pause pause pause
	db	pause  pause pause pause
	
	db	pause  pause pause pause
	db	pause  pause pause pause
	db	pause  pause pause pause
	db	pause  pause pause pause
	
	db	pause  pause pause pause
  	db	pause  pause pause pause
	
	;; Then the groovy theme with a "chord"
	
	db	pause  n(f,3) pause n(a,4)
	db	pause  n(b,4) pause n(c,5)
	
	db	pause  n(f,3) pause n(a,4)
	db	pause  n(b,4) pause n(c,5)
	
	db	pause  n(f,3) pause n(a,4)
	db	pause  n(b,4) pause n(c,5)
	
	db	pause  n(f,3) pause n(a,4)

	;; Finale with repeated note
	db	LOOP_SRC(8)
	db      pause pause pause pause
	
	;; Fade-out with a short loop at low volume
	db      pause LOOP_SRC(4) LOOP_VOL(0x30)

	ZERO_PAD_UPTO(CODE_START)

;;;  ------------------------------------------------------ main code start
;;;  Entry point
_start:

init_everything:
  	mov	ebp,_base1
s_string equ init_everything+0x3 ; Assumes 'mov ebp,_base1' as the
				; first instr and program loaded at
				; address 0x0075****. Then we have such
				; opcodes that start+3 is also the string "u\0"
	
;; ---------------------------------
;;;  Link DLLs.
;;; ---------------------------------

fill_dli_table:	

	mov	esi,dli_strs
	lea	edi,[ADDR(ebp,_base1,fptrs)] ; edi points to output array

dli_libloop:
	;;  Expect always at least one library (end condition checked only afterwards)
	push	dword RTLD_NOW
	push	esi
	call	[ADDR(ebp,_base1,dlopen_rel)]
	mov	[ADDR(ebp,_base1,handle)],eax

 	MAC_skip_str_at_esi
	
dli_funcloop:
	;; Expect always at least one function (end condition checked only afterwards)
	push	esi
	push	dword [ADDR(ebp,_base1,handle)]
 	call	[ADDR(ebp,_base1,dlsym_rel)]
	stosd			; store fptr & move to next location
	
 	MAC_skip_str_at_esi
	
 	cmp	[esi], byte 0	; zero ends funcs
	jne	dli_funcloop
	lodsb			; increases esi; reduces diversity of instructions
 	cmp	[esi], byte 0	; zero ends input
	jne	dli_libloop
	
dli_over:

;;; Always call just "reg++". Macro for the call:
%macro CALLNXT 0
	lodsd
	call	eax
%endmacro
;;; TODO: Macro for rewinding the call sequence.
	
	lea	esi,[ADDR(ebp,_base1,SDL_Init_p)] ; call "esi++" every time..

;;; ---------------------------------
;;; SDL2 Init: window, GL, audio
;;; ---------------------------------

init_sdl:
	push	(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER)
	CALLNXT			;SDL_Init_p
	
	push	dword MY_WINDOW_SPEC
	push	dword [ADDR(ebp,_base1,screen_h)]
	push	dword [ADDR(ebp,_base1,screen_w)]
	push	dword SDL_WINDOWPOS_UNDEFINED
	push	dword SDL_WINDOWPOS_UNDEFINED
	push	dword 0
	CALLNXT			;SDL_CreateWindow_p
	mov	[ADDR(ebp,_base1,sdl_pwindow)],eax

 	push	dword [ADDR(ebp,_base1,sdl_pwindow)]
	CALLNXT			;SDL_GL_CreateContext_p
	
	;; (return value is GL context ID - unused, discard)

 	push	dword 0
	push	desired_sdl_audio_spec
;;; 	lea	eax,[ADDR(ebp,_base1,desired_sdl_audio_spec)]
;;;  	push	eax
	CALLNXT			;SDL_OpenAudio_p
	
	push	dword 0
	CALLNXT			;SDL_PauseAudio_p
	
%ifndef FORGET_CURSOR
	push	dword 0
	CALLNXT 		;SDL_ShowCursor_p
%endif
	
;;; ---------------------------------
;;; Compile shaders
;;; ---------------------------------

compile_shaders:

	push	ebx		; parameter not used, but could make repeatable bytes?
	CALLNXT			;glCreateProgram_p
	mov	[ADDR(ebp,_base1,shader_pid)],eax

	push	dword GL_FRAGMENT_SHADER
	CALLNXT			;glCreateShader_p
	mov	ebx,eax		; in the 386 ABI, we can spare a preserved tmp register.
	
	push	0
	push	ptr_to_fshader
	push	1
	push	ebx
	CALLNXT			;glShaderSource_p (shaderID, count, ptr_to_fshader, NULL)

	push	ebx
	CALLNXT			;glCompileShader_p (shaderID)
	
	push	ebx
	push	dword [ADDR(ebp,_base1,shader_pid)]
	CALLNXT			;glAttachShader_p (programID, shaderID)
	
	push	ebx		; parameter not used, but could make repeatable bytes?
 	push	dword [ADDR(ebp,_base1,shader_pid)]
	CALLNXT			;glLinkProgram_p (programID)

;;; 	add	esp,80	; caller's pops for the above.. who cares.. we have space in stack..
	
;;; ------------------------------- end of INIT
	
loop:
	
;;; ------------------------------------------- start render with shaded rect
 	lea	edi,[ADDR(ebp,_base1,some_floats)]
	
	;; Provide some_floats = {time_s, screen_w, screen_h}
	
;;; If synth_frames_done is far from _base1 then an abs address is better than equally
;;; long, and equally not-used-elsewhere, base+offset.
;;;    	fild	dword [ADDR(ebp,_base1,synth_frames_done)]
 	fild	dword [synth_frames_done]
	
 	fidiv	dword [ADDR(ebp,_base1,synth_sr_i)]
	fstp	dword [edi]
	scasd
	fild	dword [ADDR(ebp,_base1,screen_w)]
	fstp	dword [edi]
	scasd
	fild	dword [ADDR(ebp,_base1,screen_h)]
	fstp	dword [edi]
	scasd
	
 	lea	esi,[ADDR(ebp,_base1,glUseProgram_p)] ; rewind "reg++" calls

	push	dword [ADDR(ebp,_base1,shader_pid)]
	CALLNXT			;glUseProgram_p
	
;;;   	lea	ebx,[ADDR(ebp,_base1,s_string)]
;;; 	push	ebx
 	push	dword s_string
	push	dword [ADDR(ebp,_base1,shader_pid)]
	CALLNXT			;glGetUniformLocation_p

;;; 	lea	edx,[ADDR(ebp,_base1,some_floats)]	;state to visualize, as vec4[]
;;; 	push	edx
	push	dword some_floats
	push	1 			; number of state vectors to visualize
	push	eax			; location as given by above call
	CALLNXT				;glUniform4fv_p
	
 	push	dword (GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT)
	CALLNXT			;glClear_p

	push	-1
	push	0x75		; 0x75 ('u') is very common, so only few bits
	push	0x75		; OpenGL will clip the large rectangle nicely.
	push	-1
	CALLNXT			;glRects_p
	
;;; ------------------------------------------- end render with shaded rect

 	lea	esi,[ADDR(ebp,_base1,SDL_GL_SwapWindow_p)] ; rewind "reg++" calls
	
	push    dword [ADDR(ebp,_base1,sdl_pwindow)]
	CALLNXT			;GL_SwapWindow_p
	
;;; 	lea 	eax,[ADDR(ebp,_base1,sdl_event_space)]
;;; 	push	eax
	push	sdl_event_space
	CALLNXT			;SDL_PollEvent_p

;;;     	add	esp,52 ; caller's pop for all of the above calls!
;;; Observe: Could be many bytes smaller if just let the stack fill
;;; up. One minute With 100 fps would yield. 60*100*52 bytes = 312000
;;; bytes of excess stack usage. How much can we spare?.
;;; Also, "add esp,48" would "leak" only 60*100*4 = 24000 bytes per
;;; minute, but might compress much better..
	

;;; Stop when time is up (as 48kHz frames):
   	cmp	dword [synth_frames_done],0x490000 ;close to 48000*100 (1m 40s)
	
;;; Byte comparison might be shorter, if there is a suitable value:
;;;  	cmp	[ADDR(ebp,_base1,synth_frames_done+2)], byte 0x3f ; 60 seconds plus a little
 	jg	event_loop_is_over

;;; Stop at keypress event:
%if 0
	;; With many attempts, I didn't figure out a shorter way to test:
	cmp	[ADDR(ebp,_base1,sdl_event_space)], word SDL_KEYDOWN
%else
	;; But then, of course this is shorter than the previous one:
 	lea	edi,[ADDR(ebp,_base1,sdl_event_space)]
 	mov	ah, (SDL_KEYDOWN >> 8) ; Looks weird.. see the comment below.
 	scasd
	;; The AH update works, if SDL_PollEvent returns 0 in EAX when the event is
	;; copied to sdl_event_space. Using words from the SDL docs, should be
	;; that there are "no pending events" when keydown reaches us here.
	;; 
	;; Now then, if EAX is proper, then scasd can be used for comparison because
	;; the first field of SDL_WhateverEvent is exactly 'Uint32 type;'.
	;; Looks like the event type is not updated when polling without events.
	;; Can't check only one byte at [edi+1] because we re-use the space
	;; for streaming floats to gfx, and it is easy to end up with a
	;; value with 0x****03** in there. But 0x300 as dword won't be out there so 
	;; scasd seems to be a robust test. Very few bits need to be stored in the
	;; deflate stream now, because we mostly repeat earlier code.
	;; 
	;; Yet another observation from SDL_events.h: keyboard events seem to be the
	;; only ones with higher byte 0x03 .. try to exploit this instead!? Can't..
	;; see above about sharing space with floats.
	;;
	;; But... Is the space sharing necessary with the 386 version?
	;; TODO: check this.. current version is mostly copy-paste from an x86-64 version.
	;; 
%endif

	jne	loop
event_loop_is_over:
	
	CALLNXT			;SDL_Quit_p()

%ifndef CRASH_AT_END
	;; Exit with whatever code.. doesn't matter, really..
	mov     eax,1			; 1 == exit system call
	int	0x80			; exit(whatever_in_BL) - repeat to pad.
%else
;;; TODO: Hmm, the show is basically over already, so could I just crash it?
;;; Maybe not... there will be a rather inpolite core dump. Well, illegal
;;; instruction of zero opcodes will yield a tempting 4 bytes shorter code than proper
;;; exit, so I'll leave it here as something to try in a desperate occasion.
  	db	0,0
%endif
	

;;; -------------------------------------------- synth callback code
;;; See SDL API for callback params.
audio_callb_sdl:
	pushad
 	mov	ebp, syn_constants
 	mov	esi,dword [ADDR(ebp,syn_constants,syn_seqptr)]
aud_buf_loop:
	mov	edi, syn_constants
	;; ---------------------------------------------------------------------
	;; Only reconfigure state when step length has been reached:
	mov	eax,dword [ADDR(ebp,syn_constants,syn_steplen)]
;;; 	cmp	eax,dword [ADDR(ebp,syn_constants,synth_env_state)]
	scasd
	jne	skip_read
	
	;; ---------------------------------------------------------------------
	;; Read a byte and reconfigure state:
	;; First, reset envelope state:
	sub	eax,eax
	mov	dword [ADDR(ebp,syn_constants,synth_env_state)],eax

read_from_sequence:
	;; Read next byte from sequence to CL, and update counter:
	lodsb
	;; Shift last bit out to see if we have a 7-bit note or a control byte:
 	shr	al,1
	mov	ecx,eax		; EAX was 0, so now CL==ECX==EAX==AL. mov changes no flag.
	;; ^wow, that was a bug in the entry; lucky it worked at asm.. mov cl,al was intended
	jnc	new_note

	lodsb
	;; Store parameter
	mov	[ADDR(ebp,syn_constants,syn_params) + 4*ecx],eax
	jmp	read_from_sequence
	
new_note:
	;;  	mov	dword [ADDR(ebp,syn_constants,syn_seqptr)], esi
	
	;; We have a note. Compute new frequency or remain at 0 Hz ("silence")
	;; NOTE: Zero flag must be set if CL==0 !
	fldz
	jz	store_frequency
	fadd	dword [ADDR(ebp,syn_constants,syn_c0freq)]
syn_loop:
	fmul	dword [ADDR(ebp,syn_constants,syn_freqr)]
	loop	syn_loop
store_frequency:
 	fstp	dword[ADDR(ebp,syn_constants,syn_currentf)]
	
skip_read:
	;; Re-compute based on params (may have just changed)
	scasd
	mov	eax,[edi]
	mul	dword [ADDR(ebp,syn_constants,syn_ticklen)]
 	scasd
 	mov	dword [edi],eax

	fld1							; (1)
 	fild	dword [ADDR(ebp,syn_constants,synth_env_state)] ; (1 ienv)
	fidiv	dword [edi]					; (1 rise)
	fsubp   						; (fall)
	
	;; Intend to play sin(2pi*frequency*framecount/srate)

 	fld	dword [ADDR(ebp,syn_constants,syn_currentf)] ;(fall note)
   	fimul	dword [ADDR(ebp,syn_constants,synth_env_state)] ; (fall note*iphase)
  	fld	st0			; (fall note*iphase note*iphase)
 	fadd	st0			; (fall note*iphase 2*note*iphase)
  	fadd	st0			; (fall note*iphase 4*note*iphase)
	fsin				; (fall note*iphase sin(4*note*iphase))
	faddp	   			; (fall note*iphase+sin(4*note*iphase))
	fsin				; (fall sin(note*iphase+sin(4*note*iphase)))
	fmulp   			; (fall*sin(note*iphase+sin(4*note*iphase)))
					; == (audio)

	;; Overall volume:
	scasd
	fimul	dword [edi]
	fmul	dword [ADDR(ebp,syn_constants,syn_basevol)]

	;; ---- Delay thingy.
	;; collect shorter delay:
	;; Compute short delay length:
	scasd
	mov	eax,[edi]
	mul	dword [ADDR(ebp,syn_constants,syn_ticklen)]
	mov	ecx,[ADDR(ebp,syn_constants,synth_frames_done)]
	sub	ecx,eax
	
	fld	dword [syn_dly+4*ecx] ;(audio delayed)
	scasd
	fimul	dword [edi]
	fmul	dword [ADDR(ebp,syn_constants,syn_basevol)]
	faddp				;(audio+dvol*delayed)
	fld	st0			;(audio+dvol*delayed audio+dvol*delayed)
	
	;; collect longer delay from history, with loop volume:
	scasd
	mov	eax,[edi]
	mul	dword [ADDR(ebp,syn_constants,syn_ticklen)]
	mov	ecx,[ADDR(ebp,syn_constants,synth_frames_done)]
	sub	ecx,eax
	
	fld	dword [syn_rec+4*ecx] ;(dly dly looped)
	scasd
	fimul	dword [edi]
	fmul	dword [ADDR(ebp,syn_constants,syn_basevol)] ;(dly dly lvol*looped)
	faddp			;(dly dly+lvol*looped)
	fld	st0		;(dly mix mix)
nomore:	
	
	;; ---- Delay thingy ends.

;;; Make a smooth distortion for output (some +8 bytes compressed..)
	fld	st0		; (x x)
	fabs			; (x |x|)
	fld1			; (x |x| 1)
	faddp			; (x 1+|x|)
	fdivp			; (x/(1+|x|))

	;;  Finally store. Fp stack is now: (dly mix final)
	mov	ecx,[ADDR(ebp,syn_constants,synth_frames_done)]
	mov	edi,dword [esp+32+8]	; buffer location
	
	;; Increase buffer pointer by 4 .. compresses well after the previous opcode.
 	inc	dword [esp+32+8] ; increase buf ptr
  	inc	dword [esp+32+8] ; increase buf ptr
  	inc	dword [esp+32+8] ; increase buf ptr
  	inc	dword [esp+32+8] ; increase buf ptr

	inc	dword [ADDR(ebp,syn_constants,synth_frames_done)]
	inc	dword [ADDR(ebp,syn_constants,synth_env_state)]
	
	fstp	dword [edi]		; -> buffer (dly mix)
	fstp	dword [syn_rec+4*ecx] 	; -> mix (dly)
	fstp	dword [syn_dly+4*ecx] 	; -> delay ()
	
	;; Subtract 4 from buffer length, and loop if 0 not reached
	dec     dword [esp+32+12] ; length in bytes
	dec     dword [esp+32+12] ; length in bytes
	dec     dword [esp+32+12] ; length in bytes
	dec     dword [esp+32+12] ; length in bytes
	
	jnz	aud_buf_loop
	
 	mov	dword [ADDR(ebp,syn_constants,syn_seqptr)], esi
	popad
	ret

	ZERO_PAD_UPTO(MAIN_DATA_START)
	
screen_w:	dd	MY_WINDOW_WIDTH
screen_h:	dd	MY_WINDOW_HEIGHT

;;; Note that most of this structure might be required only once in the audio init.
;;; Could allocate for other uses after that. (For now, we use sample rate later)
;;; Although, should check if SDL2 will be using this later.. there are these "calculated"
;;; fields, so it makes one wonder if they are used somewhere...
desired_sdl_audio_spec:
synth_sr_i:
	dd	48000		; freq as int 
 	dw	AUDIO_F32LSB	; format as SDL_AudioFormat
	db	1		; channels as Uint8; we ain't no stereo.. 1==mono
	db	0		; silence (calculated) as Uint8
	dw	1024		; samples as Uint16
	dw	0		; padding as Uint16 (compiler compat.)
	dd	0		; size as Uint32 (calculated)
  	dd	audio_callb_sdl	; callback as SDL_AudioCallback
%ifdef SYN_USERDATA_INIT
	dd	SYN_USERDATA_INIT
%else
  	dd	0		; userdata as void* (could actually omit totally)
%endif
	

ptr_to_fshader:	dd	minimized_fragment_shader_str
;;; s_string:	db	'u',0

file_size equ $ - $$

;;
;; Beginning of the program's bss section.
;; 

ABSOLUTE $

ALIGNB 0x4
			resd	4 ; pad to make addresses more compressible

			resd	1 ; pad to get dlopen_rel to a nice loc.

handle:
sdl_pwindow:			; Re-use this space after last call to dlopen()
tmpvar:			resd	1	; hmm.. use this as a local temp

_base1:
;;;  Pointers to functions after relocation ():
shader_pid:			; Re-use this space after last dlopen()
dlopen_rel:
			resd	1
 			resd	1 ; Match locations for compressible representations
dlsym_rel:
			resd	1

fptrs:
;;; ------------ sequentially ordered funcs (evloop) start here
SDL_GL_SwapWindow_p:	resd	1
SDL_PollEvent_p:	resd	1
;;; ------------ sequentially ordered funcs (evloop) end here
SDL_Quit_p:		resd	1

;;; ------------ sequentially ordered init starts here

SDL_Init_p:		resd	1
%ifdef USE_DESKTOP_DISPLAY_QUERY
SDL_GetDesktopDisplayMode_p:	resd	1
%endif
SDL_CreateWindow_p:	resd	1
SDL_GL_CreateContext_p:	resd	1
SDL_OpenAudio_p:	resd	1
SDL_PauseAudio_p:	resd	1
%ifndef FORGET_CURSOR
SDL_ShowCursor_p:	resd	1
%endif
glCreateProgram_p:	resd	1
glCreateShader_p:	resd	1
glShaderSource_p:	resd	1
glCompileShader_p:	resd	1
glAttachShader_p:	resd	1
glLinkProgram_p:	resd	1

;;; ------------ sequentially ordered init up to here

;;; ------------ sequentially ordered funcs (render) start here
glUseProgram_p:		resd	1
glGetUniformLocation_p:	resd	1
glUniform1fv_p:		resd	1
glClear_p:		resd	1
glRects_p:		resd	1
;;; ------------ sequentially ordered funcs (render) end

%ifdef USE_GL_TEXTURES	
glGenTextures_p:	resd	1
glBindTexture_p:	resd	1
glTexParameteri_p:	resd	1
glTexImage2D_p:		resd	1
%endif

%ifdef USE_GL_MATRICES	
glMatrixMode_p:		resd	1
glLoadIdentity_p:	resd	1
%endif

%ifdef USE_FUNCTIONS_FROM_GLU	
gluNewQuadric_p:	resd	1
gluSphere_p:		resd	1
gluPerspective_p:	resd	1
%endif

%ifdef USE_FUNCTIONS_FROM_LIBM
powf_p:			resd	1
%endif

;;; The following 7x8 bytes can be re-used after init:
;;; sdl_event_space (56 bytes): used when polling SDL event, once in a loop.
;;; some_floats (max 14x4 = 56 bytes) are recomputed for every visualization frame.
;;; The memory overlaps, which is not a problem, because structures are filled before
;;; use every time.

sdl_event_space:		; used at the end of ev loop
some_floats:			; used while rendering, once per ev loop round
			resb	56
	
	;; syn_loop_bottom:
	;; 	resd	0x400000	; Storage for initial zeros of the loop delay
BSS_PAD_UPTO(0x1000000-0x750000); Aiming for compressible absolute address here.
	
;;; BSS data for synth:
synth_bss_base:			; <-- can vary location of base.
	
syn_rec:
	resd	0x400000
syn_dly:
	resd	0x400000

;;; First, a power-of-two alignment, and then any number of zeros to make a nicely
;;; compressible value for the program size:
ALIGNB	MEM_SIZE_ALIGN_BYTES
TIMES	MEM_SIZE_ADJUSTMENT_BYTES	resb	1

mem_size equ $ - $$
