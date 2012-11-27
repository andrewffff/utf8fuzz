
#ifndef TESTSET_H
#define TESTSET_H

#include <vector>
#include <string>

class TestSet {
public:
	virtual ~TestSet() {;}

	/**
	 * If there is another test available, place it into the vector (destroying
	 * any existing contents) and return true. Otherwise return false.
	 */
	virtual bool next(std::vector<unsigned char>& dest) = 0;
	
	/**
	 * Return a string description of the test data last provided by next().
	 */
	virtual std::string description() = 0;

	/**
	 * (May be used, may not, depending on whether particular test options are
	 * enabled. For determinism reasons the value is always provided as part of
	 * the test item.)
	 *
	 * Return the desired offset from 64-byte alignment (valid values 0 through 63)
	 * for the test data last provided by next(). 64 bytes because that's cacheline
	 * width.
	 */
	virtual int alignment() {
		return 0;
	}

	/**
	 * (May be used, may not, depending on whether particular test options are
	 * enabled. For determinism reasons the value is always provided as part of
	 * the test item.)
	 *
	 * Do we align from the end instead of the beginning? Used when we are putting
	 * everything in large allocations so that we segfault on over/underflowing them.
	 */
	virtual bool alignEnd() {
		return false;
	}
};

#endif // TESTSET_H

