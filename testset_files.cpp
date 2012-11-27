
#include "testset_files.h"

#include <stdio.h>

bool TestSetFiles::next(std::vector<unsigned char>& dest) {
	if(currentTest != tests.end())
		++currentTest;

	if(currentTest == tests.end())
		return false;

	dest = currentTest->data;
	return true;
}


std::string TestSetFiles::description() {
	return currentTest->name;
}


TestSetFiles::TestSetFiles(const std::vector<std::string>& filenames) {
	std::vector<unsigned char> v;

	v.clear();
	tests.push_back(OneTest("you need to call next()", v));

	for(std::string fname : filenames) {
		v.clear();

		FILE* fp = fopen(fname.c_str(), "rb");
		if(fp) {
			if(fseek(fp, 0, SEEK_END) == 0) {
				off_t s = ftello(fp);
				if(s >= 0) {
					v.resize(s);
					if(fseek(fp, 0, SEEK_SET) == 0) {
						if(fread(&v[0], 1, s, fp) != s) {
							v.clear();
						}
					}
				}
			}
			fclose(fp);
		}

		if(v.empty()) {
			printf("ERROR: could not open from, or read entirely, %s", fname.c_str());
			exit(1);
		}

		tests.push_back(OneTest(fname.c_str(), v));
	}
	
	currentTest = tests.begin();
}

