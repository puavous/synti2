.phony: all
# The make system is still a hack. Maybe even worse than ever.  What I
# really need is a robust and neat way to generate a stand-alone
# source package... No time to think about this right now, though.

# The HCFLAGS (as in "hardcore") are for making a very small executable.
# I used to get a crash in PulseAudio due to misaligned stack (pa uses
# vector registers and those instructions require 16 byte alignment). 
# I hope my current init replacement will be enough...
HCFLAGS = -Os -mfpmath=387 -funsafe-math-optimizations -fwhole-program \
	-Wall -Wextra -pedantic \
	$(NONOS)

# With these, the dependency of libm could be lifted (just a few bytes gained, though):
#  -mfpmath=387 -funsafe-math-optimizations


CFLAGS = -O3 -ffast-math -g -Wall -Wextra -pedantic

## For normal 64-bit build:
ARCHFLAGS = `sdl-config --cflags`
ARCHLIBS = `sdl-config --libs` -lGL -lm

# For linking against 32-bit libraries (approx. 5% smaller packed exe):
# On my Fedora 16, as of now, the 32 bit SDL fails to init audio
# (according to the error message, there is no available device). This
# worked fine on Fedora 14, so I sort of think this is an issue in the
# current version of SDL.i686)
#ARCHFLAGS = -m32 -DNO_I64 `sdl-config --cflags` 
#ARCHLIBS = `sdl-config --libs` -lm /usr/lib/libGL.so.1

# Looks as if it is best to strip first with strip and then sstrip..
ARCHSTRIP = strip
ARCHSTRIPOPT = -s -R .comment  -R .gnu.version \
		-R .note.gnu.build-id \
		-R .eh_frame_hdr -R .eh_frame 

# I'm taking away all I can so that the executable still works...
# This must not be taken away: .gnu.hash (Seems to work on the build
# system, but not others.. need to study the elf business a bit. See
# how --hash-style= option affects this)

SSTRIP = sstrip

# These are required for the compilation:
JACKSOURCES = jack_shader_main.c synti2.c synti2_jack.c synti2_midi.c
WRITERSOURCES = file_writer_main.c synti2.c 
TINYSOURCES = sdl_shader_main.c synti2.c
TINYHEADERS = synti2_archdep.h  synti2_cap.h  synti2_guts.h  synti2.h  synti2_misss.h  synti2_params.h
TINYHACKS = shaders.c render.c patchdata.c songdata.c glfuncs.c

# Just for looking at the machine code being produced by gcc
#synti2.asm.annotated: $(S2SOURCES) $(S2HEADERS)
#	$(CC) $(HCFLAGS) -Iinclude -S -fverbose-asm -g src/synti2.c
#	as -alhnd synti2.s > $@


tiny2: $(TINYSOURCES) $(TINYHEADERS) $(TINYHACKS)
	$(CC) $(HCFLAGS) $(NONOS) $(ARCHFLAGS) $(ADDFLAGS) \
		-o $@.unstripped.payload \
		-DULTRASMALL -DNO_SAFETY -nostdlib  -nostartfiles -lc \
		$(filter %.c, $(TINYSOURCES)) \
		$(ARCHLIBS)

	cp $@.unstripped.payload $@.payload 

	$(ARCHSTRIP) $(ARCHSTRIPOPT) $@.payload
	$(SSTRIP) $@.payload

#	7za a -tgzip -mx=9 $@.payload.gz $@.payload
	zopfli --i1000 $@.payload
	mv $@.payload.gz tmp.gz
	cat selfextr.stub tmp.gz > $@
	rm tmp.gz

	chmod ugo+x $@

	@echo End result:
	@ls -lt $@

#		-I../include \
#The "no-nos" are used here, too, now(!):
vis2: $(JACKSOURCES) $(TINYHEADERS) $(TINYHACKS)
	$(CC) $(CFLAGS) $(NONOS) $(ARCHFLAGS) $(ADDFLAGS) \
		-o $@ \
                -DJACK_MIDI -DNO_FULLSCREEN \
		$(filter %.c, $(JACKSOURCES)) \
		`pkg-config --cflags --libs jack` \
		$(ARCHLIBS) -lGLU

	@echo End result:
	@ls -lt $@

#The "no-nos" are used here, too, now(!):
writ2: $(WRITERSOURCES) $(TINYHEADERS) $(TINYHACKS)
	$(CC) $(CFLAGS) $(ARCHFLAGS) $(ADDFLAGS) \
		-o $@ -DNO_DEFAULT_CAPS \
		$(NONOS) \
		$(filter %.c, $(WRITERSOURCES)) \
		`pkg-config --cflags --libs jack` \
		`sdl-config --cflags --libs` -lm -lGL -lGLU \
		-lsndfile

	@echo End result:
	@ls -lt $@


all: vis2 tiny2
