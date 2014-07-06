.PHONY: nodefaultmake tools mostlyclean clean veryveryclean

# FIXME: Should have a proper make system, finally..

GUIDEPS = gui/MainWindow.hpp gui/MainWindow.cxx \
          gui/PatchesViewFl.hpp gui/PatchesViewFl.cxx \
          gui/MidiMapperViewFl.hpp gui/MidiMapperViewFl.cxx \
          gui/FeaturesViewFl.hpp gui/FeaturesViewFl.cxx \
          gui/ExeBuilderViewFl.hpp gui/ExeBuilderViewFl.cxx \
          gui/Patch.hpp gui/Patch.cxx \
          gui/PatchBank.hpp gui/PatchBank.cxx \
          gui/MidiMap.hpp gui/MidiMap.cxx \
          gui/midihelper.hpp gui/midihelper.cxx \
          gui/miditool.hpp gui/miditool.cxx \
	  gui/misss.hpp gui/misss.cxx gui/misssevent.hpp \
	  engine/synti2_midi.h engine/synti2_midi.c \
          gui/main.cxx

# C flags common to all files (normal/debug build)
CFLAGS = -O3 -ffast-math -g -Wall -Wextra -pedantic

nodefaultmake:
	@echo "No default target is specified as of now."

tools: synti2gui

synti2gui: $(GUIDEPS) Makefile
	g++ -Wall -g -o $@ $(filter %.cxx %.hpp %.c , $^) -lfltk \
		-Iengine \
		`fltk-config --cxxflags`  \
		`pkg-config --cflags --libs jack`

# Experiments with dry runs, benchmarking, and SDL
#benchmark: tests/benchmark.c engine/synti2.c include/synti2.h testdata.c Makefile
#	$(CC) $(CFLAGS) -I./ -I./include -o $@ \
#		$(filter %.c , $^) -lm

bin:
	mkdir bin

bin/atest: engine/atest.c bin
	gcc -o $@ $<

engine/synti2_archdep.h: bin/atest
	bin/atest > $@

# These files are needed for every stand-alone 4k compilation:
TINYFILES =  engine/synti2.c \
	     engine/synti2_fdec.c \
	     engine/synti2_fenc.c \
	     engine/synti2_fcoding.h \
	     example/general_main.c \
	     engine/synti2.h \
	     engine/synti2_guts.h \
	     engine/synti2_misss.h \
	     engine/synti2_archdep.h \
	     engine/synti2_limits.h \
	     tests/tinymaker.makefile \
	     example/selfextr.stub 

# These are additionally needed for compose-time with jack engine:
JACKFILES =  engine/synti2_jack.c engine/synti2_midi.c engine/synti2_cap_full.h

# These are additionally needed for off-line rendering:
DUMPERFILES = 

# Assembly 2014 4k entry:
# Specifics needed are song, sounds, and gfx:
A14FILES = example/a14_4k/a14_4k.mid \
	   example/a14_4k/a14_4k.s2bank \
	   example/shader_test/render.c \
	   example/a14_4k/vertex.vert \
	   example/shader_test/fragment.frag \
	   example/glfuncs.c

# Instanssi 2014 4k entry:
# Specifics needed are song, sounds, and gfx:
I14FILES = example/i14_4k/i14_4k.mid \
	   example/i14_4k/i14_4k.s2bank \
	   example/i14_4k/render.c \
	   example/i14_4k/vertex.vert \
	   example/i14_4k/fragment.frag \
	   example/glfuncs.c

# Assembly Summer 13 4k entry:
# Specifics needed are song, sounds, and gfx:
A13FILES = example/a13_4k/a13_4k.mid \
	   example/a13_4k/a13_4k.s2bank \
	   example/a13_4k/render.c \
	   example/a13_4k/shaders.c \
	   example/glfuncs.c

# Instanssi 13 4k entry:
# Specifics needed are song, sounds, and gfx:
I13FILES = example/i13_4k/i13_4k.mid \
	   example/i13_4k/i13_4k.s2bank \
	   example/i13_4k/render.c \
	   example/i13_4k/shaders.c \
	   example/glfuncs.c

# Tietokonegrafiikan perusteet 13 course example:
TGP13FILES = example/raymarch_try/i13_4k.mid \
	   example/raymarch_try/i13_4k.s2bank \
	   example/raymarch_try/render.c \
	   example/raymarch_try/shaders.c \
	   example/raymarch_try/glfuncs.c

# Instanssi 13 summamutikka entry, "sm":
# Specifics needed are song, sounds, and gfx:
SMFILES = example/i13_sm/pikademo.mid \
	   example/i13_sm/pikademo.s2bank \
	   example/i13_sm/render.c \
	   example/i13_sm/shaders.c \
	   example/i13_sm/glfuncs.c 

summamutikka:
	make tiny2 CUSTOMFILES="$(SMFILES)"
	cd hackpack2; make vis2
	mv hackpack2/vis2 ./jacksynti2

a13tiny:
	make tiny2 \
	    CUSTOMFILES="$(A13FILES)" \
	    NONOS="-DNO_FULLSCREEN -DNO_STEREO -DNO_FILTER -DNO_SAFETY -DNO_CC -DNO_NOTEOFF -DNO_EXTRA_WAVETABLES -DNO_LOOPING_ENVELOPES -DNO_PITCH_SCALING -DNO_LEGATO -DNO_FINE_DETUNE -DNO_PITCH_BEND -DNO_VELOCITY -DNO_FILTER_RESO_ENVELOPE -DNO_NOTCH_FILTER -DNO_FILTER_CUT_ENVELOPE -DNO_FILTER_PITCH_FOLLOW -DNO_SYSEX_RECEIVE"
# -DNO_STEREO
	cd hackpack2; make all
	mv hackpack2/vis2 ./jacksynti2

a13tiny32:
	make tiny2_32 \
	    CUSTOMFILES="$(A13FILES)" \
	    NONOS="-DNO_FULLSCREEN -DNO_SAFETY -DNO_CC -DNO_NOTEOFF -DNO_EXTRA_WAVETABLES -DNO_LOOPING_ENVELOPES -DNO_PITCH_SCALING -DNO_LEGATO -DNO_FINE_DETUNE -DNO_PITCH_BEND -DNO_VELOCITY -DNO_FILTER_RESO_ENVELOPE -DNO_NOTCH_FILTER -DNO_FILTER_CUT_ENVELOPE -DNO_FILTER_PITCH_FOLLOW -DNO_STEREO -DNO_SYSEX_RECEIVE"
	cd hackpack2; make all
	mv hackpack2/vis2 ./jacksynti2

tgptiny:
	make tiny2 \
	    CUSTOMFILES="$(TGP13FILES)" \
	    NONOS="-DNO_SAFETY -DNO_CC -DNO_NOTEOFF -DNO_EXTRA_WAVETABLES -DNO_LOOPING_ENVELOPES -DNO_PITCH_SCALING -DNO_LEGATO -DNO_FINE_DETUNE -DNO_PITCH_BEND -DNO_VELOCITY -DNO_FILTER_RESO_ENVELOPE -DNO_NOTCH_FILTER -DNO_FILTER_CUT_ENVELOPE -DNO_FILTER_PITCH_FOLLOW -DNO_STEREO -DNO_SYSEX_RECEIVE"
	cd hackpack2; make all
	mv hackpack2/vis2 ./jacksynti2

i13tiny:
	make tiny2 \
	    CUSTOMFILES="$(I13FILES)" \
	    NONOS="-DNO_SAFETY -DNO_CC -DNO_NOTEOFF -DNO_EXTRA_WAVETABLES -DNO_LOOPING_ENVELOPES -DNO_PITCH_SCALING -DNO_LEGATO -DNO_FINE_DETUNE -DNO_PITCH_BEND -DNO_VELOCITY -DNO_FILTER_RESO_ENVELOPE -DNO_NOTCH_FILTER -DNO_FILTER_CUT_ENVELOPE -DNO_FILTER_PITCH_FOLLOW -DNO_STEREO -DNO_SYSEX_RECEIVE"
	cd hackpack2; make all
	mv hackpack2/vis2 ./jacksynti2

# A more final attempt for 4k stand-alone
hack3:
	make tiny3 CUSTOMFILES="$(A14FILES)"
	cd hackpack3; make all
	mv hackpack3/vis2 ./jacksynti2
	ls -l hackpack3/tiny2

tiny3: $(TINYFILES) $(JACKFILES) $(DUMPERFILES) $(CUSTOMFILES)
	-rm -r hackpack3
	mkdir hackpack3
	cp $(filter %.c %.h , $^) hackpack3/
	cp tests/tinymaker3.makefile hackpack3/Makefile
	cp example/selfextr.stub hackpack3/

# ------------------- Shaders.
#	shaders to c source:
	echo $(filter %vertex.vert, $^)
	echo '/*Shaders, by the messiest makefile ever..*/' > hackpack3/shaders.c
	echo 'const GLchar *vs=""' >> hackpack3/shaders.c

	sed 's/^ *//g; s/  */ /g; s/ *\([=+\*,<>;//]\) */\1/g; s/\(.*\)\/\/.*$$/\1/g; s/\(.*\)/"\1"/g;' \
		< $(filter %vertex.vert, $^) >> hackpack3/shaders.c
	echo '"";' >> hackpack3/shaders.c
	echo 'const GLchar *fs=""' >> hackpack3/shaders.c
	sed 's/^ *//g; s/  */ /g; s/ *\([=+\*,<>;//]\) */\1/g; s/\(.*\)\/\/.*$$/\1/g; s/\(.*\)/"\1"/g;' \
		< $(filter %fragment.frag, $^) >> hackpack3/shaders.c
	echo '"";' >> hackpack3/shaders.c

#	shaders to c source, using the shader_minifier tool:
	mono ~/files/hacking/shader_minifier/shader_minifier.exe --format none \
		--preserve-externals \
		-o vertshader.tmp $(filter %vertex.vert, $^)
	mono ~/files/hacking/shader_minifier/shader_minifier.exe --format none \
		--preserve-externals \
		-o fragshader.tmp $(filter %fragment.frag, $^)
	echo '/*Shaders, by the messiest makefile ever..*/' > hackpack3/shaders.c
	echo -n 'const GLchar *vs="' >> hackpack3/shaders.c
	cat vertshader.tmp >> hackpack3/shaders.c
	echo '";' >> hackpack3/shaders.c
	echo -n 'const GLchar *fs="' >> hackpack3/shaders.c
	cat fragshader.tmp >> hackpack3/shaders.c
	echo '";' >> hackpack3/shaders.c

	./synti2gui --write-caps $(filter %.s2bank, $(CUSTOMFILES)) > hackpack3/synti2_cap_custom.h
	./synti2gui --write-patches $(filter %.s2bank, $(CUSTOMFILES)) > hackpack3/patchdata.c
	./synti2gui --write-song $(filter %.mid, $(CUSTOMFILES)) $(filter %.s2bank, $(CUSTOMFILES)) > hackpack3/songdata.c

#For the visualization only:
	cp engine/synti2_midi.h hackpack3/
	cp engine/synti2_jack.h hackpack3/
	cp engine/synti2_midi_guts.h hackpack3/
	cp engine/midi_spec.h hackpack3/

# Clean-up targets:
mostlyclean:
	-rm `find . -name "*~"` *.o
	-rm -r hackpack
	-rm -r hackpack2
	-rm -r hackpack3

clean: mostlyclean
	-rm -r bin
	-rm jacksynti2 synti2gui
	-rm engine/synti2_archdep.h
