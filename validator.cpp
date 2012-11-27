
#include "validator.h"

#include <unistd.h>
#include <cassert>
#include <cerrno>

#include <stdlib.h>
#include <xlocale.h>

#include <vector>
#include <string>

#include <iconv.h>

#include "codecs/AfUtf8/afutf8.h"
#include "codecs/stangvik.h"
#include "codecs/postgresql.h"
#include "codecs/u8u16.h"

using namespace std;
using namespace AfUtf8;


class MbstowcsValidator : public Validator
{
private:
	locale_t utf8Locale;

public:
	MbstowcsValidator()
	{
		utf8Locale = newlocale(LC_CTYPE_MASK,"en_US.UTF-8",LC_GLOBAL_LOCALE);
		assert(utf8Locale);
	}

	~MbstowcsValidator() {
		freelocale(utf8Locale);
	}

	string name() const { return "mbstowcs"; }
	bool ours() const { return false; }

	bool validate(const memory<unsigned char>& v) {
		const char* inbytes = (const char*) (&v[0]);

		size_t s = mbsnrtowcs_l(NULL, &inbytes, v.size(), v.size()+2, NULL, utf8Locale);
		if(s == (size_t)-1) {
			assert(errno == EILSEQ || errno == EINVAL);
			return false;
		}

		assert(s <= v.size());
		return true;
	}
};



class IconvValidator : public Validator
{
private:
	static bool init;
	static vector<unsigned char> junk;
	static iconv_t cd;

public:
	IconvValidator() {; }
	
	std::string name() const { return "iconv"; }
	bool ours() const { return false; }

	bool validate(const memory<unsigned char>& v) {
	
		if(!init) {
			init = true;
			cd = iconv_open("UTF-8", "UTF-8");
			assert(cd != (iconv_t)-1);
		} else {
			iconv(cd, 0, 0, 0, 0);
		}
	
		if(junk.size() < 2*v.size())
			junk.resize(2*v.size());
	
		char* inbytes = (char*) (&v[0]);
		char* outbytes = (char*) (&junk[0]);
		size_t inbytesleft = v.size();
		size_t outbytesleft = junk.size();
		
		size_t result = iconv(cd, &inbytes, &inbytesleft, &outbytes, &outbytesleft);
	
		assert(inbytes + inbytesleft == (char*) &*v.end());
		assert(outbytes + outbytesleft == (char*) &*junk.end());
	
		assert(result == -1 || result == 0);
		if(result == -1) {
			assert(errno != E2BIG);
			assert(errno == EILSEQ || errno == EINVAL);
			return false;
		} else {
			return true;
		}
	}
};

bool IconvValidator::init = false;
vector<unsigned char> IconvValidator::junk;
iconv_t IconvValidator::cd;



class U8u16Validator : public Validator
{
private:
	static vector<unsigned char> junk;

public:
	U8u16Validator() {; }
	
	std::string name() const { return "u8u16"; }
	bool ours() const { return false; }

	void prepare(size_t l) {
		if(junk.size() < 128 + 2*l) {
			junk.resize(128 + 2*l);
		}
	}

	bool validate(const memory<unsigned char>& v) {
		char* inbytes = (char*) (&v[0]);
		char* outbytes = (char*) (&junk[0]);
		size_t inbytesleft = v.size();
		size_t outbytesleft = junk.size();
		
		size_t result = u8u16(&inbytes, &inbytesleft, &outbytes, &outbytesleft);
	
		assert(inbytes + inbytesleft == (char*) &*v.end());
		assert(outbytes + outbytesleft == (char*) &*junk.end());
	
		assert(result == -1 || result == 0);
		if(result == -1) {
			assert(errno != E2BIG);
			assert(errno == EILSEQ || errno == EINVAL);
			return false;
		} else {
			return true;
		}
	}
};

vector<unsigned char> U8u16Validator::junk;



class StangvikValidator : public Validator {
public:
	StangvikValidator() {;}

	std::string name() const { return "stangvik"; }
	bool ours() const { return false; }

	bool validate(const memory<unsigned char>& v) {
		return stangvik_is_valid_utf8(v.size(), (char*) &*v.begin());
	}
};


class PostgresqlValidator : public Validator {
public:
	PostgresqlValidator() {;}

	std::string name() const { return "postgresql"; }
	bool ours() const { return false; }

	bool validate(const memory<unsigned char>& v) {
		return postgresql_is_valid_utf8(v.size(), (char*) &*v.begin());
	}
};


class AfPrototypeValidator : public Validator {
public:
	AfPrototypeValidator() {;}

	std::string name() const { return "verbose"; }
	bool ours() const { return true; }

	bool validate(const memory<unsigned char>& v) {
		return AfUtf8_check_prototype(v.size(), (char*) &*v.begin());
	}
};



class AfVectorValidator : public Validator {
public:
	AfVectorValidator() {;}

	std::string name() const { return "vector"; }
	bool ours() const { return true; }

	bool validate(const memory<unsigned char>& v) {
		return AfUtf8_check(v.size(), (char*) &*v.begin());
	}
};


vector<Validator*> Validator::createAll(bool includeBrokenImpls) {
	vector<Validator*> validators;

	validators.push_back(new AfPrototypeValidator());
	validators.push_back(new AfVectorValidator());
	validators.push_back(new StangvikValidator());
	validators.push_back(new PostgresqlValidator());
	validators.push_back(new U8u16Validator());

	if(includeBrokenImpls) {
		/* iconv isn't picky enough - it's actually CESU-8 */
		validators.push_back(new IconvValidator());

		/* FreeBSD/OS X's mbs* functions accept CESU-8 stuff and
		 * also accept values above U+10FFFF
		 */
		validators.push_back(new MbstowcsValidator());
	}

	return validators;
}

