.PHONY: all mostlyclean clean

all: instanssi2016_nieminen_spinoff_1k.pdf \
	instanssi2016_nieminen_spinoff_1k.pdf \
	jtt2015_slides_nieminen.pdf \
	jtt2016_slides_nieminen.pdf

instanssi2016_nieminen_spinoff_1k.pdf: instanssi2016_nieminen_spinoff_1k.tex
	pdflatex instanssi2016_nieminen_spinoff_1k.tex
	pdflatex instanssi2016_nieminen_spinoff_1k.tex

instanssi2012_slides_nieminen.pdf: instanssi2012_slides_nieminen.tex
	pdflatex instanssi2012_slides_nieminen.tex
	pdflatex instanssi2012_slides_nieminen.tex

jtt2015_slides_nieminen.pdf: jtt2015_slides_nieminen.tex dspstuff.bib
	pdflatex jtt2015_slides_nieminen.tex
	bibtex jtt2015_slides_nieminen
	pdflatex jtt2015_slides_nieminen.tex
	pdflatex jtt2015_slides_nieminen.tex

jtt2016_slides_nieminen.pdf: jtt2015_slides_nieminen.tex dspstuff.bib
	pdflatex jtt2016_slides_nieminen.tex
	bibtex jtt2016_slides_nieminen
	pdflatex jtt2016_slides_nieminen.tex
	pdflatex jtt2016_slides_nieminen.tex


mostlyclean:
	-rm *.{aux,log,nav,out,snm,toc,bbl,blg} *~

clean: mostlyclean
	-rm instanssi2016_nieminen_spinoff_1k.pdf
	-rm instanssi2012_slides_nieminen.pdf
	-rm jtt2015_slides_nieminen.pdf
	-rm jtt2016_slides_nieminen.pdf
