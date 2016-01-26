PREFIX = /usr/local

CFLAGS = -std=c11 -g -Wall -Werror
CFLAGS += -O2 -DNDEBUG -march=native -mtune=native -fomit-frame-pointer

AMALG = gourgandine.h gourgandine.c

#--------------------------------------
# Abstract targets
#--------------------------------------

all: $(AMALG) gourgandine example

clean:
	rm -f gourgandine example test/gourgandine.so vgcore* core

check: test/gourgandine.so
	cd test && valgrind --leak-check=full --error-exitcode=1 lua test.lua

install: gourgandine
	install -spm 0755 $< $(PREFIX)/bin/gourgandine

uninstall:
	rm -f $(PREFIX)/bin/gourgandine

.PHONY: all clean check install uninstall


#--------------------------------------
# Concrete targets
#--------------------------------------

LIBS = src/lib/mascara.c src/lib/utf8proc.c

cmd/%.ih: cmd/%.txt
	cmd/mkcstring.py < $< > $@

gourgandine.h: src/api.h
	cp $< $@

gourgandine.c: $(wildcard src/*.[hc]) $(wildcard src/lib/*.[hc])
	src/mkamalg.py src/*.c > $@

gourgandine: $(wildcard cmd/*.[hc]) cmd/gourgandine.ih $(AMALG)
	$(CC) $(CFLAGS) gourgandine.c cmd/*.c $(LIBS) -o $@

example: example.c $(AMALG)
	$(CC) -Isrc/lib $(CFLAGS) $(LDLIBS) $< gourgandine.c $(LIBS) -o $@

test/gourgandine.so: test/gourgandine.c $(AMALG)
	$(CC) $(CFLAGS) -fPIC -shared $< gourgandine.c $(LIBS) -o $@
