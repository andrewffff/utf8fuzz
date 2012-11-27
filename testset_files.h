
#ifndef TESTSET_FILES_H
#define TESTSET_FILES_H

#include "testset.h"
#include <list>

/**
 * You pass in a list of filenames to the constructor.  It will read them all into memory
 * (possibly printing error message and aboring the program, if an error occurs!) Each
 * file is an individual test item.
 */
class TestSetFiles : public TestSet {
private:
	struct OneTest {
		std::string name;
		std::vector<unsigned char> data;
		
		OneTest(const char* n, std::vector<unsigned char>& d) : name(n), data(d) {;}
	};

	std::list<OneTest> tests;
	std::list<OneTest>::iterator currentTest;

public:
	TestSetFiles(const std::vector<std::string>& filenames);
	bool next(std::vector<unsigned char>& dest);
	std::string description();
};


#endif // TESTSET_FILES_H

