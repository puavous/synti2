.PHONY: nodefaultmake final mostlyclean clean veryveryclean

nodefaultmake:
	@echo "No default target is specified as of now."

bin/synti2gui: engine/synti2_midi.o
	cd gui; make
	mv gui/synti2gui bin/synti2gui

bin/atest: engine/atest.c bin
	gcc -o $@ $<

engine/synti2_archdep.h: bin/atest
	bin/atest > $@

engine/synti2_midi.o: engine/synti2_midi.c engine/synti2_midi.h engine/synti2_archdep.h
	g++ -c -o $@ $<

# These files are needed for any synth build:
ENGINEFILES = engine/synti2.c \
	     engine/synti2_fdec.c \
	     engine/synti2_fenc.c \
	     engine/synti2_fcoding.h \
	     engine/synti2.h \
	     engine/synti2_guts.h \
	     engine/synti2_misss.h \
	     engine/synti2_archdep.h \
	     engine/synti2_limits.h \

# These are additionally needed for tiny 4k executables:
TINYFILES = example/selfextr.stub \
	     example/general_main.c 

# These are additionally needed for compose-time with jack engine:
JACKFILES =  engine/synti2_jack.c engine/synti2_jack.h \
	engine/synti2_midi.c engine/synti2_midi.h engine/synti2_midi_guts.h \
	engine/midi_spec.h \
	engine/synti2_cap_full.h

# Assembly 2014 4k entry:
# Specifics needed are song, sounds, and gfx:
A14FILES = example/a14_4k/a14_4k.mid \
	   example/a14_4k/a14_4k.s2bank \
	   example/shader_test/render.c \
	   example/a14_4k/vertex.vert \
	   example/shader_test/fragment.frag \
	   example/glfuncs.c

# A more final attempt for 4k stand-alone
CUSTOMFILES=$(A14FILES)

product: $(ENGINEFILES) $(TINYFILES) $(JACKFILES) $(CUSTOMFILES) tests/tinymaker4.makefile Makefile
	mkdir -p product
	mkdir -p product/src
	mkdir -p product/bin
	cp $^ product/src
	cp tests/tinymaker4.makefile product/Makefile

bin/jackvsynti2: product
	cd product; make bin/jackvsynti2
	mv product/bin/jackvsynti2 ./bin

final: product
	cd product; make all

# Clean-up targets:
mostlyclean:
	cd gui; make clean
	-rm `find . -name "*~"`
	-rm engine/*.o
	-rm -r product

clean: mostlyclean
	cd gui; make veryclean
	-rm bin/*
	-rm jacksynti2
	-rm engine/synti2_archdep.h
