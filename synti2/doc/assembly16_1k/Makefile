.PHONY: all clean devel compo

devel: fuzzdealer.i386.run

compo: fuzzdealer.i386.compoversion.1920x1080fs.run

# Requires:
#    NASM assembler for all builds;
#    zopfli packer for building the compo version.
#    i386 architecture, libSDL2 and libOpenGL for running any version.

# Uncomment a suitable command for procucing ./tmp.gz from ./tmp

# The Assembly Summer 2016 final version used zopfli with 32000
# iterations. It took some 3 minutes to pack on my computer. With 75
# iterations, packing is almost instantaneous, but it leaves some two
# bytes too much in the zip. If you don't have zopfli installed, the
# standard "gzip" is likely to be installed on your usual Linux
# distro, but it doesn't pack very well...

GZIP_TMP_CMD=gzip -9 tmp
#GZIP_TMP_CMD = zopfli --i25 tmp
#GZIP_TMP_CMD = zopfli --i75 tmp
#GZIP_TMP_CMD = zopfli --i250 tmp
#GZIP_TMP_CMD = zopfli --i800 tmp
#GZIP_TMP_CMD = zopfli --i500 tmp
#GZIP_TMP_CMD = zopfli --i3000 tmp
#GZIP_TMP_CMD = zopfli --i32000 tmp

fuzzdealer.i386.run: a16_fuzzdealer.asm Makefile selfextr.stub.unclean
	nasm -f bin -o tmp $<
	$(GZIP_TMP_CMD)
	cp selfextr.stub.unclean $@
	tac -rs. tmp.gz >> $@
	cp tmp.gz fuzzdealer.bin.gz
	rm tmp.gz
	chmod ugo+x $@

	@echo End result:
	@ls -lt $@

fuzzdealer.i386.compoversion.1920x1080fs.run: a16_fuzzdealer.asm Makefile selfextr.stub.unclean
	nasm -f bin -DASSEMBLY_FINAL -o tmp $<
	zopfli --i32000 tmp
	cp selfextr.stub.filthy $@
	tac -rs. tmp.gz >> $@
	cp tmp.gz fuzzdealer.bin.gz
	rm tmp.gz
	chmod ugo+x $@

	@echo End result:
	@ls -lt $@

# The "development" version builds some useful debug listings.
# The tools defdb and gzthermal tools are very useful, but
# you need to install them in a suitable place and change the
# command paths here (they'll be found with google:)):
fuzzdealer.x86.devel.bin: a16_fuzzdealer.asm
	nasm -f bin -o tmp -l $<.nasmlisting $<
	nasm -DDISASS -f elf -o tmp.o $<
	objdump -S tmp.o > $<.disassembly
	$(GZIP_TMP_CMD)
	cp selfextr.stub.filthy $@
	tac -rs. tmp.gz >> $@
	cp tmp.gz $@.gz
	rm tmp.gz
	chmod ugo+x $@

	@echo End result:
	@ls -lt $@

	~/files/hacking/defdb_04b/defdb $@.gz
	~/files/hacking/gzthermal_04c/gzthermal -b -n $@.gz
	cp gzthermal-result.png gzthermal-result-binary.png
	~/files/hacking/gzthermal_04c/gzthermal -n $@.gz

clean:
	-rm *.bin.gz
	-rm *.bin *.run a.out *.o tmp
	-rm *~
