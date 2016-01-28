
.PHONY: all clean doc doc-internal

all:

doc: $(wildcard *.c *.h)
	doxygen doc/Doxyfile
doc-internal: $(wildcard *.c *.h)
	doxygen doc/Doxyfile-internal

clean:
	$(RM) test *.o
	$(RM) -r doc/html
