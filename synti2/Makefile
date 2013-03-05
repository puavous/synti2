.PHONY: nodefaultmake mostlyclean clean veryveryclean


CLTOOLDEPS = tools/cltool.cxx \
	tools/patchtool.cxx tools/patchtool.hpp \
	tools/miditool.cxx tools/miditool.hpp \
	tools/midimaptool.cxx tools/midimaptool.hpp \
	tools/misss.cxx tools/misss.hpp \
	tools/midihelper.cxx tools/midihelper.hpp \
	src/synti2_midi.c

PATCHEDITDEPS = tools/patchedit.cxx \
                tools/patchtool.cxx tools/patchtool.hpp \
		tools/miditool.cxx tools/miditool.hpp \
		tools/midimaptool.cxx tools/midimaptool.hpp \
		tools/misss.cxx tools/misss.hpp \
		tools/midihelper.cxx tools/midihelper.hpp \
                src/synti2_midi.c


# C flags common to all files (normal/debug build)
CFLAGS = -O3 -ffast-math -g -Wall -Wextra -pedantic

# The HCFLAGS (as in "hardcore") are for making a very small executable.
# I used to get a crash in PulseAudio due to misaligned stack (pa uses
# vector registers and those instructions require 16 byte alignment). 
# I hope my current init replacement will be enough...
HCFLAGS = -Os -Wall -Wextra -pedantic \
	$(NONOS)

# With these, the dependency of libm could be lifted (just a few bytes gained):
#  -mfpmath=387 -funsafe-math-optimizations

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
SSTRIP = sstrip

# This must not be taken away: .gnu.hash

nodefaultmake:
	@echo "No default target is specified as of now."

# Tools of all sorts have been popping about ... cutting stock now
# tools: midi2synti2 patchedit genpar cbank

# Just for looking at the machine code being produced by gcc
#synti2.asm.annotated: $(S2SOURCES) $(S2HEADERS)
#	$(CC) $(HCFLAGS) -Iinclude -S -fverbose-asm -g src/synti2.c
#	as -alhnd synti2.s > $@

patchedit: $(PATCHEDITDEPS) Makefile
	g++ -o $@ $(filter %.cxx %.hpp %.c , $^) -lfltk \
		-Isrc -Iinclude \
		`fltk-config --cxxflags`  \
		`pkg-config --cflags --libs jack`

cltool: $(CLTOOLDEPS)
	g++ -Itools -Isrc -Iinclude -Wall -Wextra -o $@  $(filter %.cxx %.c, $^)


# While still designing the patch system, we generate this for ease:
#include/synti2_params.h: src/patchdesign.dat cltool
#	./cltool $< $@
# Will be different altogether..

# Experiments with dry runs, benchmarking, and SDL
#benchmark: tests/benchmark.c src/synti2.c include/synti2.h testdata.c Makefile
#	$(CC) $(CFLAGS) -I./ -I./include -o $@ \
#		$(filter %.c , $^) -lm



# These files are needed for every stand-alone 4k compilation:
TINYFILES = src/synti2.c \
	     src/synti2_fdec.c \
	     example/sdl_shader_main.c \
	     include/synti2.h \
	     include/synti2_guts.h \
	     include/synti2_params.h \
	     include/synti2_misss.h \
	     include/synti2_archdep.h \
	     tests/tinymaker.makefile \
	     tests/selfextr.stub \

# These are additionally needed for compose-time with jack engine:
JACKFILES =  example/jack_shader_main.c src/synti2_jack.c src/synti2_midi.c

# Instanssi 13 4k entry:
# Specifics needed are song, sounds, and gfx:
I13FILES = example/i13_4k/i13_4k.mid \
	   example/i13_4k/i13_4k.s2bank \
	   example/i13_4k/render.c \
	   example/i13_4k/shaders.c \
	   example/i13_4k/synti2_cap.h \
	   example/i13_4k/patchdes.dat \
	   example/glfuncs.c

i13tiny:
	make tiny2 \
	    CUSTOMFILES="$(I13FILES)" \
	    NONOS="-DNO_SAFETY -DNO_CC -DNO_NOTEOFF -DNO_EXTRA_WAVETABLES -DNO_LOOPING_ENVELOPES -DNO_PITCH_SCALING -DNO_LEGATO -DNO_FINE_DETUNE -DNO_PITCH_BEND -DNO_VELOCITY -DNO_FILTER_RESO_ENVELOPE -DNO_NOTCH_FILTER -DNO_FILTER_CUT_ENVELOPE -DNO_FILTER_PITCH_FOLLOW -DNO_STEREO"
	mv hackpack2/vis2 ./jacksynti2

# For compose mode:
#vis2: $(TINYFILES) $(CUSTOMFILES)
#	-rm -r hackpack2
#	-mkdir hackpack2


# For 4k stand-alone mode:
tiny2: $(TINYFILES) $(JACKFILES) $(CUSTOMFILES)
	-rm -r hackpack2
	-mkdir hackpack2
	cp $(filter %.c %.h %.dat, $^) hackpack2/
#	cp include/synti2_cap_default
	cp tests/tinymaker.makefile hackpack2/Makefile
	cp tests/selfextr.stub hackpack2/
	sed 's/^ *//g; s/  */ /g; s/ *\([=+\*,<>;//]\) */\1/g; s/ *\/\/.*$$/\\/g' \
		< $(filter %shaders.c, $^) > hackpack2/shaders.c
	./cbank $(filter %.s2bank, $(CUSTOMFILES)) hackpack2/patchdata.c hackpack2/patchdes.dat 
	./genpar hackpack2/patchdes.dat  hackpack2/synti2_params.h
	./midi2synti2 $(filter %.mid, $(CUSTOMFILES)) $(filter %.s2bank, $^) > hackpack2/songdata.c

	cd hackpack2; make


#withoutseq:
#	make tinyexe MAIN=tests/withoutseq.c SND=patches/miniFMset.s2bank SMF=example/minimal.mid GFX='' \
#		NONOS='-DNO_NOTHING -DBUT_USE_OUTPUT_SQUASH -DDO_RESET_PHASE -DEXTREME_NO_SEQUENCER'
#	mv tinyexe withoutseq





# Clean-up targets:

mostlyclean:
	-rm `find . -name "*~"` *.o
	-rm -r hackpack
	-rm -r hackpack2

clean: mostlyclean
	-rm *.payload tinyexe visexe withoutseq \
		benchmark testdata.c test.misss \
		midi2synti2 patchedit genpar cbank \
		one_star_j jacksynti2 cube_thing example1

veryveryclean: clean
	-rm include/synti2_params.h
