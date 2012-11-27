
#include <vector>
#include <random>
#include <algorithm>
#include <cassert>

#include "testset_random.h"


using namespace std;

typedef mt19937 generator_type;

// our own bodged together random distributions
class utf8_valid_distribution {
	exponential_distribution<double> d;
public:
	utf8_valid_distribution() : d(1.0 / (double)0x600) {;}

	template<typename URNG> int operator()(URNG& urng) {
		int i;
		do {
			i = (int) d(urng);
		} while((i >= 0xd800 && i <= 0xdfff) || (i > 0x10ffff));
		return i;
	}
};

class utf8_surrogate_distribution : public uniform_int_distribution<int> {
public:
	utf8_surrogate_distribution() : uniform_int_distribution(0xD800, 0xDFFF) {;}
};

class utf8_outofrange_distribution : public uniform_int_distribution<int> {
public:
	utf8_outofrange_distribution() : uniform_int_distribution(0x110000, 0x7FFFFFFF) {;}
};


class byte_distribution : public uniform_int_distribution<int> {
public:
	byte_distribution() : uniform_int_distribution(0,255) {;}
};


class RandomData {
public:
	// these are inputs. when these are the same, an identical binary should produce identical data
	int seed;
	size_t minLength, maxLength;
	bool entirelyValid;

	// desired memory alignment
	int alignment;
	bool alignEnd;

	// generated data. everything below is intermediate data.
	vector<unsigned char> data;

	// we generate and encode a bunch of valid codepoints. log distribution.
	size_t cpCount;

	enum ANOMALY_TYPE {
		// we insert some illegal things at the same time as we generate
		// codepoints - we insert an illegal codepoint, or encode it illegally,
		// or both. There is a 2% chance of each of these occurring once, a 2% x 2%
		// of one occuring twice in the same generated data, etc.
		//
		// Offsets are 1-based, with a negative sign indicating that the offset
		// is from the end instead of the beginning. (We don't use a 0-based
		// offset because it would be impossible to distinguish between +0 and -0).
		//
		// From this perspective, +1 means "before the first item" and -1 means "after
		// the last item".
		//
		// We use a non-linear distribution to put more badness near the beginning and
		// end than in the middle. 
		cpOfsSurrogate = 0,
		cpOfsOutOfRange,
		cpOfsOverlong,
		cpOfsOverlongAndZero,
		cpOfsOverlongAndSurrogate,
		cpOfsOverlongAndOutOfRange,
	
		// similar to the above, but instead of codepoint aberrations with an
		// offset measured in codepoints, we add/remove/replace bytes after generation
		// and encoding. The same likeliness, and the same offset encoding. (Slight
		// difference with 'replace' offsets - they refer to a specific element, rather
		// than a space before or after an element)
		//
		// The distribution is, to avoid overflow easily, capped at 
		// length(data) - length(byteOfsRemove).. half of the offsets
		// being measured from either end should make that a non-issue.
		byteOfsAdd,
		byteOfsReplace,
		byteOfsRemove,

		ANOMALY_TYPE_COUNT
	};

	// XXX needs to be a struct so multiple types of issue can occur at the
	// same index in any order. then we need to start generating those subsequent
	// errors, and byte mutations at the same locations too!
	vector<ssize_t> anomalyOfs[ANOMALY_TYPE_COUNT];
	
	// nb 0.98^9 ~= 83% of generated data should have none of the above
	// introduced defects
	RandomData(int seed, size_t minLength, size_t maxLength, bool entirelyValid)
		: seed(seed), minLength(minLength), maxLength(maxLength), entirelyValid(entirelyValid)
	{

		generator_type gen(seed);
		bernoulli_distribution rHappening(0.02);
		bernoulli_distribution rFromEnd(0.5);
		uniform_int_distribution<int> rAlignment(0, 63);
		uniform_int_distribution<size_t> rCpCount(minLength, maxLength);
//		uniform_int_distribution<int> rCpCount(1, 96);

		alignment = rAlignment(gen);
		alignEnd = rAlignment(gen) & 1;

		// generate number of code points
		cpCount = rCpCount(gen);

		// generate a random number of random codepoint offsets
		uniform_int_distribution<int> rCpOfs(0, cpCount-1);
		if(!entirelyValid) 
			for(int i = cpOfsSurrogate; i <= cpOfsOverlongAndOutOfRange; ++i)
				fillVector(anomalyOfs[i], gen, rHappening, rFromEnd, rCpOfs);

		// create the data (with interleaved badness at cpOfs*)
		createData(gen);

		// generate a random number of random byte offsets
		if(!entirelyValid && data.size() > 40) {
			uniform_int_distribution<int> rByteOfs(0, data.size()-20);  // XXX
			for(int i = byteOfsAdd; i <= byteOfsRemove; ++i)
				fillVector(anomalyOfs[i], gen, rHappening, rFromEnd, rByteOfs);
		}

		// apply the byte mutuations
		mutateBytes(gen);
	}

	bool findOfs(ANOMALY_TYPE vidx, size_t size, size_t ofs) const {
		const vector<ssize_t>& v = anomalyOfs[vidx];
		return find(v.begin(), v.end(), ofs+1) != v.end() || find(v.begin(), v.end(), ofs-size-1) != v.end();
	}

	void createData(generator_type& gen) {
		utf8_valid_distribution rCpValue;
		utf8_surrogate_distribution rCpSurrogateValue;
		utf8_outofrange_distribution rCpOutOfRangeValue;
		geometric_distribution<int> rExtraBytes(0.4);

		data.reserve(cpCount * 4); // XXX revisit when no longer uniform dist

		// we run through len+1 times, to generate len values and to check for an
		// inserted anomaly before each codepoint, and also after the last one
		for(size_t ofs = 0; ofs <= cpCount; ++ofs) {

			// look for various things to generate
			if(findOfs(cpOfsSurrogate, cpCount, ofs))
				encodeToData(rCpSurrogateValue(gen));

			if(findOfs(cpOfsOutOfRange, cpCount, ofs))
				encodeToData(rCpOutOfRangeValue(gen));

			if(findOfs(cpOfsOverlong, cpCount, ofs))
				encodeToData(rCpValue(gen), rExtraBytes(gen));

			if(findOfs(cpOfsOverlongAndZero, cpCount, ofs))
				encodeToData(0, rExtraBytes(gen));

			if(findOfs(cpOfsOverlongAndSurrogate, cpCount, ofs))
				encodeToData(rCpSurrogateValue(gen), rExtraBytes(gen));

			if(findOfs(cpOfsOverlongAndOutOfRange, cpCount, ofs))
				encodeToData(rCpOutOfRangeValue(gen), rExtraBytes(gen));

			
			// generate codepoint (except for our "check after last byte" iteration)
			if(ofs < cpCount)
				encodeToData(rCpValue(gen));
		}
	}

	/**
	 * Encodes the given value as a UTF-8 codepoint, possibly as more bytes than is
	 * needed (50% exponential distribution on the extra bytes)
	 */
	void encodeToData(int value, int extraBytes = 0) {
		// this is both a fast path, and a necessary special case for 1-byte encodings
		/*if(extraBytes == 0) {
			if(value <= 0x007f) {
				data.push_back(value);
				return;
			} else if(value <= 0x07ff) {
				data.push_back(0xC0 | (value >> 6));
				data.push_back(0x80 | (value & 0x3f));
				return;
			}
		}*/
		assert(value <= 0x7FFFFFFF && value >= 0);

		// calculate what the length should be
		int len = extraBytes;
		if(value <= 0x007F)
			len += 1;
		else if(value <= 0x07FF)
			len += 2;
		else if(value <= 0xFFFF)
			len += 3;
		else if(value <= 0x1FFFFF)
			len += 4;
		else if(value <= 0x3FFFFFF)
			len += 5;
		else if(value <= 0x7FFFFFFF)
			len += 6;

		if(len > 6)
			len = 6;

		// encode. 1 byte encodings are a special case
		if(len == 1) {
			data.push_back(value);
		} else {
			data.resize(data.size() + len);
			vector<unsigned char>::reverse_iterator p = data.rbegin();
			for(int i = 0; i < len-1; ++i) {
				*(p++) = 0x80 | (value & 0x3F);
				value >>= 6;
			}

			// add the 1...10 prefix to the first byte
			*p = value | ((0xFF00 >> len) & 0xFF);
		}
	}

	void mutateBytes(generator_type& gen) {
		byte_distribution rByteValue;

		// this can be far more efficient...
		for(ssize_t ofs : anomalyOfs[byteOfsAdd])
			data.insert(byteOfsToIt(data, ofs), (unsigned char) rByteValue(gen));

		for(ssize_t ofs : anomalyOfs[byteOfsRemove])
			data.erase(byteOfsToIt(data, ofs));

		for(ssize_t ofs : anomalyOfs[byteOfsReplace])
			*(byteOfsToIt(data, ofs)) = (unsigned char) rByteValue(gen);
	}


	// note the third parameter can either be a fixed integer length
	// or a true/false distribution
	static void fillVector(vector<ssize_t>& dest,
			generator_type& gen,
			bernoulli_distribution& rHappening,
			bernoulli_distribution& rFromEnd,
			uniform_int_distribution<int>& rOffset)
	{
		while(rHappening(gen)) {
			if(rFromEnd(gen)) {
				dest.push_back(-1-rOffset(gen));
			} else {
				dest.push_back(+1+rOffset(gen));
			}
		}
	}

	template<class Container>
	static typename Container::iterator byteOfsToIt(Container& c, ssize_t ofs)
	{
		if(ofs < 0)
			return c.end() - (-ofs - 1);
		else
			return c.begin() + (ofs - 1);
	}

};
				

////////////////////////////////////

TestSetRandom::TestSetRandom(int initialSeed, int limit, size_t minLength, size_t maxLength, bool entirelyValid)
	: d(0), nextSeed(initialSeed), remaining(limit), minLength(minLength), maxLength(maxLength), entirelyValid(entirelyValid)
{ ; }


TestSetRandom::~TestSetRandom()
{
	delete d;
	d = 0;
}


bool TestSetRandom::next(std::vector<unsigned char>& dest)
{
	delete d;
	d = 0;

	if(remaining == 0)
		return false;
	if(remaining > 0)
		--remaining;

	d = new RandomData(nextSeed++, minLength, maxLength, entirelyValid);
	d->data.swap(dest); // corrupt it, never used again so who cares
	dLen = dest.size();
	return true;
}


string TestSetRandom::description()
{
	char s[100];
	sprintf(s, "Random data, seed %0d, %0zd bytes long, ofs %c%0d.", d->seed, dLen, alignEnd() ? '-' : '+', alignment());
	return s;
}

int TestSetRandom::alignment()
{
	return d->alignment;
}

bool TestSetRandom::alignEnd() {
	return d->alignEnd;
}

