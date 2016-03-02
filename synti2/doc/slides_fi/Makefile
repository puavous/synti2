all:
	pdflatex instanssi2012_slides_nieminen.tex
	pdflatex instanssi2012_slides_nieminen.tex

	pdflatex jtt2015_slides_nieminen.tex
	bibtex jtt2015_slides_nieminen
	pdflatex jtt2015_slides_nieminen.tex
	pdflatex jtt2015_slides_nieminen.tex

	pdflatex jtt2016_slides_nieminen.tex
	bibtex jtt2016_slides_nieminen
	pdflatex jtt2016_slides_nieminen.tex
	pdflatex jtt2016_slides_nieminen.tex


mostlyclean:
	-rm *.{aux,log,nav,out,snm,toc,bbl,blg} *~

clean: mostlyclean
	-rm instanssi2012_slides_nieminen.pdf
	-rm jtt2015_slides_nieminen.pdf
	-rm jtt2016_slides_nieminen.pdf
