# quick tiny test:

make tinyexe MAIN=tests/tinyplayer.c SND=patches/minimal.s2bank SMF=tests/minimal.mid

# Extremely small tiny test (with no sequencer even):
make tinyexe MAIN=tests/withoutseq.c SND=patches/miniFMset.s2bank SMF=tests/minimal.mid ADDFLAGS=-msse4 NONOS='-DNO_NOTHING -DEXTREME_NO_SEQUENCER'


make tinyexe MAIN=tests/sdltest.c SND=example/example1.s2bank SMF=example/example1.mid GFX=tests/tryout_gfx2.c NONOS=''
