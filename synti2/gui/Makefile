.PHONY: clean veryclean
LD=g++
MAIN=synti2gui

OBJS=ExeBuilderViewFl.o FeaturesViewFl.o \
  main.o MainWindow.o midihelper.o \
  MidiMap.o MidiMapperViewFl.o miditool.o \
  misss.o PatchBank.o Patch.o PatchesViewFl.o \
  ../engine/synti2_midi.o
LIBS=-lfltk

%.o: %.cxx
	g++ -Wall -g -c -o $@ $< -lfltk \
		-I../engine

synti2gui: $(OBJS) Makefile
	$(LD) 	`fltk-config --cxxflags`  \
		`pkg-config --cflags --libs jack` \
                $(LDFLAGS) $(OBJS) -o $(MAIN) $(LIBS)

clean:
	-rm *.o *~

veryclean: clean
	-rm synti2gui
