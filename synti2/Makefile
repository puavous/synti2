CC = gcc

#JACKLIBS = 

# I used to get a crash in PulseAudio due to misaligned stack (pa uses
# vector registers and those instructions require 16 byte alignment). 
# I hope my current init replacement will be enough...
HCFLAGS = -Os -DULTRASMALL -nostdlib

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

all:
	$(CC) -o testi jackmidi.c synti2.c `pkg-config --cflags --libs jack` -lm

sdltest: sdltest.c synti2.c Makefile

	$(CC) -o sdltest sdltest.c synti2.c \
	   `sdl-config --cflags --libs` -lm -lGL -lGLU

tinytest: sdltest.c synti2.c selfextr.stub Makefile
# hacking
	$(CC) $(HCFLAGS) `sdl-config --cflags` -c -o sdltest.o sdltest.c  \
	   
	$(CC) $(HCFLAGS) -c -o synti2.o synti2.c

	$(CC) -o $@.payload sdltest.o synti2.o  \
	   -nostartfiles `sdl-config --libs` -lm -lGL -lGLU

	strip $(STRIPOPT) $@.payload
	cat selfextr.stub > $@
	gzip -n9 <$@.payload >> $@
	chmod ugo+x $@


piippi:
	$(CC) -o piippitesti jackout.c `pkg-config --cflags --libs jack` -lm


clean:

	-rm *~

veryclean: clean
	-rm testi sdltest tinytest piippitesti
