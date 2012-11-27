
#ifndef TESTSET_STANDARD_H
#define TESTSET_STANDARD_H

#include "testset.h"
#include <list>

/**
 * Returns a small number of specific handcrafted tests.
 */
class TestSetStandard : public TestSet {
private:
	struct OneTest {
		std::string name;
		std::vector<unsigned char> data;
		
		OneTest(const char* n, std::vector<unsigned char>& d) : name(n), data(d) {;}
	};

	std::list<OneTest> tests;
	std::list<OneTest>::iterator currentTest;

public:
	TestSetStandard();
	bool next(std::vector<unsigned char>& dest);
	std::string description();
};


#endif // TESTSET_STANDARD_H

