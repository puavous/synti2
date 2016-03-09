;;; Picked from the interface versions I happen to have installed currently
;;; Out of luck, if these don't work somewhere :)

%ifndef DEFINITIONS_FROM_LIBS_INCLUDED
%define DEFINITIONS_FROM_LIBS_INCLUDED
	
	
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
%define SDL_DEFINE_PIXELFORMAT(type, order, layout, bits, bytes) \
	((1 << 28) | ((type) << 24) | ((order) << 20) | ((layout) << 16) | \
	((bits) << 8) | ((bytes) << 0))
;;; Ach.. just use a mini C program to come up with format u want
%define SDL_PIXELFORMAT_ARGB8888 0x16362004
%define SDL_TEXTUREACCESS_STREAMING 1
%define SDL_TEXTUREACCESS_STATIC 0	

;;; From dlfcn.h
%define RTLD_NOW       2
%define RTLD_LAZY      1

%endif
	
