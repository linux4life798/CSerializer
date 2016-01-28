CFLAGS += -g -O0
LDLAGS += -g -O0

.PHONY: all clean doc doc-internal

all: serializer.o

doc: $(wildcard *.c *.h)
	doxygen doc/Doxyfile
doc-internal: $(wildcard *.c *.h)
	doxygen doc/Doxyfile-internal

clean:
	$(RM) *.o
	$(RM) -r doc/html
