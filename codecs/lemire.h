// thin wrapper around 
#ifndef LEMIRE_UTF_8_H
#define LEMIRE_UTF_8_H

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

int lemire_is_valid_utf8 (size_t len, char *value);

#ifdef __cplusplus
}
#endif

#endif // LEMIRE_UTF_8_H
