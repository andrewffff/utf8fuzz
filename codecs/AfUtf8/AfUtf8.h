
#ifndef AFUTF8_H
#define AFUTF8_H

// for size_t
#include <stdlib.h>


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Simple character-by-character implementation. If you compile with
 * -DAFUTF8_VERBOSE=3 (or something higher than 3) then very detailed
 * diagnostics will be printed to stdout.
 */
int AfUtf8_check_prototype(const size_t len, const char* ptr);


/**
 * Returns 1 if the len bytes starting at ptr are valid UTF-8, 0 otherwise.
 *
 * If len == 0 then 1 is always returned (an empty string is valid
 * UTF-8!) and ptr is ignored.
 */
int AfUtf8_check(const size_t len, const char* ptr);


#ifdef __cplusplus
};
#endif

#endif // AFUTF8_H

