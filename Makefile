
default: test

all: utf8fuzz

test: utf8fuzz runtest

runtest:
	./utf8fuzz -s -vv

OPTFLAGS += -O2

CXX = clang++
CXXFLAGS_NO_WERROR = $(OPTFLAGS) -g -Wall -Wno-gnu-array-member-paren-init -mavx2
CXXFLAGS = $(CXXFLAGS_NO_WERROR) -Werror
CC = clang
CFLAGS_NO_WERROR = $(OPTFLAGS) -g -Wall -mavx2
CFLAGS = $(CFLAGS_NO_WERROR) -Werror
LDFLAGS = -g

OBJS = utf8fuzz.o validator.o verbosity.o aligned_alloc.o stangvik.o postgresql.o u8u16.o pg_wchar.o af_with_verbose_hack.o testset_standard.o testset_random.o testset_files.o z_validate_avx2.o z_validate_sse4.o zwegner.o lemire.o

utf8fuzz: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS)

utf8fuzz.o: utf8fuzz.cpp validator.h verbosity.h codecs/AfUtf8/memory.h aligned_alloc.h testset_random.h testset_standard.h testset_files.h 

validator.o: validator.cpp validator.h codecs/AfUtf8/memory.h verbosity.h codecs/AfUtf8/memory.h codecs/stangvik.h codecs/postgresql.h codecs/AfUtf8/AfUtf8.h codecs/u8u16.h

testset_standard.o: testset_standard.cpp testset_standard.h testset.h kuhn_extracted.h

testset_files.o: testset_files.cpp testset_files.h testset.h

testset_random.o: testset_random.cpp testset_random.h testset.h

stangvik.o: codecs/stangvik.cpp codecs/stangvik.h
	$(CXX) $(CXXFLAGS) -o $@ -c $<

postgresql.o: codecs/postgresql.c codecs/postgresql.h codecs/pg_wchar.h
	$(CC) $(CFLAGS) -o $@ -c $<

pg_wchar.o: codecs/pg_wchar.c codecs/pg_wchar.h
	$(CC) $(CFLAGS) -o $@ -c $<

u8u16.o: codecs/u8u16.cpp codecs/u8u16.h
	$(CXX) $(CXXFLAGS_NO_WERROR) -o $@ -c $<

z_validate_avx2.o: codecs/faster-utf8-validator/z_validate.c
	$(CC) $(CXXFLAGS_NO_WERROR) -DAVX2 -o $@ -c $<

z_validate_sse4.o: codecs/faster-utf8-validator/z_validate.c
	$(CC) $(CXXFLAGS_NO_WERROR) -DSSE4 -o $@ -c $<

zwegner.o: codecs/zwegner.c codecs/zwegner.h
	$(CC) $(CFLAGS) -o $@ -c $<

lemire.o: codecs/lemire.c codecs/lemire.h codecs/fastvalidate-utf-8/include/simdutf8check.h
	$(CC) $(CFLAGS_NO_WERROR) -o $@ -c $<

af_with_verbose_hack.o: codecs/af_with_verbose_hack.cpp codecs/AfUtf8/AfUtf8.cpp codecs/AfUtf8/AfUtf8.h
	$(CXX) $(CXXFLAGS) -o $@ -c $<

kuhn_extracted.h: contrib/convert-kuhn-tests.pl contrib/UTF-8-test.txt
	perl contrib/convert-kuhn-tests.pl < contrib/UTF-8-test.txt > $@

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

clean:
	rm -f utf8fuzz $(OBJS) kuhn_extracted.h

