# This is a makefile template for preview, test, and final version of
# a stand-alone synth player exe and all other targets necessary for
# real-time compose-mode and frame dumps. This should be exported to a
# folder along with release-specific (partly generated) source codes.
.phony: all visual preview test final

# The HCFLAGS (as in "hardcore") are for making a very small
# executable. Could be tuned further?
HCFLAGS = -Os -fwhole-program -Isrc \
	-mfpmath=387 -ffast-math \
	-Wl,--hash-style=sysv,-znorelro \
	-Wall -Wextra -pedantic


# With these, the dependency of libm could be lifted (just a few bytes
# gained, though): -mfpmath=387 -funsafe-math-optimizations
#
# -ffast-math implies -funsafe-math-optimizations.. compatibility of
# this and other math optimizations?

# Normal CFLAGS are for real-time compose-mode:
CFLAGS = -O3 -ffast-math -Wall -Wextra -pedantic -Isrc

## For normal 64-bit build:
ARCHFLAGS = -rdynamic `sdl-config --cflags`
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
# With strip, I'm taking away all I can so that the executable still
# works...  This must not be taken away: .gnu.hash (Seems to work on
# the build system, but not others.. need to study the elf business a
# bit. TODO: See if the --hash-style= option affects this.. in any case
# we get a smaller exe with sysv hash)

ARCHSTRIP    = strip
ARCHSTRIPOPT = -s -R .comment  -R .gnu.version \
		-R .note.gnu.build-id \
		-R .eh_frame_hdr -R .eh_frame \
		-N _start

# Hmm... _start is not necessary after linking?, so why does it remain
# in the exe? Can it be removed without hex/binary editing? (There are
# also the unnecessary push and pop instructions in function
# pro&epilogue which could be taken away with a binary edit, and not
# in any other way..?)

# Then, sstrip is used to further reduce executable size:
SSTRIP = sstrip -z

# Neither strip nor sstrip take away the symbols __bss_start, _edata, _end
# which seem to be not so necessary to have the exe working...
# Solution to this would be to use a custom linker script?? Need to learn this
# stuff a little bit..

# Command for shader minification.
# Must output a file called shader.tmp
SHADER_TMP_CMD = mono ~/files/hacking/shader_minifier/shader_minifier.exe --format none \
		--preserve-externals -o shader.tmp

# Command for creating gzip-compatible tmp.gz from tmp.file:
#GZIP_TMP_CMD = gzip -9 tmp
GZIP_TMP_CMD = 7za a -tgzip -mx=9 tmp.gz tmp
#GZIP_TMP_CMD = zopfli --i25 tmp
#GZIP_TMP_CMD = zopfli --i1000 tmp

# These are required for the compilation:
MAINFILE = src/general_main.c
JACKSOURCES = src/synti2_jack.c src/synti2_midi.c
TINYHEADERS = src/synti2_archdep.h src/synti2_limits.h src/synti2_cap_custom.h  src/synti2_guts.h  src/synti2.h  src/synti2_misss.h
VISHEADERS = src/synti2_archdep.h src/synti2_cap_full.h src/synti2_guts.h src/synti2.h src/synti2_misss.h
TINYHACKS = src/shaders.h src/render.c src/glfuncs.c src/patchdata.c src/songdata.c 
VISHACKS =  src/shaders.h src/render.c src/glfuncs.c


# Possibly override any of the above:
include current.makefile


# Just for looking at the machine code being produced by gcc
#synti2.asm.annotated: synti2.c
#	$(CC) $(HCFLAGS) -S -fverbose-asm -g synti2.c
#	as -alhnd synti2.s > $@


#	shaders to c source, using the shader_minifier tool:
src/shaders.h: src/vertex.vert src/fragment.frag
	echo '/*Shaders, by the messiest makefile ever..*/' > src/shaders.h
	echo -n 'static const GLchar *vs=' >> src/shaders.h
	$(SHADER_TMP_CMD) src/vertex.vert
	sed -e 's/.*/"&\\n"/'< shader.tmp >> src/shaders.h
	echo ';' >> src/shaders.h

	echo -n 'static const GLchar *fs=' >> src/shaders.h
	$(SHADER_TMP_CMD) src/fragment.frag
	sed -e 's/.*/"&\\n"/' shader.tmp >> src/shaders.h
	echo ';' >> src/shaders.h
	rm shader.tmp

TOOL_CMD=../bin/synti2gui

src/synti2_cap_custom.h: $(wildcard src/*.s2bank)
	$(TOOL_CMD) --write-caps $< > $@

src/patchdata.c: $(wildcard src/*.s2bank)
	$(TOOL_CMD) --write-patches $< > $@

src/songdata.c: $(wildcard src/*.mid) $(wildcard src/*.s2bank)
	$(TOOL_CMD) --write-song $(wildcard src/*.mid) $(wildcard src/*.s2bank) > $@


tiny4: $(TINYSOURCES) $(TINYHEADERS) $(TINYHACKS)
	$(CC) $(HCFLAGS) $(ARCHFLAGS) \
		-o $@.unstripped.payload \
		-DSYNTH_PLAYBACK_SDL \
		-DULTRASMALL \
		-nostdlib -nostartfiles -nodefaultlibs \
		-fwhole-program -flto \
		$(CUSTOM_FLAGS) $(CUSTOM_HCFLAGS)\
		$(MAINFILE) \
		$(HCLIBS)

	cp $@.unstripped.payload $@.payload 

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


bin/jackvsynti2: $(MAINFILE) $(JACKSOURCES) $(VISHEADERS) $(VISHACKS)
	$(CC) $(CFLAGS) $(ARCHFLAGS) \
		-o $@ \
		-DCOMPOSE_MODE \
                -DJACK_MIDI \
		-DSYNTH_COMPOSE_JACK \
		$(MAINFILE) $(filter %.c, $(JACKSOURCES)) \
		`pkg-config --cflags --libs jack` \
		$(ARCHLIBS) -lGLU -lGLEW

writ2: $(TINYSOURCES) $(TINYHEADERS) $(TINYHACKS)
	$(CC) \
		-o $@ \
		-DDUMP_FRAMES_AND_SNDFILE \
		$(CUSTOM_FLAGS) \
		$(MAINFILE) \
		$(HCLIBS) -lSDL -lGL -lm -lsndfile -lc 



writX: $(MAINFILE) $(TINYSOURCES) $(TINYHEADERS) $(TINYHACKS)
	$(CC) $(CFLAGS) $(ARCHFLAGS) \
		-o $@ \
		-DDUMP_FRAMES_AND_SNDFILE \
		-DPLAYBACK_DURATION=10.f \
		$(MAINFILE) $(filter %.c, $(WRITERSOURCES)) \
		-lsndfile

all: bin/jackvsynti2 tiny4 writ2
