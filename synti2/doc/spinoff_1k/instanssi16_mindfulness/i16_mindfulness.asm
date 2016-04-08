;;; This is for NASM assembler. To be compiled as:
;;;   nasm -f bin -o minfulness.exe i16_mindfulness.asm
;;; 
;;; Tested on two 64-bit linux platforms (x86_64): Current Fedora and
;;; current Debian. SDL2 and OpenGL libraries required. No guarantees
;;; on any other platform.
;;;
;;; No warranties. Every run is at your own risk!
;;;
;;; Author:  Paavo Nieminen <paavo.j.nieminen@jyu.fi>
;;; License: MIT License with an added wish; see LICENSE.txt
BITS 64
DEFAULT REL
;;; This was my "Universal Mandala Mindfulness Colouring Book 1k"
;;; entry for the Instanssi 2016 compo "Pikkiriikkinen demo" - roughly
;;; equivalent to a "4k intro" competition. This entry fits in 1040
;;; bytes, though.
;;;
;;; This is a spinoff project from my earlier "4k thingys", just to
;;; find the actual limits of overhead in 64-bit Linux binaries,
;;; and to see if there is any chance in making x86_64 entries in 1k
;;; intro compos these days. Not so many have been submitted to compos
;;; in the last few years...
;;;
;;; The verdict is.. with my skills, it is very difficult to get the
;;; overhead (==necessary code just to get the window and audio
;;; running) in less than 780 bytes (compressed), so there is very
;;; little space for anything "creative". Someone else could have
;;; much better ideas - please, let me know, if you can crunch this
;;; in a smaller code! I really want to know how you pull it off!
;;;
;;; I'm linking against SDL2 and OpenGL, like `fit' did it back in the
;;; days. In this one, I'm drawing one rectangle and applying a fragment
;;; shader to paint the pixels. For the record, I have tried the obvious
;;; alternative versions: (1) open a framebuffer and do software
;;; rendering, and (2) use plain OpenGL calls to generate fixed-pipeline
;;; primitives. The overhead from both alternatives turned out to be
;;; comparable to the one we witness with GLSL, and, for the moment, I
;;; felt that I'm better off creating content with GLSL. So I stuck
;;; with that. Somebody more experienced with
;;; generative graphics using plain OpenGL primitives could do a
;;; nice job without GLSL in a similar 1k production. All those bytes
;;; where I say "gl_fragCoord" could be used to create primitives on
;;; screen.
;;; 
;;; Here is what I could do this time:
;;; The graphics try to mimic a "colouring book for grown-ups"
;;; -kind of thing, with stroked patterns that you'd fill with
;;; coloured pencils. The audio tries to augment this "mindfulness"
;;; idea by emulating a "Tibetan singing bowl", although the bowl is
;;; struck instead of sweeped in this case.
;;;
;;; Some "random errors" accidentally made on the
;;; way provided perhaps more intriguing graphics or sounds, but I'm
;;; very happy that I was able to keep the original concept throughout
;;; the production. Feel free to experiment with the codes to generate
;;; such "more interesting" graphics which are very much behind the
;;; corner!! Just change the shader code a little bit...

%undef DEBUG
	
%include "debug_macros.asm"
%include "definitions_from_libs.asm"
%include "general_macros.asm"


;;; Zero bytes can be added in .bss to adjust mem_size to a
;;; value that compresses better. Large numbers can't be used with gdb
;;; debugging, though, because starting the program in gdb seems to take
;;; a very long time when the memory size grows... Not sure what magic it
;;; likes to perform before starting the program. Size tweaks seem to be OK
;;; for the executable otherwise.
%if 1
%define MEM_SIZE_ALIGN_BYTES 		0x0400000
%define MEM_SIZE_ADJUSTMENT_BYTES	0x0400000
%else
%define MEM_SIZE_ALIGN_BYTES 		0x100
%define MEM_SIZE_ADJUSTMENT_BYTES	0x000000
%endif
	
;;; Here, you can choose to %undef instead of %define ...
%define BREAK_THE_SPECIFICATION
	
;;; If some 'rumors' are correct, Linux doesn't mind if ELF header gives wrong
;;; size for some sections. 
;;; Quirky hack, for sure, but saves me a byte each:
%ifdef BREAK_THE_SPECIFICATION
file_size_makebelieve    equ file_size + 0x100 - (file_size % 0x100) + 0x100 ; some extra
dynamic_size_makebelieve equ dynamic_size + 0x100 - (dynamic_size % 0x100)
e_ehsize_makebelieve     equ 0x00
reltext_size_makebelieve equ reltext_size + 0x10 ; tweaked
%else
file_size_makebelieve equ file_size
dynamic_size_makebelieve equ dynamic_size
e_ehsize_makebelieve equ 0x40 ; (the actual size in elf64 spec.)
reltext_size_makebelieve equ reltext_size
%endif

;;; Hardcoded layout of file - implemented with zero paddings:
%define ZERO_PAD_UPTO(nxt) TIMES nxt + $$ - $	db	0
%define STRINGS_START 		0x0040	; ASCII strings (lib & func names, shaders)
;;; %define SHADERSTRINGS_START	0x01c0	; ASCII string(s) for shader(s)
%define SHADERSTRINGS_START	0x01ab	; ASCII string(s) for shader(s)
%define HEADERS_START		0x0400	; headers & sections other than ELF&prg hd.
%define RELTEXT_START		0x04c0	; Relocation
%define PROGHEADERS_START	0x0500	; Program headers
%define SYNTH_CONST_START	0x06c0	; Synth constants
%define CODE_START		0x06e0	; Actual program text
;;; %define SYNTH_PATTERNS_START	0x0900	; Synth song patterns
%define MAIN_DATA_START		0x08e8	; Actual constant data for main program


;;; Minimal addressing for calls (3 bytes) is via rbp or rbx
;;; holding a base address less than 128 bytes away from target.
%define ADDR(r,base,a)   r + ((a) - base)
	
;;; The preferrable screen size at Instanssi is 1280x720. Final production fullscreen:
%define MY_WINDOW_WIDTH 1280
%define MY_WINDOW_HEIGHT 720
%define MY_WINDOW_SPEC SDL_WINDOW_OPENGL
;;; %define MY_WINDOW_SPEC SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN
	
;;;  Definitions for the synth code:
%define SYN_REPEAT_LEN 48000*6 ; cycle of sequence
%define SYN_DLY_LEN 48080*5	 ; cycle time of the delay
%define SYN_DLY_BUF_SIZE 48000*10 ; buffer size


%ifndef	DISASS			; Define DISASS for easy "objdump -d"
;; Virtual memory load address..
;;	org	0x25490000	; could perhaps encode some data, if hacker wants? :)
;;;	org	0x400000	; The usual value
;;; 	org	0x380000	; Happens to include 0x38 == prg header entry sz
;;;  	org	0xff0000	; 'Rhymes' very well with rest of the deflated world
  	org	0x750000	; Could rhyme even better..?
	
%endif

;;; The gzip puzzle game with the headers and other parts is as follows. There
;;; are some contiguous sections that we absolutely need, and we want to store them
;;; so that any crossreferences between them are encoded in a way favored by the
;;; compression algorithm. Observe possibilities with the unavoidable rip-relative
;;; or absolute indices to constant data. Also, the specified lengths may not be
;;; checked or used in the current(!) platform implementation shipped with
;;; (some) current Linux distributions. I'll keep the code in a shape that can be 
;;; easily reverted to meet the standards, but try out some cheats that my own
;;; Fedora (21) distro allows at the moment. Regarding 1k compos, I've come to
;;; think that it just needs to work on one platform, one day, ever, under 1k.
;;; "Debug versions" can be included in the compo entry package additionally..
;;;
;;; We absolutely need the following contiguous areas (572+ bytes uncompressed):
;;;
;;; bytes
;;; -------------------------------------------------------------------------------
;;; 64   for the ELF header; must be the first in the output file.
;;; 168  (3x56) for program headers (interp, load, dynamic)
;;; 24+  (6x4) for the hash (dlopen(), dlsym()); can contain rubbish at end
;;; 144+ (9x16) for the dynamic section; actually 160 bytes, but can overlap 0s
;;; 72   (3x24) for dyn symbols (NULL, dlopen(), dlsym()); NULL contaminable?
;;; 48   (2x24) for relocation table (dlopen(), dlsym())
;;; 28+  for "/lib64/ld-linux-x86-64.so.2"; can(?) contain rubbish but \0 in end
;;; 24+	 for "libdl.so.2\0dlopen\0dlsym\0" ; could have paddings in-between
;;; -------------------------------------------------------------------------------
;;; 
;;; With SDL2 to beep and blink, we absolutely need the following:
;;; -------------------------------------------------------------------------------
;;; 208+ of library&function names for window, audio, and a shaded rectangle
;;; 216  (approx) shader source in GLSL
;;; 32+  (approx) synth sequence data
;;; -------------------------------------------------------------------------------
;;;
;;; Necessary, possibly not contiguous, areas that are required:
;;; -------------------------------------------------------------------------------
;;; 300  (approx) main code to initialize aud&vid and run the event loop
;;; 83   (approx) to link the libraries
;;; 23   (approx) for audio callback
;;; 206  (approx) for audio rendering
;;; -------------------------------------------------------------------------------
;;;
;;; Now, this puzzle allows some parts to overlap and some to be suitably changed.
;;; Aim of the game is simply to minimize the number bytes resulting from gzip
;;; compression of a valid overlay of the pieces. Simple? Quite many alternatives
;;; though.. Current zopfli seems to quite intelligently split the stream in two
;;; blocks where the ASCII part (first in file) and headers + binary
;;; code part (second in file) get treated separately. I suppose it is good to
;;; keep it this way..

	
eh_start:	
		db	0x7F, "ELF"
		db	2 			; 64 bits (EI_CLASS)
		db	1 			; 2's compl. little endian (EI_DATA)
		db	1			; version 1 'current' (EI_VERSION)
		db	0			; sysv (EI_OSABI)
		db	0			; usually 0 (EI_ABIVERSION)
  		db	0,0,0,0,0,0,0		; unused '14 (EI_PAD[0..6])
	;; 16 bytes so far - at file offset 0x10
	
		dw	2			; executable (e_type)
		dw	0x3e			; x86-64 (e_machine)
		dd	1			; version 1 (e_version)
		dq	_start			; start (e_entry)
	;; 32 bytes so far - at file offset 0x20
  		dq	prg_head-eh_start	; offset to prog hdrtab (e_phoff)
	
  		dq	0			; offset to sec hdrtab (e_shoff)
	;; 48 bytes so far - at file offset 0x30
		dd	0			; flags (e_flags)
		dw	e_ehsize_makebelieve	; this hdr size (e_ehsize) can cheat??
		dw	0x38			; prog hdr entry size (e_phentsize)
		dw	4			; num entries in prog hdrtab (e_phnum)
		dw	0			; sec hdr entry size (e_shentsize)
		dw	0			; num entries in sec hdrtab (e_shnum)
  		dw      0			; header entry with sec names (e_shstrndx)
	;; 64 bytes (total length of x86-64 elf header) - at file offset 0x40
	
	ZERO_PAD_UPTO(STRINGS_START)

;;; This is problematic to place.. compressibility of the header seems to vary quite
;;; a lot, depending on where this resides, and whether the length can be a good value.
;;; So far it looks OK here.. zopfli will nicely pack this along with other ASCII data. 
interp:
 		db	'/lib64/ld-linux-x86-64.so.2', 0 ; 28 bytes including \0 
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

;;; Names of libraries and functions. Will use the above funcs to link the rest.
dli_strs:
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
%ifdef USE_CURSOR_HIDING
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
  	db '#version 120',10
	db 'uniform vec4 u;'
	db 'vec4[]a=vec4[]('
	db       'vec4(0),'
	db       'vec4(0),'
	db       'vec4(0),'
	db       'vec4(0),'
	db       'vec4(1,0,0,1),'
	db       'vec4(0),'
 	db       'vec4(.2,1,.2,1),'
	db       'vec4(1),'
	db       'vec4(1,0,0,1),'
	db       'vec4(0),'
 	db       'vec4(.2,1,.2,1),'
	db       'vec4(1));'
	db 'void main(){'
    	db    'vec4 o=1-gl_FragCoord/u.tptt*2;'
 	db    'o.s/=u.p/u.t;'
	db    'o=vec4(length(o.st),atan(o.t,o.s),atan(o.t,o.s),atan(o.t,o.s));'	
	
	db    'o.s/=1+fract(u.s/6);'    ; zoom in with time
 	db    'o.p=smoothstep(0,.01+o.s/6,'
 	db                     'o.s*sin((o.t+u.s/6*o.s*sin(2*o.s))*int(1+u.s/6)))'
	db         '+smoothstep(0,.01+o.s/6,'
 	db                     'o.s*sin((o.t-u.s/6*o.s*sin(2*o.s))*int(1+u.s/6)));'
    	db    'gl_FragColor=fract(o.p)>0?vec4(0):'
 	db                                 'a[int(u.s/6+o.p)]/o.s'
    	db                                 '+2-2*sin(u.s/6/6)'
;;;      	db	  '+smoothstep(0,6,u.s-54)' ; fade-out costs some 7 bytes
	db	';'
	db '}'
	db 0			; Check spec: need newline?? no.. no need.

 	ZERO_PAD_UPTO(HEADERS_START)

;; Need to have a hash.. can tweak size and numbers
hash:
		dd	3			; nbucket = 1
		dd	3			; nchain whatever >2 (maybe?)
		dd	2			; start dynsym chain from ind 2
		dd	0, 0, 1

;; Array of Elf64_Sym structures:
dynsym:
		dd	0	; u32 st_name
		db	0	; uc st_info
		db	0	; uc st_other
		dw	0	; u16 st_shndx
 		dq	0	; u64 st_value
		dq	0	; u64 st_size
dlopen_sym equ 1
		dd	dlopen_name
		db	0x12			; STB_GLOBAL, STT_FUNC
		db	0
		dw	0
		dq	0
	;; Interp must end with \0 - not sure what happens if there is rubbish before
	;; .. does the loader take parameters? Anyway, the exe seems to work.. funny.
interp_size_makebelieve equ $ - interp ; can try to make the size aligned to 0x100
		dq	0
dlsym_sym equ 2
		dd	dlsym_name
		db	0x12			; STB_GLOBAL, STT_FUNC
		db	0
		dw	0
		dq	0
		dd	0
dynstr_size_makebelieve equ $ - dynstr ; make a nice size.
		dd	0
dynsym_count equ 3
dynsym_size equ $ - dynsym

	ZERO_PAD_UPTO(RELTEXT_START)

;; The relocation table. 
;; It is array of type Elf64_Rela .. {u64 adr, u64 info, u64 addend}
reltext:
		dq	dlopen_rel
		dq	((dlopen_sym << 32) + 1)  ; R_X86_64_64
		dq	0

		dq	dlsym_rel
		dq	((dlsym_sym << 32) + 1)   ; R_X86_64_64
		dq	0
	
reltext_size equ $ - reltext
	
		dq	0	; allow for tweaked sizes..

;;; ------------------------------------------------------------

	ZERO_PAD_UPTO(PROGHEADERS_START)
	
;; Next, of type Elf64_Phdr:
prg_head:
		dd	3			; PT_INTERP
		dd	7			; flags PF_R
		dq	interp - $$		; file offset
		dq	interp			; virtual address
		dq	0			; (phys addr.) unused
		dq	interp_size_makebelieve		; filesz
		dq	interp_size_makebelieve	        ; memsz
		dq	0			; align (used? how??)

		dd	1		; 1 = PT_LOAD (p_type)
		dd	7		; 4 = PF_R (p_flags) 7=RWX
		dq	0		; from file beg. (p_offset)
	        dq	$$		; to this addr (p_vaddr)
		dq	0     		; phys. addr. (p_paddr) (unused)
		dq	file_size_makebelieve ;filesize (p_filesz) 'can be > real?' 
 		dq	mem_size 	; mem size, must allow for data area
        	dq	0	  	; 'alignment' (p_align) (used? how?)
	
;;; There will be a "Bus error" around second page for memory access
;;; if cheated file size is too much greater than the actual size.
;;; Cheats have limits...

		dd	2			; PT_DYNAMIC
		dd	7			; flags PF_R | PF_W (..why w?)
		dq	dynamic - $$		; file offset
		dq	dynamic			; virtual address
		dq	0			; (phys addr.) unused
		dq	dynamic_size_makebelieve		; filesz
		dq	dynamic_size_makebelieve            	; memsz
		dq	0			; (used? how??)

	;; Then one more header look-alike, marked unused, to make number of hdrs 4.
		dd	0			; PT_NULL (== unused)
		dd	0
	;; pad up to 0x100 alignment to get simpler numbers as start of dynamic
	;; (the effect of these might depend on the other contents, especially the
	;; shader string.. can affect the block count chosen by zopfli in two ways
	;; and thus make many bytes of difference.. for very short shader seems OK:)
		dq	0,0,0,0,0,0,0,0,0,0


	;; In theory, could have some code like audio callback here? Maybe?

;; The .dynamic section. Indicates the presence and location of the
;; dynamic symbol section (and associated string table and hash table)
;; and the relocation section.

;; Dynamic section entries, of type Elf64_Dyn {u64, u64}

dynamic:
		dq	5,  dynstr		; DT_STRTAB
		dq	1,  libdl_name		; DT_NEEDED
		dq	7,  reltext		; DT_RELA
		dq	4,  hash		; DT_HASH
		dq	10, dynstr_size_makebelieve		; DT_STRSZ
		dq	6,  dynsym		; DT_SYMTAB
		dq	8,  reltext_size_makebelieve	; DT_RELASZ
		dq	9,  24			; DT_RELAENT = sizeof(Elf64_Rela)
		dq	11, 24			; DT_SYMENT = sizeof(Elf64_Sym)
	
;;; The required DT_NULL content could be part of next section (size modified with +16):
		dq	0,0			; DT_NULL 

dynamic_size equ $ - dynamic

	




;;; Macro to skip zeros in string. Compresses very well; no need for function call
;;; Could increase 32-bit register with shorter uncompressed code, but somehow
;;; doesn't compress as well as when using full 64-bit instructions for %1 and %2.
%macro MAC_skip_str_at_reg 2
%%skip_str_at_reg:
	inc	%2
	cmp	[%1], byte 0	; zero ends string
	jne	%%skip_str_at_reg
	inc	%2		; becomes one past end of string.
%endmacro
	
	ZERO_PAD_UPTO(SYNTH_CONST_START)
	
;;;  ------------------------------------------------------ synth constants
syn_constants:
syn_c0freq:
;;;  	dd	0.001034631	; 0x3a879c75; close to MIDI note 0 freq / sr * 2 * pi
	dd	0.016554097	; 0x3c879c75; close to MIDI note 0x30
syn_freqr:
;;; 	dd	1.0594630943592953  ; freq. ratio between notes
	dd	1.0594622	; 0x3f879c75; close to 1.0594630943592953
syn_half:
	dd	0.5 		; volume.. 0x3f000000 (0.5f) compresses well
syn_currentf:
	dd	0
syn_repeatlen:
	dd	SYN_REPEAT_LEN
syn_miniseq:
  	db	0x80,0x10,0xc0,0x17 ; Played 4bits at a time; zeros after last.
	
	ZERO_PAD_UPTO(CODE_START)

;;;  ------------------------------------------------------ main code start
;;;  Entry point
_start:
	DEBUG_iniprintf
	
init_everything:
	mov	ebp,_base1
	
;; ---------------------------------
;;;  Link DLLs.
;;; ---------------------------------
	
;;; This is absolutely the smallest solution I can think of (and I did try
;;; quite a few alternative approaches... different addressing modes with indices
;;; in registers, in stack, and in global memory; even tried to pack the strings
;;; manually... I just can't make this part any smaller than now). Reality check:
;;; This runtime linking code yields 41-45 bytes in the compressed exe, depending
;;; on how the rest of the binary forms.. The names of the libraries and functions
;;; required in one-rect GLSL renderer take up 155 bytes. So, the total overhead of
;;; linking everything is approximately 45+155 = 200 bytes, i.e., 20% of the quota
;;; in "1k intro" competitions. I just can't do any better than this, so far.


fill_dli_table:	

	mov	ebx,dli_strs
	lea	r14,[ADDR(rbp,_base1,fptrs)] ; r14 points to output array

dli_libloop:
	;;  Expect always at least one library (end condition checked only afterwards)
 	DEBUG_printline(rbx)
	
	GPR_MOV rdi,rbx
	GPR_LDB	rsi,RTLD_NOW
	call	[ADDR(rbp,_base1,dlopen_rel)]
	mov	[ADDR(rbp,_base1,handle)],eax

 	DEBUG_dlerror

 	MAC_skip_str_at_reg ebx,ebx
	
dli_funcloop:
	;; Expect always at least one function (end condition checked only afterwards)
 	DEBUG_printline(rbx)

	mov	edi,[ADDR(rbp,_base1,handle)]
	GPR_MOV	rsi,rbx
 	call	[ADDR(rbp,_base1,dlsym_rel)]
	mov	[r14],rax	; store fptr
	add	r14,8		; move to next storage location
	
	DEBUG_printaddress rax
 	DEBUG_dlerror
	
 	MAC_skip_str_at_reg ebx,ebx
	
	cmp	[ebx], byte 0	; zero ends funcs
	jne	dli_funcloop
	inc	ebx
	cmp	[ebx], byte 0	; zero ends input
	jne	dli_libloop
	
dli_over:



;;; Expect RBP at _base1
 	lea	r14,[ADDR(rbp,_base1,SDL_Init_p)] ; call "r14++" every time..

;;; ---------------------------------
;;; SDL2 Init: window, GL, audio
;;; ---------------------------------

init_sdl:
	GPR_LDB	rdi,(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER)
	call	[r14]
	add	r14,8
;;; 	call	[ADDR(rbx,_base2,SDL_Init_p)]
	
	GPR_LD0	edi
	mov	esi,SDL_WINDOWPOS_UNDEFINED
	mov	edx,esi
	mov	ecx,[ADDR(rbp,_base1,screen_w)]
	mov	r8d,[ADDR(rbp,_base1,screen_h)]
	GPR_LDB	r9,MY_WINDOW_SPEC
;;; 	GPR_LDB	r9,SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN
	call	[r14]
	add	r14,8
;;;	call	[ADDR(rbx,_base2,SDL_CreateWindow_p)]
	mov	[ADDR(rbp,_base1,sdl_pwindow)],rax

	mov	rdi,[ADDR(rbp,_base1,sdl_pwindow)] ; (same as in render)
	call	[r14]
	add	r14,8
;;;	call	[ADDR(rbx,_base2,SDL_GL_CreateContext_p)]
	;; (return value is GL context ID - unused, discard)

	lea	edi,[ADDR(rbp,_base1,desired_sdl_audio_spec)]
 	GPR_LD0	esi
	call	[r14]
	add	r14,8
;;;	call	[ADDR(rbx,_base2,SDL_OpenAudio_p)]
	
 	GPR_LD0	edi
	call	[r14]
	add	r14,8
;;;	call	[ADDR(rbx,_base2,SDL_PauseAudio_p)]

;;; ---------------------------------
;;; Compile shaders
;;; ---------------------------------

;;; Expect RBP at _base1 and RBX at _base2
compile_shaders:

;;; Funny... the packed image seems to become larger when I try to
;;; minimize the code in the unpacked original... "The stories may be
;;; true"... might be better off with standardizing instead of minimizing?
;;; TODO: Could try different ordering of calls here, or keeping
;;; identical parameters ready in a register or stacktop between calls
;;; Ok.. tried, with pain but no gain..
	
	call	[r14]
	add	r14,8
;;;	call	[ADDR(rbp,_base1,glCreateProgram_p)]
	mov	[ADDR(rbp,_base1,shader_pid)],eax

	mov	edi,GL_FRAGMENT_SHADER
	call	[r14]
	add	r14,8
;;;	call	[ADDR(rbp,_base1,glCreateShader_p)]
	mov	[ADDR(rbp,_base1,tmpvar)],eax
	
 	GPR_LDB	rsi,1		; Hmm.. should have a comment about the interface here...
	lea	edx,[ADDR(rbp,_base1,ptr_to_fshader)]
	GPR_LD0	ecx
	mov	edi,[ADDR(rbp,_base1,tmpvar)]
	call	[r14]
	add	r14,8
;;;	call	[ADDR(rbp,_base1,glShaderSource_p)]

	mov	edi,[ADDR(rbp,_base1,tmpvar)]
	call	[r14]
	add	r14,8
;;;	call	[ADDR(rbp,_base1,glCompileShader_p)]
	
	mov	edi,[ADDR(rbp,_base1,shader_pid)]
	mov	esi,[ADDR(rbp,_base1,tmpvar)]
	call	[r14]
	add	r14,8
;;;	call	[ADDR(rbp,_base1,glAttachShader_p)]

	
%ifdef USE_VERTEX_SHADER
	mov	rdi,GL_VERTEX_SHADER
	;; ... and so on... do the same source+compile+attach to vertex shader
%endif	

 	mov	edi,[ADDR(rbp,_base1,shader_pid)]
;;; Possible, if xchg used in other places but not really useful when tried:
;;; 	mov	esi,[ADDR(rbp,_base1,shader_pid)]
;;; 	xchg	rsi,rdi
	call	[r14]
	add	r14,8
;;;	call	[ADDR(rbp,_base1,glLinkProgram_p)]
	
;;; ------------------------------- end of INIT

	
loop:
	call	render_w_shaders
;;; Expect R14 to point to SDL_GL_SwapWindow_p .. tad smaller with just base+offs
;;; No... ultimately it compresses best when calling "r14++". Code for lea has little
;;; difference between repetitions..
 	lea	r14,[ADDR(rbp,_base1,SDL_GL_SwapWindow_p)] ; call "r14++" every time..
	mov	rdi,[ADDR(rbp,_base1,sdl_pwindow)]
;;;  	call	[ADDR(rbp,_base1,SDL_GL_SwapWindow_p)]
	call	[r14]
	add	r14,8
	
	lea 	edi,[ADDR(rbp,_base1,sdl_event_space)]
	call	[r14]
	add	r14,8
;;; 	call	[ADDR(rbp,_base1,SDL_PollEvent_p)]

;;; Stop when time is up:
;;; 	cmp	dword [ADDR(rbp,_base1,synth_frames_done)],(48000*60+3584) ;close to 48000*60
;;; Byte comparison is some bytes shorter, compressed:
 	cmp	[ADDR(rbp,_base1,synth_frames_done+2)], byte 0x2b ; 60 seconds plus a little
 	jg	event_loop_is_over


;;; Stop at keypress event:
%if 0
	;; With many attempts, I didn't figure out a shorter way to test:
	cmp	[ADDR(rbp,_base1,sdl_event_space)], word SDL_KEYDOWN
%else
	;; But then, of course this is shorter than the previous one:
 	lea	edi,[ADDR(rbp,_base1,sdl_event_space)]
 	mov	ah, (SDL_KEYDOWN >> 8) ; Looks weird.. see the comment below.
 	scasd
	;; The AH update works, if SDL_PollEvent returns 0 in rax when the event is
	;; copied to sdl_event_space. Using words from the SDL docs, should be
	;; that there are "no pending events" when keydown reaches us here.
	;; 
	;; Now then, if EAX is proper, then scasd can be used for comparison because
	;; the first field of SDL_WhateverEvent is exactly 'Uint32 type;'.
	;; Looks like the event type is not updated when polling without events.
	;; Can't check only one byte at [rdi+1] because we re-use the space
	;; for streaming floats to gfx, and it is easy to end up with a
	;; value with 0x****03** in there. But 0x300 as dword won't be out there so 
	;; scasd seems to be a robust test. Very few bits need to be stored in the
	;; deflate stream now, because we mostly repeat earlier code.
	;; 
	;; Yet another observation from SDL_events.h: keyboard events seem to be the
	;; only ones with higher byte 0x03 .. try to exploit this instead!? Can't..
	;; see above about sharing space with floats.
%endif

	jne	loop
event_loop_is_over:
	
	call	[r14]
;;;  	call	[ADDR(rbp,_base1,SDL_Quit_p)]
%if 0
 	add	r14,8		; Unnecessary - but balances the zopfli blocksplit.
 	add	r14,8		; Unnecessary - but balances the zopfli blocksplit.
%endif
	
	
;;; Exit with whatever code.. doesn't matter, really..
%if 1
	GPR_LDB	rax,1			; 1 == exit system call
times 1	int	0x80			; exit(whatever_in_BL) - repeat to pad.
%else
;;; TODO: Hmm, the show is basically over already, so could I just crash it?
;;; Maybe not... there will be a rather inpolite core dump. Well, illegal
;;; instruction of zero opcodes will yield a tempting 4 bytes shorter code than proper
;;; exit, so I'll leave it here as something to try in a desperate occasion.
	db	0,0
	db	0,0,0,0,0,0
%endif

	
;;; ------------------------------------------- start render with shaded rect
;;; Expect R14 to point to glUseProgram_p
render_w_shaders:
 	lea	r14,[ADDR(rbp-8,_base1,glUseProgram_p)] ; call "r14++" every time..
	add	r14,8
	
	mov	edi,[ADDR(rbp,_base1,shader_pid)]
	call	[r14]
	add	r14,8
;;; 	call	[ADDR(rbp,_base1,glUseProgram_p)]
	
	;; Use rsi, like in the newer synth codes.. same opcodes for storing floats
	lea	edi,[ADDR(rbp,_base1,some_floats)]
	
	;; Get float seconds from current synth time to some_floats[0]:
  	fild	dword [ADDR(rbp,_base1,synth_frames_done)]
;;;  	fild	dword [ABS synth_frames_done]
;;;   	fild	dword [synth_frames_done]
 	fidiv	dword [ADDR(rbp,_base1,synth_sr_i)]
;;; 	fidiv	dword [ABS synth_sr_i]
	fstp	dword [rdi]
	;; scasd compresses some bits better than 'add edi,4' ...
 	scasd

	;; Get screen size to some_floats[1] and some_floats[2]:
	fild	dword [ADDR(rbp,_base1,screen_w)]
	fstp	dword [rdi]
 	scasd

	fild	dword [ADDR(rbp,_base1,screen_h)]
	fstp	dword [rdi]
	;;; scasd  -- unnecessary, and repetition doesn't help. here
	
	
 	lea	esi,[ADDR(rbp,_base1,s_string)]
	mov	edi,[ADDR(rbp,_base1,shader_pid)]
	call	[r14]
	add	r14,8
;;;	call	[ADDR(rbp,_base1,glGetUniformLocation_p)]

	GPR_MOV	rdi,rax
	GPR_LDB	rsi,1 		;number of state vectors to visualize
	lea	edx,[ADDR(rbp,_base1,some_floats)]	;state to visualize, as vec4[]
	call	[r14]
	add	r14,8
;;;	call	[ADDR(rbp,_base1,glUniform4fv_p)]

 	mov	edi,(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT)
;;; 	GPR_LDB	rdi,(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT)
	call	[r14]
	add	r14,8
;;;	call	[ADDR(rbp,_base1,glClear_p)]


%if 0
	;; This looks "less red" in gzthermal, but total takes some bits more.
	push	-1
	push	1
	push	1
	push	-1
	pop	rsi
	pop	rdx
	pop	rcx
	pop	rdi
%else
	;; So far, this version seems unbeatable:
 	GPR_LDB	rsi,-1
  	GPR_MOV rdi,rsi
 	GPR_LDB	rdx,1
  	GPR_MOV rcx,rdx
%endif	
	call	[r14]
;;; 	call	[ADDR(rbp,_base1,glRects_p)]
;;; Games are over.. rewind r14 to first function needed in render function.
;;;   	sub	r14,(glRects_p - glUseProgram_p) ; move to earlier func.
	
;;; ------------------------------------------- end render with shaded rect
	ret

	
;;; -------------------------------------------- synth callback code
;;; We get userdata in rdi, stream address in rsi, buffer length in rdx
audio_callb_sdl:
	push	rbp		; Use this here as ptr
	push	rbx		; Use this here as ptr
	;; (no need to align stack; we call no-one here; use red zone if need be)

;;;  	lea	ebx, [synth_frames_done]
 	mov	ebp, synth_bss_base
	mov	ebx, syn_constants

aud_buf_loop:
	;; Restart envelope when at 0:
;;; 	cmp	dword [ADDR(rbp,synth_bss_base,synth_env_state)],SYN_REPEAT_LEN
	mov	eax,dword [ADDR(rbp,synth_bss_base,synth_env_state)]
	cmp	eax,dword [ADDR(rbx,syn_constants,syn_repeatlen)]
	jne	no_restart
	xor	eax,eax
no_restart:

	cmp	eax,0
	jne	no_recompute
	
	;; Also update note. Load RCX with note number.
	mov	cl, byte [ADDR(rbx,syn_constants,syn_miniseq)]
;;;  	add	ecx,0x30
;;; 	inc	cl
 	shr	dword [ADDR(rbx,syn_constants,syn_miniseq)], 4	

	fld	dword [ADDR(rbx,syn_constants,syn_c0freq)]
	and	ecx,0x0f
	jz	syn_loop_over
syn_loop:
	fmul	dword [ADDR(rbx,syn_constants,syn_freqr)]
	loop	syn_loop
syn_loop_over:
 	fstp	dword[ADDR(rbx,syn_constants,syn_currentf)]
	
no_recompute:

	mov	dword [ADDR(rbp,synth_bss_base,synth_env_state)],eax
	fild	dword [ADDR(rbp,synth_bss_base,synth_env_state)] ; (ienv)
	fidiv	dword [ADDR(rbx,syn_constants,syn_repeatlen)]    ; (rise)
	fld1							 ; (rise 1)
	fsubp	st1						 ; (fall)
	
	;; Intend to play sin(2pi*frequency*framecount/srate)
	fld	dword [ADDR(rbx,syn_constants,syn_currentf)] ;(fall note)
	fimul	dword [ADDR(rbp,synth_bss_base,synth_env_state)] ; *iphase
	
	fld	st0					; (fall note note)
	fadd	st0
	fsin
	fmul	st2
	fmul	st2
	fadd	st0
	fadd	st0
	faddp	st1		; (fall note+sin(2*note))

	fsin

	fmul	dword [ADDR(rbx,syn_constants,syn_half)]
	fmulp	st1		; (audio)

	;; ---- Delay thingy.
	;;  stream out the whole thing:
	mov	ecx, dword [ADDR(rbp,synth_bss_base,synth_frames_done)]
	fld	dword [ADDR(rbp,synth_bss_base,syn_delay)+rcx*4]
 	fmul	dword [ADDR(rbx,syn_constants,syn_half)]
	faddp
	add	ecx, SYN_DLY_LEN
	fst	dword [ADDR(rbp,synth_bss_base,syn_delay)+rcx*4]
	;; ---- Delay thingy ends.
	
	fstp	dword [rsi]	; ()
	add	esi,4

	add	dword [ADDR(rbp,synth_bss_base,synth_frames_done)],1
	add	dword [ADDR(rbp,synth_bss_base,synth_env_state)],1
	
	sub	edx,4
	jnz	aud_buf_loop

	pop	rbx
	pop	rbp
	ret

;;; 	ZERO_PAD_UPTO(SYNTH_PATTERNS_START)
	
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
	dw	2048		; samples as Uint16
	dw	0		; padding as Uint16 (compiler compat.)
	dd	0		; size as Uint32 (calculated)
	dq	audio_callb_sdl	; callback as SDL_AudioCallback
%ifdef SYN_USERDATA_INIT
	dq	SYN_USERDATA_INIT
%else
  	dq	0		; userdata as void* (could actually omit totally)
%endif
	

ptr_to_fshader:	dq	minimized_fragment_shader_str
s_string:	db	'u',0

file_size equ $ - $$

;;
;; Beginning of the program's bss section.
;; 

ABSOLUTE $

ALIGNB 0x8

;;;  Pointers to functions after relocation ():
;;;  The defines require fptrs to be in register "r"

;;; resq	1	; Little tweaks like this can affect compression by +/- 2 bytes
handle:
tmpvar:			resq	1	; hmm.. use this as a local temp
	

_base1:
sdl_pwindow:			; Re-use this space after last call to dlopen()
dlopen_rel:
			resq	1
;;; 			resq	1 ; This is padding to get a different offset in header
shader_pid:			; Re-use this too. (dword in qword space)
			resb	2 ; Match locations for compressible representations
dlsym_rel:
			resq	1

fptrs:
;;; ------------ sequentially ordered funcs (evloop) start here
SDL_GL_SwapWindow_p:	resq	1
SDL_PollEvent_p:	resq	1
;;; ------------ sequentially ordered funcs (evloop) end here
SDL_Quit_p:		resq	1

;;; ------------ sequentially ordered init starts here

;;; The following 7x8 bytes can be re-used after init:
;;; sdl_event_space (56 bytes): used when polling SDL event, once in a loop.
;;; some_floats (max 14x4 = 56 bytes) are recomputed for every visualization frame.
;;; The memory overlaps, which is not a problem, because structures are filled before
;;; use every time.

sdl_event_space:		; used at the end of ev loop
some_floats:			; used while rendering, once per ev loop round
SDL_Init_p:		resq	1
%ifdef USE_DESKTOP_DISPLAY_QUERY
SDL_GetDesktopDisplayMode_p:	resq	1
%endif
SDL_CreateWindow_p:	resq	1
SDL_GL_CreateContext_p:	resq	1
SDL_OpenAudio_p:	resq	1
SDL_PauseAudio_p:	resq	1
%ifdef USE_CURSOR_HIDING
SDL_ShowCursor_p:	resq	1
%endif
glCreateProgram_p:	resq	1
glCreateShader_p:	resq	1
glShaderSource_p:	resq	1
glCompileShader_p:	resq	1
glAttachShader_p:	resq	1
glLinkProgram_p:	resq	1

;;; ------------ sequentially ordered init up to here

;;; ------------ sequentially ordered funcs (render) start here
glUseProgram_p:		resq	1
glGetUniformLocation_p:	resq	1
glUniform1fv_p:		resq	1
glClear_p:		resq	1
glRects_p:		resq	1
;;; ------------ sequentially ordered funcs (render) end

%ifdef USE_GL_TEXTURES	
glGenTextures_p:	resq	1
glBindTexture_p:	resq	1
glTexParameteri_p:	resq	1
glTexImage2D_p:		resq	1
%endif

%ifdef USE_GL_MATRICES	
glMatrixMode_p:		resq	1
glLoadIdentity_p:	resq	1
%endif

%ifdef USE_FUNCTIONS_FROM_GLU	
gluNewQuadric_p:	resq	1
gluSphere_p:		resq	1
gluPerspective_p:	resq	1
%endif

%ifdef USE_FUNCTIONS_FROM_LIBM
powf_p:			resq	1
%endif
	

;;; BSS data for synth:
synth_bss_base:			; <-- can vary location of base.
synth_env_state:
	resq	1
synth_frames_done:
	resq	1
syn_dly_read:
	resq	1
syn_delay_bottom:
	resd	SYN_DLY_LEN
syn_delay:
	resd	(48000*60+3584)	; close to 48000*60


;;;  Outsiders.. for debug only, basically... no need for nearby base
printf_p:		resq	1
dlerror_rel:		resq	1

;;; First, a power-of-two alignment, and then any number of zeros to make a nicely
;;; compressible value for the program size:
ALIGNB	MEM_SIZE_ALIGN_BYTES
TIMES	MEM_SIZE_ADJUSTMENT_BYTES	resb	1

mem_size equ $ - $$
