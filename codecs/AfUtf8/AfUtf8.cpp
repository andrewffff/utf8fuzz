
#include "AfUtf8.h"

#include "memory.h"

#include <string>
#include <cassert>
#include <vector>

#include <immintrin.h>

using namespace std;
using namespace AfUtf8;

#ifndef AFUTF8_VERBOSE
#define AFUTF8_VERBOSE (0)
#endif

static void printVector(const __m128i& vec, const char* lineWithDescription = 0)
{
	if(AFUTF8_VERBOSE < 3)
		return;

	char output[150];
	
	const unsigned char* data = (const unsigned char*)(&vec);
	for(int i = 0; i < sizeof(vec); ++i) {
		sprintf(output + i*3, "%02x%c",
				(int) data[i],
				(i % 4) == 3 ? '-' : ' ');
	}
	*(output + 3*sizeof(vec) - 1) = '\0';

	if(lineWithDescription)
		printf("%10s: %s\n", lineWithDescription, output);
	else
		printf("%s", output);
}

#define PV(v) (( printVector((v), (#v)) ))

// Categories that an octet can fall into (nothing falls into IMPOSSIBLE!)
#define sSTART       ( 0 )
#define sN_LAST      ( 1 )
#define sA0BF        ( 2 )
#define s809F        ( 3 )
#define s90BF        ( 4 )
#define s808F        ( 5 )
#define sIMPOSSIBLE  ( 6 )

#define maskSTART  (( 1 << sSTART ))
#define maskN_LAST  (( 1 << sN_LAST ))
#define maskA0BF  (( 1 << sA0BF ))
#define mask809F  (( 1 << s809F ))
#define mask90BF  (( 1 << s90BF ))
#define mask808F  (( 1 << s808F ))
#define maskIMPOSSIBLE  (( 1 << sIMPOSSIBLE ))


static unsigned char getCategoriesForOctet(unsigned char c) {
	if(c <= 0x7F)
		return maskSTART;
	else if(c <= 0x8F)
		return maskN_LAST | mask809F | mask808F;
	else if(c <= 0x9F)
		return maskN_LAST | mask809F | mask90BF;
	else if(c <= 0xBF)
		return maskN_LAST | maskA0BF | mask90BF;
	else if(c <= 0xC1)
		return 0;
	else if(c <= 0xF4)
		return maskSTART;
	else
		return 0;
}

static const char mapCategoriesForOctet[] = {
	maskSTART, maskSTART, maskSTART, maskSTART, maskSTART, maskSTART, maskSTART, maskSTART, // 0_ .. 7_
	maskN_LAST | mask809F | mask808F, // 8_
	maskN_LAST | mask809F | mask90BF, // 9_
	maskN_LAST | maskA0BF | mask90BF, // A_
	maskN_LAST | maskA0BF | mask90BF, // B_
	0, 0, 0, 0 // C_, D_, E_, F_ - some are maskSTART, this isn't precise enough
};


__m128i vecGetCategoriesForOctet(const __m128i& data)
{
	__m128i catmap = *(__m128i*) mapCategoriesForOctet;
	__m128i shifted = _mm_srli_epi16(_mm_and_si128(data, _mm_set1_epi8(0xf0)), 4);
	__m128i mostlyMapped = _mm_shuffle_epi8(catmap, shifted);

/*
	printVector(catmap, "CATMAP");
	printVector(data, "DATA");
	printVector(shifted, "SHIFTED");
	printVector(mostlyMapped, "MOSTLY MAPPED");
	*/
	// we want to match C2 .. F4 or, expressed as signed 8-bit ints,
	// -62 to -12. Subtracting (unstaturated) 66 makes acceptable
	// bytes -128 to -78 inclusive, at which point one signed compare
	// can recognise all acceptable bytes
	// /*
	__m128i valuesFromC2 = _mm_sub_epi8(data, _mm_set1_epi8(66));
	__m128i matchesFromC2 = _mm_cmpgt_epi8(_mm_set1_epi8(-77), valuesFromC2);
	matchesFromC2 = _mm_and_si128(matchesFromC2, _mm_set1_epi8(maskSTART));
/*
	PV(valuesFromC2);
	PV(matchesFromC2);
*/

	return _mm_or_si128(matchesFromC2, mostlyMapped);
}


static string maskToString(unsigned char m) {
	const char hex[] = "0123456789abcdef";

	string s = "(";
	s += hex[m >> 4];
	s += hex[m & 0xf];
	s += ") ";

	if(m & maskSTART) s += "START ";
	if(m & maskN_LAST) s += "N_LAST ";
	if(m & maskA0BF) s += "A0BF ";
	if(m & mask809F) s += "809F ";
	if(m & mask90BF) s += "90BF ";
	if(m & mask808F) s += "808F ";
	if(m & maskIMPOSSIBLE) s += "IMPOSSIBLE ";
	return s;
}

// Given the first step after the START state in the DFA, all of the
// octet category requirements that occur as you step through the 
// DFA until you get back to START
#define reqSTART ( 0 )
#define reqIMPOSSIBLE ( 1 )
#define req1_LAST ( 2 )
#define req2_LAST ( 3 )
#define req3_LAST ( 4 )
#define reqA0BF ( 5 )
#define req809F ( 6 )
#define req90BF ( 7 )
#define req808F ( 8 )
#define reqNONE ( 9 )

static const char nextItems[][4] = {
	{ maskSTART, 0, 0, 0 },              // reqSTART
	{ maskIMPOSSIBLE, 0, 0, 0 },      // reqIMPOSSIBLE
	{ maskN_LAST, maskSTART, 0, 0 },      // req1_LAST
	{ maskN_LAST, maskN_LAST, maskSTART, 0 },      // req2_LAST
	{ maskN_LAST, maskN_LAST, maskN_LAST, maskSTART },      // req3_LAST
	{ maskA0BF, maskN_LAST, maskSTART, 0 }, // reqA0BF
	{ mask809F, maskN_LAST, maskSTART, 0 }, // req809F
	{ mask90BF, maskN_LAST, maskN_LAST, maskSTART }, // req90BF
	{ mask808F, maskN_LAST, maskN_LAST, maskSTART }, // req808F
	{ 0, 0, 0, 0 }
};

// transposed version of nextItems[]
static const char nextItemVectors[][16] = {
	// 0         1               2           3           4           5           6           7           8           9   10..15
	{ maskSTART, maskIMPOSSIBLE, maskN_LAST, maskN_LAST, maskN_LAST, maskA0BF,   mask809F,   mask90BF,   mask808F,   0,  0,0,0,0,0,0 },
	{ 0,         0,              maskSTART,  maskN_LAST, maskN_LAST, maskN_LAST, maskN_LAST, maskN_LAST, maskN_LAST, 0,  0,0,0,0,0,0 },
	{ 0,         0,              0,          maskSTART,  maskN_LAST, maskSTART,  maskSTART,  maskN_LAST, maskN_LAST, 0,  0,0,0,0,0,0 },
	{ 0,         0,              0,          0,          maskSTART,  0,          0,          maskSTART,  maskSTART,  0,  0,0,0,0,0,0 }
};


static unsigned char getRequirementAfterOctet(unsigned char c) {
	if(c <= 0x7F)
		return reqSTART;
	else if(c <= 0xBF)
		return reqNONE;
	else if(c <= 0xC1)
		return reqIMPOSSIBLE;
	else if(c <= 0xDF)
		return req1_LAST;
	else if(c <= 0xE0)
		return reqA0BF;
	else if(c <= 0xEC || c == 0xEE || c == 0xEF)
		return req2_LAST;
	else if(c == 0xED)
		return req809F;
	else if(c == 0xF0)
		return req90BF;
	else if(c <= 0xF3)
		return req3_LAST;
	else if(c == 0xF4)
		return req808F;
	else
		return reqIMPOSSIBLE;
}

__m128i vecGetRequirementAfterOctets(__m128i& next, const __m128i& data) {
	static const unsigned char mapTopNibbleToReq[] = {
		reqSTART, reqSTART, reqSTART, reqSTART, reqSTART, reqSTART, reqSTART, reqSTART, /* 0 .. 7f */
		reqNONE, reqNONE, reqNONE, reqNONE, /* 80 .. bf */
		req1_LAST, req1_LAST,       /* c0 .. df;  c0, c1 are treated separately */
		0, 0                        /* e0 .. ff are all treated separately */
	};

	static const unsigned char highMap[] = {
		req2_LAST,     // E1 .. EC
		req809F,       // ED
		req2_LAST,     // EE
		req2_LAST,     // EF
		req90BF,       // F0
		req3_LAST,     // F1
		req3_LAST,     // F2
		req3_LAST,     // F3
		req808F,       // F4
		reqIMPOSSIBLE, // F5
		reqIMPOSSIBLE, // F6
		reqIMPOSSIBLE, // F7
		reqIMPOSSIBLE, // F8
		reqIMPOSSIBLE, // F9
		reqIMPOSSIBLE, // FA
		reqIMPOSSIBLE  // FB .. FF
	};

	__m128i topNibbles = _mm_srli_epi16(_mm_and_si128(data, _mm_set1_epi8(0xf0)), 4);
	__m128i valuesFromTopNibbles = _mm_shuffle_epi8( *(__m128i*)mapTopNibbleToReq, topNibbles);

	__m128i isE0 = _mm_cmpeq_epi8( data, _mm_set1_epi8(0xE0) );

	__m128i isC0orC1 = _mm_cmpgt_epi8( _mm_add_epi8(data, _mm_set1_epi8(0x7f - 0xc1)), _mm_set1_epi8(0x7d) );

	__m128i isE1orGreater = _mm_cmpgt_epi8( _mm_xor_si128(data, _mm_set1_epi8(0x80)), _mm_set1_epi8(0x60) );
	__m128i highMapIdx = _mm_subs_epu8(  _mm_adds_epu8(data, _mm_set1_epi8(0xFF - 0xFB)),  _mm_set1_epi8(0xF0));
	__m128i highMapValues = _mm_shuffle_epi8( *(__m128i*)highMap, highMapIdx);

	// blend together all the potential results
	__m128i blend1 = _mm_blendv_epi8( _mm_set1_epi8(reqA0BF), _mm_set1_epi8(reqIMPOSSIBLE), isC0orC1);
	__m128i using1 = _mm_or_si128(isC0orC1, isE0);
	__m128i blend2 = _mm_blendv_epi8( highMapValues, blend1, using1);
	__m128i using2 = _mm_or_si128(isE1orGreater, using1);

	__m128i requireSet = _mm_blendv_epi8( valuesFromTopNibbles, blend2, using2);
	PV(requireSet);

	// merge requirements into vectors

	__m128i required1 = _mm_shuffle_epi8( *(__m128i*)nextItemVectors[0], requireSet);
	__m128i required2 = _mm_shuffle_epi8( *(__m128i*)nextItemVectors[1], requireSet);
	__m128i required3 = _mm_shuffle_epi8( *(__m128i*)nextItemVectors[2], requireSet);
	__m128i required4 = _mm_slli_si128(_mm_shuffle_epi8( *(__m128i*)nextItemVectors[3], requireSet), 4); // XXX what am i doing here?

	__m128i rtmp1 = _mm_or_si128( _mm_slli_si128(required1, 1), _mm_slli_si128(required2, 2) );
	__m128i rtmp2 = _mm_or_si128( _mm_slli_si128(required3, 3), _mm_slli_si128(required4, 4) );

	__m128i required = next;
	required = _mm_or_si128(required, rtmp1);
	required = _mm_or_si128(required, rtmp2);

	__m128i ntmp1 = _mm_or_si128( _mm_srli_si128(required1,15), _mm_srli_si128(required2,14) );
	__m128i ntmp2 = _mm_or_si128( _mm_srli_si128(required3,13), _mm_srli_si128(required4,12) );

	PV(next);
	next = _mm_or_si128(ntmp1, ntmp2);
	PV(next);

	PV(required);
	PV(required1);
	PV(required2);
	PV(required3);
	PV(required4);
	return required;
}

static int countBits(unsigned int n) {
	int bits = 0;
	while(n) {
		if(n&1)
			++bits;
		n >>= 1;
	}
	return bits;
}


// -1 indicates "no value at this index"
// returns true if any error occurred
// if print is true, stuff will be printed out
// for internal (ie should never occur) errors
// won't ever assert out for errors that just indicate validation of the utf-8 will fail
static bool checkForErrors(bool print, bool pastEnd, int character, int categories, int required) {
	assert(character  >= -1 && character  < 256);
	assert(categories >= -1 && categories < 256);
	assert(required   >= -1 && required   < 256);

	bool err = false;

	if(categories >= 0 || required >= 0) {
		if(categories < 0 || required < 0) {
			err = true;
			if(print) printf("<<< inconsistent end! >>>  ");
			else(assert(false));
		} else if((required & categories) != required) {
			err = true;
			if(print) printf("<-- not in required category  ");
		}
	}

	if(required >= 0 && countBits((unsigned char)required) != 1) {
		if(!pastEnd || countBits((unsigned char)required) != 0) {
			err = true;
			if(print) printf("<<< bitsRequired != 1 >>>");
		}
	}

	return err;
}


static bool runthrough(bool print,
	const memory<unsigned char>& str,
	const memory<unsigned char>& categories,
	const memory<unsigned char>& requiredMasks)
{
	memory<unsigned char>::const_iterator s = str.begin(),
		c = categories.begin(),
		r = requiredMasks.begin();

	bool err = false;

	while(s != str.end() || c != categories.end() || r != requiredMasks.end()) {

		if(print) {
			printf(" [%3lx] ", s - str.begin());

			if(s != str.end()) {
				if(isprint(*s))
					printf(" %02x  %c  ", *s, *s);
				else
					printf(" %02x .   ", *s);
			} else {
				printf("    --  ");
			}
		}

		if(print) printf("%-25s", (c != categories.end()) ? maskToString(*c).c_str() : "--");

		if(print) printf("%-13s", (r != requiredMasks.end()) ? maskToString(*r).c_str() : "--");

		err |= checkForErrors(print, s == str.end(), 
			s == str.end() ? -1 : *s,
			c == categories.end() ? -1 : *c,
			r == requiredMasks.end() ? -1 : *r);

		if(print) printf("\n");

		if(s != str.end()) ++s;
		if(c != categories.end()) ++c;
		if(r != requiredMasks.end()) ++r;
	}

	return !err;
}

extern "C" int AfUtf8_check_prototype(const size_t len, const char* ptr) 
{
	bool print = (AFUTF8_VERBOSE >= 3);
	const memory<unsigned char> str((unsigned char*) ptr, (unsigned char*) ptr + len);

	vector<unsigned char> categories(str.size() + 4);
	vector<unsigned char> requiredMasks(str.size() + 4);

	size_t i;

	for(i = str.size(); i < str.size() + 4; ++i) {
		categories[i] = maskSTART;
	}

	requiredMasks[0] = maskSTART;


	for(i = 0; i < str.size(); ++i) {
		categories[i] = getCategoriesForOctet(str[i]);

		unsigned char req = getRequirementAfterOctet(str[i]);
		requiredMasks[i+1] |= nextItems[req][0];
		requiredMasks[i+2] |= nextItems[req][1];
		requiredMasks[i+3] |= nextItems[req][2];
		requiredMasks[i+4] |= nextItems[req][3];
	}

	return runthrough(print,str,categories,requiredMasks) ? 1 : 0;
}



extern "C" int AfUtf8_check(const size_t inLen, const char* const inPtr)
{
	// don't read from memory if inLen==0 as inPtr may be invalid
	if(!inLen)
		return 1;

	// achieve alignment by ignoring bytes at the start if necessary
	int ignoreAtStart = (((unsigned long long) inPtr) & 15);
	const char* ptr = inPtr - ignoreAtStart;
	int len = inLen + ignoreAtStart;

	// we mask out the loaded content before the real start. we also require
	// that the real start offset contain a start character
	__m128i data = *(__m128i*)ptr;
	if(ignoreAtStart > 0) {
		char maskSpace[16];
		int maskI = 0;
		while(maskI < ignoreAtStart) maskSpace[maskI++] = 0x00;
		while(maskI < 16) maskSpace[maskI++] = 0xff;
		data = _mm_and_si128(data, *(__m128i*) maskSpace);
	}
	//__m128i next = _mm_insert_epi8(_mm_setzero_si128(), maskSTART, ignoreAtStart);
	char temp[16] = {0};
	temp[ignoreAtStart] = maskSTART;
	__m128i next = _mm_loadu_si128((__m128i*)temp);

	do {
		if(len < 16) {
			// Mask out any content after the real end. Replace with start characters.
			// We continue to run until a few bytes after the end - if the final byte
			// we received required that certain characters follow, we need to test for
			// them (and fail)
			if(len < -4) {
				return 1;
			}
			
			char maskSpace[16];
			int maskI = 0;
			while(maskI < len) maskSpace[maskI++] = 0xff;
			while(maskI < 16) maskSpace[maskI++] = 0x00;
			data = _mm_and_si128(data, *(__m128i*) maskSpace);
		}

		__m128i categories = vecGetCategoriesForOctet(data);

		if(AFUTF8_VERBOSE >= 3) {
			printf(" DATA[$%0lx]: ", ptr - inPtr);
			printVector(data);
			printf("\n");
			printVector(categories, "CATEGORIES");
		}

		__m128i requirements = vecGetRequirementAfterOctets(next, data);
		__m128i tests = _mm_cmpeq_epi8( _mm_and_si128(requirements, categories), requirements);
		PV(tests);

		int testResults = _mm_movemask_epi8(tests);
		if(AFUTF8_VERBOSE >= 3)
			printf("testResults: %0x\n", testResults);
		if(testResults != 0xffff) {
			return 0;
		}

		// prepare for next loop around. branch prediction should save us from going
		// really poorly here.. we just want to protect against a fault if we have
		// run past the end, and the end is at the end of a page
		len -= 16;
		ptr += 16;
		if(len > 0) {
			data = *(__m128i*)ptr;
		}
	} while(true);
}




