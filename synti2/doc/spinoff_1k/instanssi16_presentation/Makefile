.PHONY: all clean

all: i16example.run

disa: i16example.disass

# It is clear that zopfli packs much better than gzip. Adding iterations
# usually shave off a couple of more bytes, but it takes many seconds:

#GZIP_TMP_CMD=gzip -n -9 tmp
GZIP_TMP_CMD = zopfli --i25 tmp
#GZIP_TMP_CMD = zopfli --i75 tmp
#GZIP_TMP_CMD = zopfli --i250 tmp
#GZIP_TMP_CMD = zopfli --i500 tmp
#GZIP_TMP_CMD = zopfli --i2000 tmp
#GZIP_TMP_CMD = zopfli --i3000 tmp

# Experiment with run script:
i16example.run: i16example.bin
	cp i16example.bin tmp
	$(GZIP_TMP_CMD)
	cp selfextr.stub $@
	tac -rs. tmp.gz >> $@
	cp tmp.gz $@.payload.gz
	rm tmp.gz
	chmod ugo+x $@

	@echo End result:
	@ls -lt $@

i16example.bin: i16example.asm \
		debug_macros.asm \
		general_macros.asm \
		definitions_from_libs.asm
	nasm -f bin -o $@ $<
	chmod +x $@

i16example.disass: i16example.asm \
		debug_macros.asm \
		general_macros.asm \
		definitions_from_libs.asm
	nasm -f elf64 -D DISASS -o tmp.o $<
	objdump -d tmp.o > $@

clean:
	-rm *.run
	-rm sizetest*.bin.gz
	-rm *.bin a.out *.o tmp
	-rm *~

