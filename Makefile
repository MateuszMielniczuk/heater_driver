doc:
	@-mkdir doxygen-out
	doxygen docs/src/doxygen.cfg > doxygen.log

release: clean doc
	cd doxygen-out/latex ; make
	mv doxygen-out/latex/refman.pdf docs/manual.pdf
	mv doxygen-out/html docs/manual
	make clean-temp

clean-temp:
	@rm -f doxygen.log
	@-find doxygen-out/ -delete 2>/dev/null

clean-examples:
	@cd examples; for i in * ; do \
		cd $$i ; \
		make -s clean ; \
		cd .. ; \
	done

clean: clean-temp clean-examples
	@-find docs/manual/ -delete 2>/dev/null
	@rm -f docs/manual.pdf
