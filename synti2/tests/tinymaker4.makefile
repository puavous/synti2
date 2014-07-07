# This is a makefile template for preview, test, and final version of
# a stand-alone synth player exe and all other targets necessary for
# real-time compose-mode and frame dumps. This should be exported to a
# folder along with release-specific (partly generated) source codes.
.phony: all preview test final

# The HCFLAGS (as in "hardcore") are for making a very small
# executable. Could be tuned further?
HCFLAGS = -Os -mfpmath=387 -funsafe-math-optimizations -fwhole-program \
	-Wall -Wextra -pedantic

# With these, the dependency of libm could be lifted (just a few bytes
# gained, though): -mfpmath=387 -funsafe-math-optimizations

# Normal CFLAGS are for real-time compose-mode:
CFLAGS = -O3 -ffast-math -Wall -Wextra -pedantic

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

# These are required for the compilation:
MAINFILE = general_main.c
JACKSOURCES = synti2_jack.c synti2_midi.c
TINYHEADERS = synti2_archdep.h synti2_limits.h synti2_cap_custom.h  synti2_guts.h  synti2.h  synti2_misss.h
VISHEADERS = synti2_archdep.h  synti2_cap_full.h  synti2_guts.h  synti2.h  synti2_misss.h
TINYHACKS = shaders.c render.c glfuncs.c patchdata.c songdata.c 
VISHACKS =  shaders.c render.c glfuncs.c

# Just for looking at the machine code being produced by gcc
#synti2.asm.annotated: synti2.c
#	$(CC) $(HCFLAGS) -S -fverbose-asm -g synti2.c
#	as -alhnd synti2.s > $@


tiny2: $(TINYSOURCES) $(TINYHEADERS) $(TINYHACKS)
	$(CC) $(HCFLAGS) $(NONOS) $(ARCHFLAGS) $(ADDFLAGS) \
		-o $@.unstripped.payload \
		-DULTRASMALL \
		-DSYNTH_PLAYBACK_SDL \
		-nostdlib  -nostartfiles -lc \
		$(MAINFILE) \
		$(ARCHLIBS)

	cp $@.unstripped.payload $@.payload 

	$(ARCHSTRIP) $(ARCHSTRIPOPT) $@.payload
	$(SSTRIP) $@.payload

	7za a -tgzip -mx=9 $@.payload.gz $@.payload
#	zopfli --i25 $@.payload
#	zopfli --i1000 $@.payload
	mv $@.payload.gz tmp.gz
	cat selfextr.stub tmp.gz > $@
	rm tmp.gz

	chmod ugo+x $@

	@echo End result:
	@ls -lt $@


#The "no-nos" are used here, too, now(!):
vis2: $(MAINFILE) $(JACKSOURCES) $(VISHEADERS) $(VISHACKS)
	$(CC) $(CFLAGS) $(NONOS) $(ARCHFLAGS) $(ADDFLAGS) \
		-o $@ \
		-DCOMPOSE_MODE \
                -DJACK_MIDI \
		-DSYNTH_COMPOSE_JACK \
		$(MAINFILE) $(filter %.c, $(JACKSOURCES)) \
		`pkg-config --cflags --libs jack` \
		$(ARCHLIBS) -lGLU -lGLEW

#The "no-nos" are used here, too, now(!):
writ2: $(WRITERSOURCES) $(TINYHEADERS) $(TINYHACKS)
	$(CC) $(CFLAGS) $(NONOS) $(ARCHFLAGS) $(ADDFLAGS) \
		-o $@ \
		-DDUMP_FRAMES_AND_SNDFILE \
		-DPLAYBACK_DURATION=10.f \
		$(MAINFILE) \
		`pkg-config --cflags --libs jack` \
		`sdl-config --cflags --libs` -lm -lGL -lGLU -lGLEW \
		-lsndfile

all: vis2 tiny2 writ2
