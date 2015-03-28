# Customization for a final product. Can override any switches. Must
# define at least CUSTOMFILES.

# Assembly 2014 4k entry:
# Specifics needed are song, sounds, and gfx:
A14FILES = example/i14_4k/i14_4k.mid \
	   example/i14_4k/i14_4k.s2bank \
	   example/i15_4k/vertex.vert \
	   example/i15_4k/fragment.frag \
	   example/i15_4k/render.c \
	   example/i15_4k/glfuncs.h \
	   example/i15_4k/glfuncs.c

CUSTOMFILES=$(A14FILES)

CUSTOM_FLAGS=-DSCREEN_HEIGHT=320 -DPLAYBACK_DURATION=35

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
