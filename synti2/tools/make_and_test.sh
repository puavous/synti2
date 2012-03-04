# quick tiny test:

make tinyexe MAIN=tests/tinyplayer.c SND=patches/minimal.s2bank SMF=tests/minimal.mid

make patchedit && make jsynti2 && killall jsynti2 ; killall patchedit ; ./jsynti2 & sleep 2 && ./patchedit & sleep 4 &&  aj-snapshot -r tools/s2devel.ajsnapshot 

make patchedit && make jsynti2 && make jmiditrans && killall jsynti2 ; killall patchedit ; ./jsynti2 & sleep 2 && ./patchedit & sleep 3 &&  aj-snapshot -r tools/s2devel.ajsnapshot
