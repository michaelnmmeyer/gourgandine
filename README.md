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

## Implementation

To extract acronyms, we scan the input text, looking for the pattern
`<expansion> (<acronym>)`, or, alternatively, `<acronym> (<expansion>)`. A
number of preconditions must be met for a token to be considered a potential
acronym, all of which are explained in the source code.

Once a potential acronym definition is found, we normalize it to NFKC Unicode,
fold it to lowercase, remove non-alphabetic characters, and drop diacritics. We
then attempt to match the acronym against its expansion. For the two to be
considered matching, the first letter of the expansion must match that of the
acronym, and the remaining letters of the acronym must either occur at the
beginning of a token of the expansion, where a token is a consecutive sequence
of alphabetic characters, or inside a token which first letter is matching.

## References

This implementation draws from the following papers:
* [Adar (2002), S-RAD: A Simple and Robust Abbreviation
  Dictionary](http://psb.stanford.edu/psb-online/proceedings/psb03/schwartz.pdf)
* [Schwartz and Hearst (2003), A Simple Algorithm for Identifying Abbreviation
  Definitions in Biomedical
  Text](http://psb.stanford.edu/psb-online/proceedings/psb03/schwartz.pdf)
* [Gooch (2012), BADREX: In situ expansion and coreference of biomedical
  abbreviations using dynamic regular expressions](http://arxiv.org/pdf/1206.4522.pdf)

For matching abbreviations against their expansion, I originally used the method
of Schwartz and Hearst. It turned out to be too permissive, leading to a lot of
false positives, so I devised another stricter matching method.
