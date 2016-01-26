# gourgandine

An acronym extractor.

## Purpose

This is a library and command-line tool for extracting acronym definitions from
written texts.

## Building

The library is available in source form, as an amalgamation. Compile
`gourgandine.c` together with your source code, and use the interface described
in `gourgandine.h`. A C11 compiler is required for compilation, which means
either GCC or CLang on Unix. You'll also need to link the compiled code to
[`utf8proc`](https://github.com/JuliaLang/utf8proc).

A command-line tool `gourgandine` is also included. To compile and install it:

    $ make && sudo make install


## Usage

There is a concrete usage example in `example.c`. Compile this file with `make`,
and use the output binary like so:

    $ ./example "In 1985, Barnard founded the Physicians Committee for Responsible Medicine (PCRM)."
    PCRM	Physicians Committee for Responsible Medicine

Full details are given in `gourgandine.h`.
