S2HEADERS = include/synti2_guts.h \
		include/synti2.h \
		include/synti2_jack.h \
		include/synti2_midi.h \
		include/synti2_misss.h \
		include/synti2_params.h \

S2SOURCES = src/synti2.c src/synti2_jack.c src/synti2_midi.c

PTSOURCES = tools/patchtool.cxx tools/patchtool.hpp 

# Ahaa.. you can have a make inside a make... 

cube_thing:
	make tinyexe GFX=example/cube_thing_gfx.c SND=example/cube_thing.s2bank SMF=example/cube_thing.mid NONOS='-DNO_NOTEOFF -DNO_LOOPING_ENVELOPES -DNO_EXTRA_WAVETABLES -DNO_VELOCITY -DNO_FILTER -DNO_SYSEX_RECEIVE -DNO_PITCH_BEND -DNO_CC'
	mv tinyexe cube_thing

jacksynti2:
	make visexe GFX=example/cube_thing_gfx.c SND=example/cube_thing.s2bank SMF=example/cube_thing.mid 
	mv visexe jacksynti2

pikkune:
	make tinyexe GFX=example/pikkune_gfx.c


# A "demo" contains: main (which can be always the same!?), graphics,
# patches (in original format), midi song (in sfm0/1 format)
# (These can be overridden from the command line:)
#MAIN ?= example/sdl_main.c
MAIN ?= example/sdl_main.c
GFX ?= tests/tryout_gfx2.c
SMF ?= example/example1.mid
SND ?= example/example1.s2bank
NONOS ?= 
BUTDOS ?= 

TINYEXESOURCES = $(MAIN) $(GFX) $(SND) $(SMF) 

VISEXESOURCES = tests/javtest.c $(GFX) $(SND) $(SMF)

GENPARDEPS = tools/genpar.cxx $(PTSOURCES)

PATCHEDITDEPS = tools/patchedit.cxx $(PTSOURCES)


# C flags common to all files (normal/debug build)
CFLAGS = -O3 -g -Wall -Wextra -pedantic

## For normal 64-bit build:
ARCHFLAGS = 
ARCHLIBS = `sdl-config --libs` -lm -lGL
ARCHSTRIP = strip
ARCHSTRIPOPT = -s -R .comment  -R .gnu.version \
		-R .note.gnu.build-id -R .gnu.hash \
		-R .eh_frame_hdr -R .eh_frame 

# For linking against 32-bit libraries (approx. 5% smaller packed exe):
# (and even 10 % when sstrip is used instead of strip!!
#ARCHFLAGS = -m32 -DNO_I64
#ARCHLIBS = `sdl-config --libs` -lm /usr/lib/libGL.so.1
#ARCHSTRIP = sstrip
#ARCHSTRIPOPT = 

# The HCFLAGS (as in "hardcore") are for making a very small executable.
# I used to get a crash in PulseAudio due to misaligned stack (pa uses
# vector registers and those instructions require 16 byte alignment). 
# I hope my current init replacement will be enough...
HCFLAGS = -Os  -Wextra -pedantic \
	$(NONOS)


# For some reason at least -msse4 seems to shave bytes.. so it is
# worthwhile to keep the following memories. I used the following the
# last time.. but there were some stability problems with the compo
# machine...

# -fno-strict-aliasing \
#          -nostdlib -nostartfiles -DULTRASMALL \
#          -ffast-math -fomit-frame-pointer \
#	  -fpredictive-commoning \
#          --param max-unroll-times=0 \
#          --param max-unrolled-insns=0 \
#          -msse4 -mavx -mhard-float 


nodefaultmake:
	@echo "No default target is specified as of now."

# Tools of all sorts have been popping about ...
tools: midi2synti2 patchedit genpar cbank

midi2synti2: tools/midi2synti2.cxx tools/miditool.cxx \
		tools/miditool.hpp Makefile
	g++ -Wextra -o $@ $(filter %.cxx, $+) \
		-lfltk `fltk-config --cxxflags` \
		`pkg-config --cflags --libs jack`

patchedit: $(PATCHEDITDEPS) Makefile
	g++ -o $@ $(filter %.cxx %.hpp , $^) -lfltk \
		`fltk-config --cxxflags`  \
		`pkg-config --cflags --libs jack`

genpar: $(GENPARDEPS)
	g++ -Itools/ -o $@ $(filter %.cxx , $^)

cbank: tools/cbank.cxx tools/patchtool.cxx tools/patchtool.hpp Makefile
	g++ -o $@ -Wextra $(filter %.cxx, $^)


# While still designing the patch system, we generate this for ease:

include/synti2_params.h: src/patchdesign.dat genpar
	./genpar $< $@


# Experiments with dry runs, benchmarking, and SDL

benchmark: tests/benchmark.c synti2.c synti2.h testdata.c Makefile
	$(CC) $(CFLAGS) -I./ -o $@ \
		$(filter %.c , $^) -lm


# This is a template for an easy 4k-builder 
dummy: # this is a hack to make the other hack always re-compile

tinyexe: $(TINYEXESOURCES) $(S2SOURCES) $(S2HEADERS) Makefile dummy \
		midi2synti2 cbank
	-mkdir hackpack
	./midi2synti2 $(filter %.mid, $^) > hackpack/songdata.c
	./cbank $(filter %.s2bank, $^) hackpack/patchdata.c
	cp $(filter %.c, $^) hackpack/

	$(CC) $(HCFLAGS) $(NONOS) $(ARCHFLAGS) $(ADDFLAGS) \
		-o hackpack/$@.unstripped.payload \
		-fwhole-program -flto \
		-Iinclude \
		-DULTRASMALL -nostdlib  -nostartfiles -lc \
		$(filter %.c, $(TINYEXESOURCES)) \
		$(filter %.c, $(S2SOURCES)) \
		hackpack/songdata.c hackpack/patchdata.c \
		$(ARCHLIBS)

#		-combine -fwhole-program -flto \


	cp hackpack/$@.unstripped.payload hackpack/$@.payload 

	$(ARCHSTRIP) $(ARCHSTRIPOPT) hackpack/$@.payload

	7za a -tgzip -mx=9 tmp.gz hackpack/$@.payload
	cat tests/selfextr.stub tmp.gz > $@
	rm tmp.gz

	chmod ugo+x $@

	@echo End result:
	@ls -lat $@


visexe: $(VISEXESOURCES) $(S2SOURCES) $(S2HEADERS) Makefile dummy \
		midi2synti2 cbank
	-mkdir hackpack

	./midi2synti2 $(filter %.mid, $^) > hackpack/songdata.c

	./cbank $(filter %.s2bank, $^) hackpack/patchdata.c
	cp $(filter %.c, $^) hackpack/

	$(CC) $(CFLAGS) $(NONOS) -I./include -DJACK_MIDI -o $@ \
		$(filter %.c , $^) \
		hackpack/songdata.c hackpack/patchdata.c \
		`pkg-config --cflags --libs jack` \
		`sdl-config --cflags --libs` -lm -lGL -lGLU



# Clean-up targets:

mostlyclean:
	-rm `find . -name "*~"` *.o
	-rm -r hackpack

clean: mostlyclean
	-rm *.payload tinyexe visexe \
		benchmark testsong.c test.misss \
		midi2synti2 patchedit genpar cbank

veryveryclean: clean
	-rm include/synti2_params.h
