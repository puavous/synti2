;;; The first try of a song. Too complicated, alas.

%macro SYNTH_PATTERN_DATA 0
syn_sequence:

	db	0x03, 0x00	; instrument info; length & mix (latter unused)
	db	0, 37		; low-ish note for bass
%macro PAT1	0
	db	2, 37
	db	2, 37
	db	2, 37
	db	2, 37
	db	2, 37
	db	2, 37
	db	2, 37
	db	2, 35
	db	2, 35
	db	2, 35
	db	2, 35
	db	2, 32
	db	2, 32
	db	2, 32
	db	2, 32
	db	2, 37
%endmacro
	
	PAT1
	PAT1
	PAT1
	PAT1
	db	SYN_END_SEQ
	
	
	db	0x18, 0x00
	;; ---
	db	0, 49
	db	0x10, 53
	db	0x10, 56
	db	0x10, 59
	db	0x10, 49
	db	0x10, 53
	db	0x10, 56
	db	0x10, 59
	
	db	SYN_END_SEQ

	db	0x08, 0x00
	;; ---
	
	db	2, 73
	db	2, 71
	db	2, 66
	db	1, 68
	db	1, 61
	db	1, 63
	db	1, 66
	db	1, 68
	
	db	6, 73
	db	2, 71
	db	2, 66
	db	1, 68
	db	1, 61
	db	1, 63
	db	1, 66
	db	1, 68
	
	db	6, 73
	db	2, 71
	db	2, 66
	db	1, 68
	db	1, 61
	db	1, 63
	db	1, 66
	db	1, 68
	
	
	db	6, 73
	db	2, 71
	db	2, 66
	db	1, 68
	db	1, 61
	db	1, 63
	db	1, 66
	db	1, 68
	
	
	
	
	
	db	SYN_END_SEQ
	
%if 0
	
	db	0x03, 0x02
	db	0, 61
%macro PAT3	0
	db	4, 61
	db	2, 61
	db	4, 61
	db	2, 61
	db	4, 61
	db	2, 61
	db	4, 61
	db	2, 61
	db	4, 61
	db	2, 61
	db	4, 61
%endmacro

	PAT3
	PAT3
	PAT3
	PAT3
	PAT3
%endif

	db	SYN_END_SEQ
	db	SYN_END_SEQ	; end song

%endmacro
	
