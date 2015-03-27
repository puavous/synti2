# Customization for a final product. Can override any switches. Must
# define at least CUSTOMFILES.

# Specifics needed are song, sounds, and gfx:
A14FILES = example/jtt2015/jtt2015.mid \
	   example/jtt2015/jtt2015.s2bank \
	   example/jtt2015/vertex.vert \
	   example/shader_test/fragment.frag \
	   example/jtt2015/render.c \
	   example/jtt2015/glfuncs.c \
	   example/jtt2015/glfuncs.h

CUSTOMFILES=$(A14FILES)

CUSTOM_FLAGS=-DSCREEN_HEIGHT=80 -DPLAYBACK_DURATION=35

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
