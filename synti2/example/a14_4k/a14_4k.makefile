# Customization for a final product. Can override any switches. Must
# define at least CUSTOMFILES.

# Assembly 2014 4k entry:
# Specifics needed are song, sounds, and gfx:
A14FILES = example/a14_4k/a14_4k.mid \
	   example/a14_4k/a14_4k.s2bank \
	   example/a14_4k/vertex.vert \
	   example/a14_4k/fragment.frag \
	   example/a14_4k/render.c \
	   example/a14_4k/glfuncs.c

CUSTOMFILES=$(A14FILES)

CUSTOM_FLAGS=-DSCREEN_HEIGHT=80 -DPLAYBACK_DURATION=35

#GZIP_TMP_CMD = zopfli --i1000 tmp

#Assembly compo rules were 32bit exe with forced resolution:
#CUSTOM_FLAGS=-DSCREEN_HEIGHT=720 -DFULLSCREEN -DPLAYBACK_DURATION=35
