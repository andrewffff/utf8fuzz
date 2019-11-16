#include "zwegner.h"

extern int z_validate_utf8_avx2(const char* data, size_t len);
extern int z_validate_utf8_sse4(const char* data, size_t len);

int zwegner_avx2_is_valid_utf8 (size_t len, char *value)
{
	return z_validate_utf8_avx2(value, len);
}

int zwegner_sse4_is_valid_utf8 (size_t len, char *value)
{
	return z_validate_utf8_sse4(value, len);
}
