CC = gcc

#JACKLIBS = 

# C flags common to all files (normal/debug build)
CFLAGS = -O3 -g -Wall -pedantic

# For normal 64-bit build:
#ARCHFLAGS = 
#LIBS = `sdl-config --libs` -lm -lGL -lGLU

# For linking against 32-bit libraries (approx. 5% smaller packed exe):
ARCHFLAGS = -m32 -DNO_I64
LIBS = `sdl-config --libs` -lm /usr/lib/libGL.so.1 /usr/lib/libGLU.so.1

# The HCFLAGS (as in "hardcore") are for making a very small executable.
# I used to get a crash in PulseAudio due to misaligned stack (pa uses
# vector registers and those instructions require 16 byte alignment). 
# I hope my current init replacement will be enough...
HCFLAGS = -Os -g -Wall -pedantic -DULTRASMALL   -DNO_LOOPING_ENVELOPES \
	-DNO_SYSEX_RECEIVE -DNO_EXTRA_SYSEX -nostdlib



# Used the following the last time.. but now they don't seem to help much
# at least with the first versions..

# -fno-strict-aliasing \
#          -nostdlib -nostartfiles -DULTRASMALL \
#          -ffast-math -fomit-frame-pointer \
#	  -fpredictive-commoning \
#          --param max-unroll-times=0 \
#          --param max-unrolled-insns=0 \
#          -msse4 -mavx -mhard-float 

STRIPOPT = -s -R .comment \
	-R .gnu.version \
	-R .note.gnu.build-id -R .gnu.hash \
	-R .eh_frame_hdr \
	-R .eh_frame 

all: jacktest

tools: misssify jmiditrans patchedit

misssify: misssify.c Makefile
	$(CC) $(CFLAGS) -Wextra -o misssify misssify.c -lm

jmiditrans: jmiditrans.c Makefile
	$(CC) $(CFLAGS) -Wextra -o jmiditrans jmiditrans.c \
		`pkg-config --cflags --libs jack`

patchedit: patchedit.cxx patchtool.hpp patchtool.cxx Makefile
	g++ `fltk-config --cxxflags` -o patchedit \
		patchedit.cxx patchtool.cxx -lfltk \
		`pkg-config --cflags --libs jack`


benchmark: benchmark.c synti2.c synti2.h testdata.c Makefile
	$(CC) $(CFLAGS) -o benchmark benchmark.c synti2.c testdata.c -lm

jacktest: jackmidi.c synti2.c synti2.h testdata.c Makefile
	$(CC) $(CFLAGS) -DJACK_MIDI -o testi jackmidi.c synti2.c testdata.c \
		`pkg-config --cflags --libs jack` -lm

testdata.c: patchbank.txt testi.mid hacktac.py misssify Makefile
	python hacktac.py < patchbank.txt > testdata.c
	./misssify --tpq 12 --velocity-all 100 --only-on testi.mid
#	./misssify --tpq 12 --velocity-all 100 --only-on testi.mid
#	./misssify --tpq 12 --only-on testi.mid
#	./misssify --tpq 12 testi.mid
#	./misssify testi.mid
	python file2c.py test.misss hacksong >> testdata.c

sdltest: sdltest.c synti2.c synti2.h Makefile

	$(CC) $(CFLAGS) $(ARCHFLAGS) -o sdltest sdltest.c synti2.c \
	   `sdl-config --cflags --libs` -lm -lGL -lGLU

tinytest: sdltest.c synti2.c synti2.h testdata.c selfextr.stub teh4k_gfx.c Makefile
# hacking
	$(CC) $(HCFLAGS) $(ARCHFLAGS) `sdl-config --cflags` -c -o sdltest.o sdltest.c 
	$(CC) $(HCFLAGS) $(ARCHFLAGS) -c -o synti2.o synti2.c
	$(CC) $(HCFLAGS) $(ARCHFLAGS) -c -o testdata.o testdata.c
	$(CC) $(HCFLAGS) $(ARCHFLAGS) -c -o teh4k_gfx.o teh4k_gfx.c

	$(CC) $(ARCHFLAGS) -o $@.unstripped.payload sdltest.o synti2.o testdata.o \
		teh4k_gfx.o \
	   -nostartfiles $(LIBS)

#	$(CC) $(ARCHFLAGS) -o $@.unstripped.payload sdltest.o synti2.o testdata.o \
#		teh4k_gfx.o \
	   -nostartfiles `sdl-config --libs` -lm -lGL -lGLU

#	strip $(STRIPOPT) -o $@.payload $@.unstripped.payload
#	cat selfextr.stub > $@
#	gzip -n9 <$@.payload >> $@

# I got a 32-bit sstrip executable from Zipola :).  The zipped result
# got from 4.3k down to 3.8k by compiling with -m32 and linking
# against 32bit libraries, using 7zip, and stripping with sstrip
# instead of strip:
	cp $@.unstripped.payload $@.payload 
	sstrip $@.payload

	7za a -tgzip -mx=9 tmp.gz $@.payload
	cat selfextr.stub tmp.gz > $@
	rm tmp.gz

	chmod ugo+x $@


clean:

	-rm *~ *.o

veryclean: clean
	-rm *.payload testi sdltest tinytest piippitesti \
		benchmark testdata.c misssify test.misss \
		jmiditrans patchedit
