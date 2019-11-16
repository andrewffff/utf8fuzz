
#include "lemire.h"

// this file contains this inline:
//static bool validate_utf8_fast_avx(const char *src, size_t len)
#include "fastvalidate-utf-8/include/simdutf8check.h"

int lemire_is_valid_utf8 (size_t len, char *value)
{
	return validate_utf8_fast_avx(value, len);
}

