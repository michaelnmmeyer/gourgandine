PREFIX = /usr/local

CFLAGS = -std=c11 -g -Wall -Werror
CFLAGS += -O2 -DNDEBUG -march=native -mtune=native -fomit-frame-pointer

AMALG = gourgandine.h gourgandine.c

#--------------------------------------
# Abstract targets
#--------------------------------------

all: $(AMALG) gourgandine

clean:
	rm -f gourgandine vgcore* core

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

LIBS = src/lib/mascara.o src/lib/utf8proc.o

cmd/%.ih: cmd/%.txt
	cmd/mkcstring.py < $< > $@

src/lib/%.o: src/lib/%.c
	$(CC) $(CFLAGS) -c $< -o $@

gourgandine.h: src/api.h
	cp $< $@

gourgandine.c: $(wildcard src/*.[hc] src/lib/*.[hc])
	src/mkamalg.py src/*.c > $@

gourgandine: $(AMALG) $(wildcard cmd/*.[hc]) $(LIBS) cmd/gourgandine.ih
	$(CC) $(CFLAGS) gourgandine.c cmd/*.c $(LIBS) -o $@

test/gourgandine.so: test/gourgandine.c $(AMALG) src/lib/mascara.c src/lib/utf8proc.c
	$(CC) $(CFLAGS) -fPIC -shared $< gourgandine.c src/lib/mascara.c src/lib/utf8proc.c -o $@
