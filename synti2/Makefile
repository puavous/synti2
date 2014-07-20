.PHONY: nodefaultmake tools final mostlyclean clean

nodefaultmake:
	@echo "No default target is specified as of now."
	@echo "'make product' generates a source folder for the current project,"
	@echo "which must be defined in 'current.makefile'."

# Variable CUSTOMFILES should be defined in the included file:
include current.makefile

tools: bin/synti2gui

bin/synti2gui: engine/synti2_midi.o
	cd gui; make
	mv gui/synti2gui bin/synti2gui

bin/atest: engine/atest.c
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
	     engine/synti2_limits.h

# These are additionally needed for tiny executables (like 4k intros):
TINYFILES = example/selfextr.stub \
	     example/general_main.c 

# These are additionally needed for any compose-mode MIDI synth:
MIDIFILES = engine/synti2_midi.c engine/synti2_midi.h engine/synti2_midi_guts.h \
	     engine/midi_spec.h 

# These are additionally needed for compose-time with jack engine:
JACKFILES = $(MIDIFILES) engine/synti2_jack.c engine/synti2_jack.h \
	engine/synti2_cap_full.h

# All these are required for "current production":
PRODFILES = $(ENGINEFILES) $(TINYFILES) $(JACKFILES) $(CUSTOMFILES) tests/tinymaker4.makefile

product/: tools Makefile $(PRODFILES)
	mkdir -p product
	mkdir -p product/src
	mkdir -p product/bin
	cp $(ENGINEFILES) $(TINYFILES) $(JACKFILES) $(CUSTOMFILES) product/src
	cp tests/tinymaker4.makefile product/Makefile

bin/jackvsynti2: product/ $(PRODFILES)
	cd product; make bin/jackvsynti2
	mv product/bin/jackvsynti2 ./bin

final: product/
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
