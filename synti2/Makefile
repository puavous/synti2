.PHONY: nodefaultmake tools mostlyclean clean veryveryclean


CLTOOLDEPS = tools/cltool.cxx \
	tools/patchtool.cxx tools/patchtool.hpp \
	tools/miditool.cxx tools/miditool.hpp \
	tools/midimaptool.cxx tools/midimaptool.hpp \
	tools/misss.cxx tools/misss.hpp \
	tools/midihelper.cxx tools/midihelper.hpp \
	tools/captool.cxx tools/captool.hpp \
	tools/featurehelper.cxx tools/featurehelper.hpp \
	tools/keyvalhelper.cxx tools/keyvalhelper.hpp \
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


nodefaultmake:
	@echo "No default target is specified as of now."

tools: patchedit cltool

patchedit: $(PATCHEDITDEPS) Makefile
	g++ -o $@ $(filter %.cxx %.hpp %.c , $^) -lfltk \
		-Isrc -Iinclude \
		`fltk-config --cxxflags`  \
		`pkg-config --cflags --libs jack`

cltool: $(CLTOOLDEPS) Makefile
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
	     include/synti2_misss.h \
	     include/synti2_archdep.h \
	     tests/tinymaker.makefile \
	     example/selfextr.stub \

# These are additionally needed for compose-time with jack engine:
JACKFILES =  example/jack_shader_main.c src/synti2_jack.c src/synti2_midi.c

# These are additionally needed for off-line rendering:
DUMPFILES =  example/file_writer_main.c


# Instanssi 13 4k entry:
# Specifics needed are song, sounds, and gfx:
I13FILES = example/i13_4k/i13_4k.mid \
	   example/i13_4k/i13_4k.s2bank \
	   example/i13_4k/render.c \
	   example/i13_4k/shaders.c \
	   example/i13_4k/synti2_cap.h \
	   example/i13_4k/patchdes.dat \
	   example/glfuncs.c

# Instanssi 13 summamutikka entry, "sm":
# Specifics needed are song, sounds, and gfx:
SMFILES = example/i13_sm/pikademo.mid \
	   example/i13_sm/pikademo.s2bank \
	   example/i13_sm/render.c \
	   example/i13_sm/shaders.c \
	   example/i13_sm/glfuncs.c \
	   example/i13_sm/synti2_cap.h \
	   example/i13_sm/patchdes.dat


summamutikka:
	make tiny2 CUSTOMFILES="$(SMFILES)"
	cd hackpack2; make vis2
	mv hackpack2/vis2 ./jacksynti2

i13tiny:
	make tiny2 \
	    CUSTOMFILES="$(I13FILES)" \
	    NONOS="-DNO_SAFETY -DNO_CC -DNO_NOTEOFF -DNO_EXTRA_WAVETABLES -DNO_LOOPING_ENVELOPES -DNO_PITCH_SCALING -DNO_LEGATO -DNO_FINE_DETUNE -DNO_PITCH_BEND -DNO_VELOCITY -DNO_FILTER_RESO_ENVELOPE -DNO_NOTCH_FILTER -DNO_FILTER_CUT_ENVELOPE -DNO_FILTER_PITCH_FOLLOW -DNO_STEREO"
	cd hackpack2; make all
	mv hackpack2/vis2 ./jacksynti2


# For 4k stand-alone mode:
tiny2: $(TINYFILES) $(JACKFILES) $(DUMPFILES) $(CUSTOMFILES)
	-rm -r hackpack2
	-mkdir hackpack2
	cp $(filter %.c %.h %.dat, $^) hackpack2/
	echo 'NONOS=' $(NONOS) '' > hackpack2/Makefile
	cat tests/tinymaker.makefile >> hackpack2/Makefile
	cp example/selfextr.stub hackpack2/
	sed 's/^ *//g; s/  */ /g; s/ *\([=+\*,<>;//]\) */\1/g; s/ *\/\/.*$$/\\/g' \
		< $(filter %shaders.c, $^) > hackpack2/shaders.c
	./cltool patchdata $(filter %.s2bank, $(CUSTOMFILES)) hackpack2/patchdes.dat > hackpack2/patchdata.c
	./cltool parheader hackpack2/patchdes.dat > hackpack2/synti2_params.h
	./cltool songdata $(filter %.mid, $(CUSTOMFILES)) $(filter %.s2bank, $^) > hackpack2/songdata.c


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
		benchmark testdata.c \
	        patchedit cltool \
		one_star_j jacksynti2 cube_thing example1

veryveryclean: clean
	-rm include/synti2_params.h
