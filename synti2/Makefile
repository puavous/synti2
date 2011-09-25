CC = gcc

#JACKLIBS = 

# Size optimization breaks audio!! Probably a bug in my code..
HCFLAGS = 

# -fno-strict-aliasing \
#          -nostdlib -nostartfiles -DULTRASMALL \
#          -ffast-math -fomit-frame-pointer \
#	  -fpredictive-commoning \
#          --param max-unroll-times=0 \
#          --param max-unrolled-insns=0 \
#          -msse4 -mavx -mhard-float 

STRIPOPT = -s -R .comment -R .gnu.version -R .note.gnu.build-id -R .gnu.hash


all:
	$(CC) -o testi jackmidi.c synti2.c `pkg-config --cflags --libs jack` -lm

sdltest: sdltest.c synti2.c Makefile

	$(CC) -o sdltest sdltest.c synti2.c \
	   `sdl-config --cflags --libs` -lm -lGL -lGLU

tinytest: sdltest.c synti2.c selfextr.stub Makefile

	$(CC) $(HCFLAGS) -o $@.payload sdltest.c synti2.c  \
	   `sdl-config --cflags --libs` -lm -lGL -lGLU

#	strip $(STRIPOPT) $@.payload
	cat selfextr.stub > $@
	gzip -n9 <$@.payload >> $@
	chmod ugo+x $@


piippi:
	$(CC) -o piippitesti jackout.c `pkg-config --cflags --libs jack` -lm


clean:

	-rm *~

veryclean: clean
	-rm testi sdltest piippitesti
