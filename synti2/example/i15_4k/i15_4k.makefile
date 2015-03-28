# Customization for a final product. Can override any switches. Must
# define at least CUSTOMFILES.

# Instanssi 2015 4k entry:
# Specifics needed are song, sounds, and gfx (and other stuff as of now):
FILES = example/i15_4k/song.mid \
	   example/i15_4k/patches.s2bank \
	   example/i15_4k/vertex.vert \
	   example/i15_4k/fragment.frag \
	   example/i15_4k/render.c \
	   example/i15_4k/glfuncs.h \
	   example/i15_4k/glfuncs.c \
	   example/i15_4k/info.txt

CUSTOMFILES=$(FILES)

CUSTOM_FLAGS=-DSCREEN_HEIGHT=160 -DPLAYBACK_DURATION=35
# Instanssi 2015 Linux compo machine maybe can autodetect screen size:
#CUSTOM_FLAGS=-DFULLSCREEN -DSCREEN_AUTODETECT -DPLAYBACK_DURATION=8

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
