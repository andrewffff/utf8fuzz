
#ifndef TESTSET_RANDOM_H
#define TESTSET_RANDOM_H

#include "testset.h"

class RandomData;

/**
 * Returns randomly generated utf-8 blocks. Some are valid and
 * some aren't. Initialising with the same seed will always yield
 * the same data (given the same version of the code).
 */
class TestSetRandom : public TestSet {
private:
	RandomData* d;
	size_t dLen;
	int nextSeed;
	int remaining;
	size_t minLength, maxLength;  // generate this many codepoints (not bytes!)
	bool entirelyValid;           // if true, we only produce valid utf-8

public:
	/* limit == -1 goes on forever! */
	TestSetRandom(int initialSeed, int limit, size_t minLength, size_t maxLength, bool entirelyValid);
	virtual ~TestSetRandom();

	bool next(std::vector<unsigned char>& dest);
	std::string description();

	int alignment();
	bool alignEnd();
};

#endif // TESTSET_RANDOM_H

