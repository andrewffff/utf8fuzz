
default: aftest

OPTFLAGS += -O2

CXX = clang++
CXXFLAGS_NO_WERROR = $(OPTFLAGS) -g -Wall -Wno-gnu-array-member-paren-init -std=gnu++11 -stdlib=libc++ -msse4.1
CXXFLAGS = $(CXXFLAGS_NO_WERROR) -Werror
CC = clang
CFLAGS_NO_WERROR = $(OPTFLAGS) -g -Wall
CFLAGS = $(CFLAGS_NO_WERROR) -Werror
LDFLAGS = -g -liconv -stdlib=libc++


aftest: aftest.o AfUtf8.o
	$(CXX) $(LDFLAGS) -o $@ $^

clean:
	rm -f aftest aftest.o AfUtf8.o


%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<

