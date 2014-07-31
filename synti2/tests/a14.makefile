# FIXME: Should define entries elsewhere, not in a common Makefile (?)
# Assembly 2014 4k entry:
# Specifics needed are song, sounds, and gfx:
A14FILES = example/a14_4k/a14_4k.mid \
	   example/a14_4k/a14_4k.s2bank \
	   example/a14_4k/render.c \
	   example/a14_4k/vertex.vert \
	   example/a14_4k/fragment.frag \
	   example/glfuncs.c

CUSTOMFILES=$(A14FILES)
