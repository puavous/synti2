# This is a makefile template for preview, test, and final version of
# a stand-alone synth player exe and all other targets necessary for
# real-time compose-mode and frame dumps. This should be exported to a
# folder along with release-specific (partly generated) source codes.
.phony: all visual preview test final

# The HCFLAGS (as in "hardcore") are for making a very small
# executable. Could be tuned further?
HCFLAGS = -Os -Isrc \
	-DSYNTH_PLAYBACK_SDL \
	-DULTRASMALL \
	-nostdlib -nostartfiles -nodefaultlibs \
	-fno-exceptions \
	-fno-asynchronous-unwind-tables \
	-Wl,--hash-style=sysv \
	-Wl,--build-id=none \
	-mfpmath=387 -ffast-math \
	-Wall -Wextra -pedantic

HCLINKF = --hash-style=sysv --build-id=none


# With these, the dependency of libm could be lifted (just a few bytes
# gained, though): -mfpmath=387 -funsafe-math-optimizations
#
# -ffast-math implies -funsafe-math-optimizations.. compatibility of
# this and other math optimizations?

# Normal CFLAGS are for real-time compose-mode:
CFLAGS = -O3 -ffast-math -Wall -Wextra -pedantic -Isrc

## For normal 64-bit build:
ARCHFLAGS = `sdl-config --cflags`
ARCHLIBS = `sdl-config --libs` -lGL -lGLEW -lm -ldl
HCLIBS = -ldl

# `sdl-config --libs` polite but gives -lpthread which is not needed.
# I wonder why I used to have -lGLEW ... not using it nowadays...
# Also, was -lm necessary for some reason? sqrt? sin?
#ARCHLIBS = `sdl-config --libs` -lGL -lGLEW -lm

# For linking against 32-bit libraries (ca 5% smaller packed exe, but
# target machine needs separate installation of the 32-bit libs..):

#ARCHFLAGS = -m32 -DNO_I64 `sdl-config --cflags`
#ARCHLIBS = `sdl-config --libs` -lm /usr/lib/libGL.so.1
#HCLIBS = -lSDL -lm /usr/lib/libGL.so.1

# Looks as if it is best to strip first with strip and then sstrip..

ARCHSTRIP    = strip
ARCHSTRIPOPT = -s -R .comment  -R .gnu.version \
		-R .note.gnu.build-id \
		-R .eh_frame_hdr -R .eh_frame \
		-N _start

#(There are also the unnecessary push and pop instructions in function
# pro&epilogue which could be taken away with a binary edit, and not
# in any other way..?)

# Then, sstrip is used to further reduce executable size:
SSTRIP = sstrip -z

# Command for shader minification.
# Must output a file called shader.tmp
SHADER_TMP_CMD = mono ~/files/hacking/shader_minifier/shader_minifier.exe --format none \
		--preserve-externals -o shader.tmp

# Command for creating gzip-compatible tmp.gz from tmp.file:
#GZIP_TMP_CMD = gzip -9 tmp
GZIP_TMP_CMD = 7za a -tgzip -mx=9 tmp.gz tmp
#GZIP_TMP_CMD = zopfli --i25 tmp
#GZIP_TMP_CMD = zopfli --i1000 tmp

# These are required for the compilation (todo: better deps):
TINYHEADERS = src/synti2_archdep.h src/synti2_limits.h src/synti2_cap_custom.h  src/synti2_guts.h  src/synti2.h  src/synti2_misss.h
VISHEADERS = src/synti2_archdep.h src/synti2_cap_full.h src/synti2_guts.h src/synti2.h src/synti2_misss.h


# Possibly override any of the above:
include current.makefile

BASE_OBJS=startup64.o glfuncs.o shaders.o general_main.o
PLAY_OBJS=patchdata.o songdata.o
JACK_OBJS=synti2_jack.o synti2_midi.o

# Target objects in different output folders:
VIS_OBJS=$(addprefix obj_vis/,$(BASE_OBJS)) \
	 $(addprefix obj_vis/,$(JACK_OBJS))
TINY_OBJS=$(addprefix obj_tiny/,$(BASE_OBJS) $(PLAY_OBJS))
WRITER_OBJS = obj_writ/general_main.o \
		obj_writ/startup64.o \
		obj_tiny/glfuncs.o \
		obj_tiny/shaders.o \
		obj_tiny/patchdata.o \
		obj_tiny/songdata.o


#directories for object files
obj_vis:
	mkdir obj_vis
obj_tiny:
	mkdir obj_tiny
obj_writ:
	mkdir obj_writ

# Building differently for different outputs
obj_vis/%.o: src/%.c obj_vis
	$(CC) $(CUSTOM_FLAGS) $(CFLAGS) \
		-DCOMPOSE_MODE -DJACK_MIDI \
		-DSYNTH_COMPOSE_JACK \
		-c -o $@ $<

obj_tiny/%.o: src/%.c obj_tiny $(TINYHEADERS)
	$(CC) $(CUSTOM_FLAGS) $(CUSTOM_HCFLAGS) $(HCFLAGS) -c -o $@ $<

obj_writ/%.o: src/%.c obj_writ $(TINYHEADERS)
	$(CC) $(CUSTOM_FLAGS) $(CFLAGS) \
		-DDUMP_FRAMES_AND_SNDFILE \
		-c -o $@ $<


bin/jackvsynti2: $(VIS_OBJS)
	$(CC) -o $@ $^ `pkg-config --cflags --libs jack` \
		$(ARCHLIBS) -lGLU -lGLEW

writ2: $(WRITER_OBJS)
	$(CC) -o $@ $^ \
		 -ldl -lSDL -lGL -lm -lsndfile 


# Just for looking at the machine code being produced by gcc
#synti2.asm.annotated: synti2.c
#	$(CC) $(HCFLAGS) -S -fverbose-asm -g synti2.c
#	as -alhnd synti2.s > $@


#	shaders to c source, using the shader_minifier tool:
src/shaders.c: src/vertex.vert src/fragment.frag
	echo '/*Shaders, by the messiest makefile ever..*/' > $@
	echo '#include "GL/gl.h"' >> $@
	echo -n 'const GLchar * const vs[]={' >> $@
	$(SHADER_TMP_CMD) src/vertex.vert
	sed -e 's/.*/"&\\n"/'< shader.tmp >> $@
	echo '};' >> $@

	echo -n 'const GLchar * const fs[]={' >> $@
	$(SHADER_TMP_CMD) src/fragment.frag
	sed -e 's/.*/"&\\n"/' shader.tmp >> $@
	echo '};' >> $@
	rm shader.tmp

TOOL_CMD=../bin/synti2gui

src/synti2_cap_custom.h: $(wildcard src/*.s2bank)
	$(TOOL_CMD) --write-caps $< > $@

src/patchdata.c: $(wildcard src/*.s2bank)
	$(TOOL_CMD) --write-patches $< > $@

src/songdata.c: $(wildcard src/*.mid) $(wildcard src/*.s2bank)
	$(TOOL_CMD) --write-song $(wildcard src/*.mid) $(wildcard src/*.s2bank) > $@


tiny4: $(TINY_OBJS)

	$(LD) -i -o therest.o $^
	$(LD) \
		-o $@.unstripped.payload \
		therest.o \
		--hash-style=sysv \
		--build-id=none \
		-z noexecstack \
		--dynamic-linker=/lib64/ld-linux-x86-64.so.2 \
		-Ttinyexe.ld \
		$(HCLIBS)

	cp $@.unstripped.payload $@.payload 

# get rid of symbol versioning in dynamic segment:
# The 432 may be fragile! should have a tool that can
# actually seek the location of 0x6ffffffe and friends.
	dd conv=nocreat,notrunc bs=1 count=48 seek=432 of=$@.payload < /dev/zero

# Then, there is no need for the respective sections anymore:
	strip -R ".gnu.version" -R ".gnu.version_r" -R ".comment" $@.payload

# Then, we can strip all we want...
	$(ARCHSTRIP) $(ARCHSTRIPOPT) $@.payload
	$(SSTRIP) $@.payload

# Pack through tmp->tmp.gz to allow different packers to be used
	cp $@.payload tmp
	$(GZIP_TMP_CMD)
	cat src/selfextr.stub tmp.gz > $@
	rm tmp.gz

	chmod ugo+x $@

	@echo End result:
	@ls -lt $@


all: bin/jackvsynti2 tiny4 writ2
