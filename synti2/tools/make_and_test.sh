# quick tiny test:

make tinyexe MAIN=tests/tinyplayer.c SND=patches/minimal.s2bank SMF=tests/minimal.mid

# Extremely small tiny test (with no sequencer even):

make tinyexe MAIN=tests/withoutseq.c SND=patches/minimal.s2bank SMF=tests/minimal.mid ADDFLAGS=-msse4 NONOS='-DNO_NOTHING -DDRYRUN -DEXTREME_NO_SEQUENCER'

make patchedit && make jsynti2 && killall jsynti2 ; killall patchedit ; ./jsynti2 & sleep 2 && ./patchedit & sleep 4 &&  aj-snapshot -r tools/s2devel.ajsnapshot 

make patchedit && make jsynti2 && make jmiditrans && killall jsynti2 ; killall patchedit ; ./jsynti2 & sleep 2 && ./patchedit & sleep 3 &&  aj-snapshot -r tools/s2devel.ajsnapshot

make tinyexe MAIN=tests/sdltest.c SND=example/example1.s2bank SMF=example/example1.mid GFX=tests/tryout_gfx2.c NONOS=''
