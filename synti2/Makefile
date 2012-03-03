S2HEADERS = include/synti2_guts.h \
		include/synti2.h \
		include/synti2_jack.h \
		include/synti2_misss.h \
		include/synti2_params.h \

S2SOURCES = src/synti2.c src/synti2_jack.c

# A "demo" contains: main (which can be always the same!), graphics,
# patches (in original format), midi song (in sfm0/1 format)
GFX = tests/tryout_gfx.c
SMF = tests/minimal.mid
SND = patches/testi1.s2bank
NONOS = -DNO_SYSEX_RECEIVE -DNO_EXTRA_SYSEX 

# -DNO_LOOPING_ENVELOPES
# -DULTRASMALL
# -DNO_LOOPING_ENVELOPES

TINYHACKSOURCES = tests/sdltest.c $(GFX) $(SND) $(SMF) 



JSYNTI2DEPS = tests/jsynti2.c tests/testbank.c testsong.c \
	src/synti2.c src/synti2_jack.c

JVISDEPS = tests/javtest.c tests/testbank.c testsong.c \
	 src/synti2.c src/synti2_jack.c \
	 tests/tryout_gfx2.c $(S2HEADERS)

GENPARDEPS = tools/genpar.cxx tools/patchtool.cxx tools/patchtool.hpp 

PATCHEDITDEPS = tools/patchedit.cxx tools/patchtool.hpp	\
		tools/patchtool.cxx

SDLTESTDEPS = tests/sdltest.c tests/testbank.c tests/tryout_gfx.c \
	testsong.c src/synti2.c include/synti2.h 

TINYPLAYERDEPS = tests/tinyplayer.c tests/testbank.c testsong.c \
	 src/synti2.c include/synti2.h 


# C flags common to all files (normal/debug build)
CFLAGS = -O3 -g -Wall -pedantic

## For normal 64-bit build:
#ARCHFLAGS = 
#ARCHLIBS = `sdl-config --libs` -lm -lGL -lGLU
#ARCHSTRIP = strip
#ARCHSTRIPOPT = -s -R .comment  -R .gnu.version \
#		-R .note.gnu.build-id -R .gnu.hash \
#		-R .eh_frame_hdr -R .eh_frame 

# For linking against 32-bit libraries (approx. 5% smaller packed exe):
# (and even 10 % when sstrip is used instead of strip!!
ARCHFLAGS = -m32 -DNO_I64
ARCHLIBS = `sdl-config --libs` -lm /usr/lib/libGL.so.1 /usr/lib/libGLU.so.1
ARCHSTRIP = sstrip
ARCHSTRIPOPT = 

# The HCFLAGS (as in "hardcore") are for making a very small executable.
# I used to get a crash in PulseAudio due to misaligned stack (pa uses
# vector registers and those instructions require 16 byte alignment). 
# I hope my current init replacement will be enough...
HCFLAGS = -Os -g -Wall -pedantic \
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

# Tools of all sorts have been popping in here ...
tools: misssify midi2synti2 patchedit genpar

misssify: tools/misssify.c Makefile
	$(CC) $(CFLAGS) -I./include -Wextra -o $@ tools/misssify.c -lm

cbank: tools/cbank.cxx tools/patchtool.cxx tools/patchtool.hpp Makefile
	g++ -o $@ -Wextra $(filter %.cxx, $^)

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


# Experiments with jack

jsynti2: $(JSYNTI2DEPS) $(S2HEADERS) Makefile
	$(CC) $(CFLAGS) -I./include -DJACK_MIDI -o $@ \
		$(filter %.c , $^) \
		`pkg-config --cflags --libs jack` -lm

jvis: $(JVISDEPS) Makefile
	$(CC) $(CFLAGS) -I./include -DJACK_MIDI -o $@ \
		$(filter %.c , $^) \
		`pkg-config --cflags --libs jack` \
		`sdl-config --cflags --libs` -lm -lGL -lGLU

include/synti2_params.h: src/patchdesign.dat genpar
	./genpar $< $@


# Experiments with dry runs, benchmarking, and SDL

benchmark: tests/benchmark.c synti2.c synti2.h testdata.c Makefile
	$(CC) $(CFLAGS) -I./ -o $@ \
		$(filter %.c , $^) -lm

testsong.c: tests/minimal.mid misssify Makefile
	./misssify --tpq 12 --velocity-all 100 --only-on $<
#	./misssify --tpq 12 --velocity-all 100 --only-on testi.mid
#	./misssify --tpq 12 --only-on testi.mid
#	./misssify --tpq 12 testi.mid
#	./misssify testi.mid
	python tools/file2c.py test.misss hacksong > testsong.c

sdltest: $(SDLTESTDEPS) Makefile
	$(CC) $(CFLAGS) -o $@ -Iinclude \
		$(filter %.c , $(SDLTESTDEPS)) \
	   `sdl-config --cflags --libs` -lm -lGL -lGLU

# This is a template for an easy 4k-builder 
# Basically only TINYHACKSOURCES and HCFLAGS need to be changed.
tinyhack: $(TINYHACKSOURCES) $(S2SOURCES) $(S2HEADERS) Makefile \
		misssify cbank
	mkdir -p hackpack
	./misssify --tpq 12 --velocity-all 100 --only-on \
		$(filter %.mid, $^) hackpack/song.misss
	python tools/file2c.py hackpack/song.misss hacksong > hackpack/songdata.c
	./cbank $(filter %.s2bank, $^) hackpack/patchdata.c
	cp $(filter %.c, $^) hackpack/

	$(CC) $(HCFLAGS) $(ARCHFLAGS) -o hackpack/$@.unstripped.payload \
		-combine -fwhole-program -flto \
		-Iinclude \
		-DULTRASMALL -nostdlib  -nostartfiles -lc \
		$(filter %.c, $(TINYHACKSOURCES)) \
		$(filter %.c, $(S2SOURCES)) \
		hackpack/songdata.c hackpack/patchdata.c \
		$(ARCHLIBS)

	cp hackpack/$@.unstripped.payload hackpack/$@.payload 

	$(ARCHSTRIP) $(ARCHSTRIPOPT) hackpack/$@.payload

	7za a -tgzip -mx=9 tmp.gz hackpack/$@.payload
	cat tests/selfextr.stub tmp.gz > $@
	rm tmp.gz

	chmod ugo+x $@



tinyplayer: $(TINYPLAYERDEPS) Makefile
	$(CC) $(HCFLAGS) $(ARCHFLAGS) -o $@.unstripped.payload \
		-combine -fwhole-program -flto \
		-Iinclude \
		-DULTRASMALL -nostdlib  -nostartfiles -lc \
		$(filter %.c, $(TINYPLAYERDEPS)) \
		$(ARCHLIBS)

	cp $@.unstripped.payload $@.payload 
	$(ARCHSTRIP) $(ARCHSTRIPOPT) $@.payload
	7za a -tgzip -mx=9 tmp.gz $@.payload
	cat tests/selfextr.stub tmp.gz > $@
	rm tmp.gz
	chmod ugo+x $@



tinytest: $(SDLTESTDEPS) tests/selfextr.stub Makefile
# hacking
#
#	$(CC) $(HCFLAGS) $(ARCHFLAGS) `sdl-config --cflags` -c -o sdltest.o sdltest.c 
#	$(CC) $(HCFLAGS) $(ARCHFLAGS) -c -o synti2.o synti2.c
#	$(CC) $(HCFLAGS) $(ARCHFLAGS) -c -o testdata.o testdata.c
#	$(CC) $(HCFLAGS) $(ARCHFLAGS) -c -o teh4k_gfx.o teh4k_gfx.c

#	$(CC) $(ARCHFLAGS) -o $@.unstripped.payload sdltest.o synti2.o testdata.o \
		teh4k_gfx.o \
	   -nostartfiles $(LIBS)

	$(CC) $(HCFLAGS) $(ARCHFLAGS) -o $@.unstripped.payload \
		-combine -fwhole-program -flto \
		-Iinclude \
		-DULTRASMALL -nostdlib  -nostartfiles -lc \
		$(filter %.c, $(SDLTESTDEPS)) \
		$(ARCHLIBS)

# Why do I get problems with these and -m32:
# `sdl-config --libs` -lm -lGL -lGLU


#	$(CC) $(ARCHFLAGS) -o $@.unstripped.payload sdltest.o synti2.o testdata.o \
#		teh4k_gfx.o \
#	   -nostartfiles `sdl-config --libs` -lm -lGL -lGLU

#	strip $(STRIPOPT) -o $@.payload $@.unstripped.payload
#	cat selfextr.stub > $@
#	gzip -n9 <$@.payload >> $@

# I got a 32-bit sstrip executable from Zipola :).  The zipped result
# got from 4.3k down to 3.8k by compiling with -m32 and linking
# against 32bit libraries, using 7zip, and stripping with sstrip
# instead of strip:
	cp $@.unstripped.payload $@.payload 

	$(ARCHSTRIP) $(ARCHSTRIPOPT) $@.payload

	7za a -tgzip -mx=9 tmp.gz $@.payload
	cat tests/selfextr.stub tmp.gz > $@
	rm tmp.gz

	chmod ugo+x $@


mostlyclean:
	-rm `find . -name "*~"` *.o

clean: mostlyclean
	-rm *.payload jsynti2 jvis sdltest tinytest tinyplayer \
		benchmark testsong.c misssify test.misss \
		midi2synti2 patchedit genpar cbank

veryveryclean: veryclean
	-rm include/synti2_params.h
