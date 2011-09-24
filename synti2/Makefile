#JACKLIBS = 


all:
	$(CC) -o testi jackmidi.c synti2.c `pkg-config --cflags --libs jack` -lm

sdltest: sdltest.c synti2.c

	$(CC) -o sdltest sdltest.c synti2.c  `sdl-config --cflags --libs` -lm

piippi:
	$(CC) -o piippitesti jackout.c `pkg-config --cflags --libs jack` -lm


clean:

	-rm *~

veryclean: clean
	-rm testi sdltest piippitesti
