
# AfUtf8

AfUtf8 is a vectorised UTF-8 validator. It requires SSE 4.1 and, to date, has only
been compiled on Mac OS X using clang++. It is a work in progress.

It validates UTF-8 as defined by [RFC 3629](http://tools.ietf.org/html/rfc3629). 
Character sequences like UTF-16 surrogate characters, overlong zero bytes encoded
as two bytes, and codepoints higher than U+10FFFF are _not_ permitted.

Valid UTF-8 strings may contain embedded zero bytes / null characters.

## Using AfUtf8 from a C / C++ program

Build and link AfUtf8.cpp into your project, #include AfUtf8.h, and call this function:

        /**
         * Returns 1 if the len bytes starting at ptr are valid UTF-8, 0 otherwise.
         *
         * If len == 0 then 1 is always returned (an empty string is valid
         * UTF-8!) and ptr is ignored.
         */
        int AfUtf8_check(const size_t len, const char* ptr);


## Command line tool

The Makefile will build *aftest*, a simple command line program which accepts
filenames on the command line and prints out whether they are valid UTF-8 or
not.

[utf8fuzz](https://github.com/andrewffff/utf8fuzz) is a much better test harness.


## Developing / debugging

AfUtf8_check_prototype has the same interface as AfUtf8_check; it's a plain-C
non-vectorised implementation of the core ideas. If you compile with -DAFUTF8_VERBOSE=3
then the command line tool will invoke this function, and it'll dump a load of 
debugging info to stdout.


## License

Copyright (c) 2012, Grumpy Panda Pty Ltd as trustee for The AJF Family Trust.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
