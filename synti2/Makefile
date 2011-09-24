#JACKLIBS = 


all:
	$(CC) -o testi jackmidi.c synti2.c `pkg-config --cflags --libs jack` -lm

piippi:
	$(CC) -o piippitesti jackout.c `pkg-config --cflags --libs jack` -lm
