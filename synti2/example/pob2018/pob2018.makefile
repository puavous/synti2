# Customization for a final product. Can override any switches. Must
# define at least CUSTOMFILES.

# Portrait of Beyond cabin trip 2018, and some synth development...:
# Specifics needed are song, sounds, and gfx (and other stuff as of now):
FILES = example/pob2018/song.mid \
	   example/pob2018/patches.s2bank \
	   example/pob2018/vertex.vert \
	   example/pob2018/fragment.frag \
	   example/pob2018/render.c \
	   example/pob2018/glfuncs.h \
	   example/pob2018/glfuncs.c \
	   example/pob2018/info.txt

CUSTOMFILES=$(FILES)

CUSTOM_FLAGS=-DSCREEN_HEIGHT=360 -DPLAYBACK_DURATION=30 -DAUDIO_BUFFER_SIZE=4096

# Use if the compo machine can autodetect screen size:
#CUSTOM_FLAGS=-DFULLSCREEN -DSCREEN_AUTODETECT -DPLAYBACK_DURATION=30 -DAUDIO_BUFFER_SIZE=4096

## This gcc flag will use tinyexe.ld as the link script (..dangerous?):
#		-Wl,-dTtinyexe.ld \

## Uncomment to test without non-standard software dependencies:
#GZIP_TMP_CMD = gzip -9 tmp
#SHADER_TMP_CMD = sed -f src/shader_preview.sed > shader.tmp
#SSTRIP=echo 'Not using sstrip!'

# Final version benefits from zopfli compression:
#GZIP_TMP_CMD = zopfli --i1000 tmp

#Assembly compo rules were 32bit exe with forced resolution:
#CUSTOM_FLAGS=-DSCREEN_HEIGHT=720 -DFULLSCREEN -DPLAYBACK_DURATION=35
#ARCHFLAGS = -m32 -DNO_I64 `sdl-config --cflags`
#ARCHLIBS = `sdl-config --libs` -lm /usr/lib/libGL.so.1
