.PHONY: nodefaultmake tools mostlyclean clean veryveryclean

# FIXME: Should have a proper make system, finally..

# C flags common to all files (normal/debug build)
CFLAGS = -O3 -ffast-math -g -Wall -Wextra -pedantic

nodefaultmake:
	@echo "No default target is specified as of now."

bin/synti2gui: engine/synti2_midi.o
	cd gui; make
	mv gui/synti2gui bin/synti2gui

# Experiments with dry runs, benchmarking, and SDL
#benchmark: tests/benchmark.c engine/synti2.c include/synti2.h testdata.c Makefile
#	$(CC) $(CFLAGS) -I./ -I./include -o $@ \
#		$(filter %.c , $^) -lm

bin/atest: engine/atest.c bin
	gcc -o $@ $<

engine/synti2_archdep.h: bin/atest
	bin/atest > $@

engine/synti2_midi.o: engine/synti2_midi.c engine/synti2_midi.h engine/synti2_archdep.h
	g++ -c -o $@ $<



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
	rm vertshader.tmp fragshader.tmp

	bin/synti2gui --write-caps $(filter %.s2bank, $(CUSTOMFILES)) > hackpack3/synti2_cap_custom.h
	bin/synti2gui --write-patches $(filter %.s2bank, $(CUSTOMFILES)) > hackpack3/patchdata.c
	bin/synti2gui --write-song $(filter %.mid, $(CUSTOMFILES)) $(filter %.s2bank, $(CUSTOMFILES)) > hackpack3/songdata.c

#For the visualization only:
	cp engine/synti2_midi.h hackpack3/
	cp engine/synti2_jack.h hackpack3/
	cp engine/synti2_midi_guts.h hackpack3/
	cp engine/midi_spec.h hackpack3/

# Clean-up targets:
mostlyclean:
	cd gui; make clean
	-rm `find . -name "*~"`
	-rm engine/*.o
	-rm -r hackpack3

clean: mostlyclean
	cd gui; make veryclean
	-rm bin/*
	-rm jacksynti2
	-rm engine/synti2_archdep.h
