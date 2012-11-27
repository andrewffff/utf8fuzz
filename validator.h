
#ifndef VALIDATOR_H
#define VALIDATOR_H

#include <string>
#include <vector>
#include "codecs/AfUtf8/memory.h"


class Validator {
public:
	Validator() {;}
	virtual ~Validator() {;}

	virtual std::string name() const = 0;
	virtual bool ours() const = 0;
	virtual bool validate(const AfUtf8::memory<unsigned char>& v) = 0;

	/** Prepare enough junk buffer space, if required, to process l bytes of UTF-8.
	 *  To keep buffer allocation out of the timed stuff.
	 */
	virtual void prepare(size_t l) {
		return;
	}

	/**
	 * Return a vector with one of each validator in it. You may
	 * elect not to include broken validators (which validate bad UTF-8
	 * or reject good UTF-8)
	 */
	static std::vector<Validator*> createAll(bool includeBrokenImpls);
};


#endif // VALIDATOR_H


