
# utf8fuzz

utf8fuzz is a program to test UTF-8 validation. I developed it to help me as I was
writing my AfUtf8 validator. It has two core functions:

* Measuring the time taken to validate UTF-8 strings, with the help of [FFTW's cycle.h](http://www.fftw.org/cycle.h)
* Ensuring that all validation implementations return the same result (accept or reject) for a particular UTF-8 string.

It can operate on three types of data:

* Raw data from existing files.
* A set of small UTF-8 test strings (including most of [Markus Kuhn's tests](http://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt).)
* Randomly generated UTF-8, which may be clean UTF-8, or may contain high-level introduced errors (eg overlong sequences, characters which aren't permitted in UTF-8) and/or random byte mutations. There are 9 types of anomalies, each of which have a 2% chance of occuring once in any generated string (and a 0.04% chance of occurring twice, etc). This means ~83% of generated strings should be valid UTF-8. We are biased towards introducing errors near the beginning and end of strings.

There are a few glaring issues:

* Only tested on Mac OS X with clang++ in 64-bit mode.
* We don't insert errors into very short strings (under about 40 codepoints)
* The error introduction code currently can't produce adjacent errors in any particular order, and in any event is unlikely to place errors next to each other at all. This would expose a lot more boundary conditions in the code under test!
* The code is messy :)


## License and disclaimer

Although the license already covers this, a statement to remove any doubt: if you really, truly need to know that a UTF-8 implementation contains no security vulnerabilities and will always return a correct result, you should find a consultant with indemnity insurance to tell you so. This software may contain errors!

The core test framework and UTF-8 generation are licensed under the GPL2. See LICENSE.txt for more details.

Various other items in contrib/ and codecs/, including other UTF-8 implementations, have other licenses. See the individual files for details. Notably, my own code in codecs/AfUtf8/ has a 2-clause BSD license.

It probably isn't legal to distribute a built binary of this software.

## Usage

Note that you'll want to build with OPTFLAGS="-DBENCHMARK_BUILD" for benchmarking purposes, and probably without for other purposes.

If you care about performance, you may want to tweak the optimisation and CPU flags passed to the compiler during the build.

        USAGE: ./utf8fuzz [<options>] [<filenames...>]
        
        Options:
          -k   Continue to run tests even after an error is encountered.
          -b   Include the C library mbs* function and GNU iconv validators.
               They accept not-quite-valid UTF-8 that the rest of the codecs
               reject. So use with -k!
          -c x Use only one codec. See possibilities below.
          -m   Vary memory alignment for testing. Consider using MallocGuardEdges=1.
          -M a Consistent memory alignment of <a> bytes for testing. Prefix a with
               a '-' to align to the end of a page (eg '-0' to be flush up against
               the end.
          -v   Verbose (-vv, -vvv etc for more verbose).
          -s   Run some hardcoded tests.
          -r   Randomly generate test data (valid and invalid UTF-8) - 300 samples.
          -R n As with -r, but generate n samples.
          -S n Use n as initial random seed for all randomness (default = 1).
          -V   Randomly generate only valid UTF-8.
          -l m Minimum number of codepoints to include in a randomly generated sample.
          -L m Maximum number of codepoints to include in a randomly generated sample.
          -t   Run benchmarks, print CSV when finished.
          -h   Display this help.
        
        If you supply filenames, then those files will be read in and each
        processed individually. If you do not use -s or supply filenames then
        test data will be randomly generated.
        
        Included validators (specify with -c):
           verbose vector stangvik postgresql u8u16 iconv mbstowcs 
        
## Examples

### Build with verbosity enabled, run basic test

    make

### Get help

    ./utf8fuzz -h

### Produce CSV timings from an optimised build

    OPTFLAGS="-DBENCHMARK_BUILD" make clean utf8fuzz; ./utf8fuzz -r -t > test.csv

### Test that the "vector" UTF-8 implementation handles >4gig of data

    ./utf8fuzz -R 1 -l 2300000000 -L 2300000000 -c vector -vv


## UTF-8 implementations under test

### What is UTF-8?

See [RFC 3629](http://tools.ietf.org/html/rfc3629).

In particular, we don't consider [derivatives like CESU-8 or Modified UTF-8](http://en.wikipedia.org/wiki/UTF-8#Derivatives) to be valid UTF-8.

### Included

These are working validators:

* *vector* is my AfUtf8 UTF-8 validator, implemented using SSE4.1 intrinsics.
* *verbose* is a particularly inefficient UTF-8 validator used to test the core ideas in, and sharing code with, the *vector* validator.
* *stangvik* is the UTF-8 validator from Einar Otto Stangvik's [ws](https://github.com/einaros/ws) WebSocket implementation.
* *postgresql* is the UTF-8 validator from the [PostgreSQL]("http://www.postgresql.org/") database, tweaked to accept inline zero bytes (which are legal UTF-8, but not in string values in PostgreSQL).

These are working converters, meaning that we use them to convert from UTF-8 to another encoding and see if an error is raised. It's not really fair to compare them to a validator, performance-wise, because they are doing more work!

* *u8u16* is Prof. Rob Cameron's [vectorised UTF-8 to UTF-16 converter](http://u8u16.costar.sfu.ca/).

These are broken converters, and disabled by default (specifically, they accept things legal in [UTF-8 variants](http://en.wikipedia.org/wiki/UTF-8#Derivatives) which are not, strictly speaking, valid UTF-8):

* *iconv* is the GNU iconv library as provided by the native operating system.
* *mbstowcs* is the C library mb* converter functions as provided by the native operating system. (Mac OS X's are basically pulled straight from FreeBSD).


### Not included

These are UTF-8 validators / converters which I've not yet taken the effort to include, for various reasons enumerated below (basically, I took a quick look and decided they are unlikely to be very fast):

* Firefox's UTF8 -> UTF16 is in http://mxr.mozilla.org/mozilla-central/source/intl/uconv/src/nsUTF8ToUnicode.cpp and http://mxr.mozilla.org/mozilla-central/source/intl/uconv/src/nsUTF8ToUnicodeSSE2.cpp. Discussion at https://bugzilla.mozilla.org/show_bug.cgi?id=506430 doesn't indicate a speed win over u8u16. It isn't fully vectorised but uses SSE2 for unpacking ASCII runs.

* SpiderMonkey has its own js_InflateUTF8StringToBuffer function (in jsstr.cpp); it's not vectorised, so I'm assuming not particularly fast.

*  v8's unicode.cc has UTF-8 parsing which isn't vectorised.

* ICU (which is used by lots of things, including WebKit) has UTF-8 code which isn't vectorised but otherwise appears to have been heavily optimised.

* WebKit has non-vectorised UTF-8 code in WTF/wtf/unicode, different non-vectorised UTF-8 code in Source/WebCore/platform/text, and also uses ICU!

* uclibc has a non-vectorised mbsnrtowcs implementation

* glibc has optimised but unvectorised utf-8 code in iconv/gconv_simple.c which is ultimately invoked by the glibc iconv* and mb* functions.


### Adding support for another validator

In validator.cpp, create a new subclass of Validator which calls the validator of interest. Then add it to Validator::createAll at the bottom of that file.

The interface is pretty simple and there are lots of examples in that file. It's particularly important to keep setup and cleanup code which shouldn't be included in benchmark timings out of the validate() function.

Validator subclasses which return "true" rather than "false" in response to ours() calls are reported slightly differently. If you're developing a validator, it will be useful to have it, and only it, return true.

