.PHONY: all clean

# Choose a command that packs 'tmp' into 'tmp.gz':
#GZIP_TMP_CMD=gzip -9 tmp
#GZIP_TMP_CMD = zopfli --i25 tmp
#GZIP_TMP_CMD = zopfli --i75 tmp
#GZIP_TMP_CMD = zopfli --i250 tmp
#GZIP_TMP_CMD = zopfli --i800 tmp
GZIP_TMP_CMD = zopfli --i500 tmp
#GZIP_TMP_CMD = zopfli --i2000 tmp
#GZIP_TMP_CMD = zopfli --i3000 tmp


# Compile and pack current version:
current.run: i16_mindfulness.asm definitions_from_libs.asm general_macros.asm debug_macros.asm
	nasm -f bin -o $@.bin $<
	chmod +x $@.bin
	cp $@.bin tmp
	$(GZIP_TMP_CMD)
	cp selfextr.stub.unclean $@
	tac -rs. tmp.gz >> $@
	cp tmp.gz $@.bin.gz
	rm tmp.gz
	chmod ugo+x $@

	@echo End result:
	@ls -lt $@


clean:
	-rm *.bin a.out *.o tmp
	-rm *.bin.gz
	-rm *~
	-rm current.run


