# This is a makefile template for preview, test, and final version of
# a stand-alone synth player exe and all other targets necessary for
# real-time compose-mode and frame dumps. This should be exported to a
# folder along with release-specific (partly generated) source codes.
.phony: all visual preview test final

# The HCFLAGS (as in "hardcore") are for making a very small
# executable. Could be tuned further?
HCFLAGS = -Os -mfpmath=387 -funsafe-math-optimizations -fwhole-program \
	-Wall -Wextra -pedantic -Isrc

# With these, the dependency of libm could be lifted (just a few bytes
# gained, though): -mfpmath=387 -funsafe-math-optimizations

# Normal CFLAGS are for real-time compose-mode:
CFLAGS = -O3 -ffast-math -Wall -Wextra -pedantic -Isrc

## For normal 64-bit build:
ARCHFLAGS = `sdl-config --cflags`
ARCHLIBS = `sdl-config --libs` -lGL -lGLEW -lm

# For linking against 32-bit libraries (ca 5% smaller packed exe, but
# target machine needs separate installation of the 32-bit libs..):

#ARCHFLAGS = -m32 -DNO_I64 `sdl-config --cflags`
#ARCHLIBS = `sdl-config --libs` -lm /usr/lib/libGL.so.1

# Looks as if it is best to strip first with strip and then sstrip..
# With strip, I'm taking away all I can so that the executable still
# works...  This must not be taken away: .gnu.hash (Seems to work on
# the build system, but not others.. need to study the elf business a
# bit. TODO: See if the --hash-style= option affects this)

ARCHSTRIP    = strip
ARCHSTRIPOPT = -s -R .comment  -R .gnu.version \
		-R .note.gnu.build-id \
		-R .eh_frame_hdr -R .eh_frame 

# Then, sstrip is used to further reduce executable size:
SSTRIP = sstrip

# Command for shader minification. Must support "-o OUTFNAME"
SHADER_CMD = mono ~/files/hacking/shader_minifier/shader_minifier.exe --format none \
		--preserve-externals 

# Command for creating gzip-compatible tmp.gz from tmp.file:
GZIP_TMP_CMD = 7za a -tgzip -mx=9 tmp.gz tmp


# These are required for the compilation:
MAINFILE = src/general_main.c
JACKSOURCES = src/synti2_jack.c src/synti2_midi.c
TINYHEADERS = src/synti2_archdep.h src/synti2_limits.h src/synti2_cap_custom.h  src/synti2_guts.h  src/synti2.h  src/synti2_misss.h
VISHEADERS = src/synti2_archdep.h src/synti2_cap_full.h src/synti2_guts.h src/synti2.h src/synti2_misss.h
TINYHACKS = src/shaders.c src/render.c src/glfuncs.c src/patchdata.c src/songdata.c 
VISHACKS =  src/shaders.c src/render.c src/glfuncs.c



# Just for looking at the machine code being produced by gcc
#synti2.asm.annotated: synti2.c
#	$(CC) $(HCFLAGS) -S -fverbose-asm -g synti2.c
#	as -alhnd synti2.s > $@



# ------------------- Shaders.
#	shaders to c source using sed..:
#	nothingFIXME:
#		echo $(filter %vertex.vert, $^)
#		echo '/*Shaders, by the messiest makefile ever..*/' > hackpack3/shaders.c
#		echo 'const GLchar *vs=""' >> hackpack3/shaders.c
#	
#		sed 's/^ *//g; s/  */ /g; s/ *\([=+\*,<>;//]\) */\1/g; s/\(.*\)\/\/.*$$/\1/g; s/\(.*\)/"\1"/g;' \
#			< $(filter %vertex.vert, $^) >> hackpack3/shaders.c
#		echo '"";' >> hackpack3/shaders.c
#		echo 'const GLchar *fs=""' >> hackpack3/shaders.c
#		sed 's/^ *//g; s/  */ /g; s/ *\([=+\*,<>;//]\) */\1/g; s/\(.*\)\/\/.*$$/\1/g; s/\(.*\)/"\1"/g;' \
#			< $(filter %fragment.frag, $^) >> hackpack3/shaders.c
#		echo '"";' >> hackpack3/shaders.c

#	shaders to c source, using the shader_minifier tool:
src/shaders.c: src/vertex.vert src/fragment.frag
	$(SHADER_CMD) -o vertshader.tmp src/vertex.vert
	$(SHADER_CMD) -o fragshader.tmp src/fragment.frag
	echo '/*Shaders, by the messiest makefile ever..*/' > src/shaders.c
	echo -n 'const GLchar *vs="' >> src/shaders.c
	cat vertshader.tmp >> src/shaders.c
	echo '";' >> src/shaders.c
	echo -n 'const GLchar *fs="' >> src/shaders.c
	cat fragshader.tmp >> src/shaders.c
	echo '";' >> src/shaders.c
	rm vertshader.tmp fragshader.tmp

TOOL_CMD=../bin/synti2gui

src/synti2_cap_custom.h: $(wildcard src/*.s2bank)
	$(TOOL_CMD) --write-caps $< > $@

src/patchdata.c: $(wildcard src/*.s2bank)
	$(TOOL_CMD) --write-patches $< > $@

src/songdata.c: $(wildcard src/*.mid) $(wildcard src/*.s2bank)
	$(TOOL_CMD) --write-song $(wildcard src/*.mid) $(wildcard src/*.s2bank) > $@


tiny4: $(TINYSOURCES) $(TINYHEADERS) $(TINYHACKS)
	$(CC) $(HCFLAGS) $(ARCHFLAGS) $(ADDFLAGS) \
		-o $@.unstripped.payload \
		-DULTRASMALL \
		-DSYNTH_PLAYBACK_SDL \
		-nostdlib  -nostartfiles -lc \
		$(MAINFILE) \
		$(ARCHLIBS)

	cp $@.unstripped.payload $@.payload 

	$(ARCHSTRIP) $(ARCHSTRIPOPT) $@.payload
	$(SSTRIP) $@.payload

	mv $@.payload tmp
	$(GZIP_TMP_CMD)

#	7za a -tgzip -mx=9 $@.payload.gz $@.payload
#	zopfli --i25 $@.payload
#	zopfli --i1000 $@.payload
#	mv $@.payload.gz tmp.gz
	cat src/selfextr.stub tmp.gz > $@
	rm tmp.gz

	chmod ugo+x $@

	@echo End result:
	@ls -lt $@


bin/jackvsynti2: $(MAINFILE) $(JACKSOURCES) $(VISHEADERS) $(VISHACKS)
	$(CC) $(CFLAGS) $(ARCHFLAGS) $(ADDFLAGS) \
		-o $@ \
		-DCOMPOSE_MODE \
                -DJACK_MIDI \
		-DSYNTH_COMPOSE_JACK \
		$(MAINFILE) $(filter %.c, $(JACKSOURCES)) \
		`pkg-config --cflags --libs jack` \
		$(ARCHLIBS) -lGLU -lGLEW

writ2: $(WRITERSOURCES) $(TINYHEADERS) $(TINYHACKS)
	$(CC) $(CFLAGS) $(ARCHFLAGS) $(ADDFLAGS) \
		-o $@ \
		-DDUMP_FRAMES_AND_SNDFILE \
		-DPLAYBACK_DURATION=10.f \
		$(MAINFILE) \
		`pkg-config --cflags --libs jack` \
		`sdl-config --cflags --libs` -lm -lGL -lGLU -lGLEW \
		-lsndfile

all: bin/jackvsynti2 tiny4 writ2
