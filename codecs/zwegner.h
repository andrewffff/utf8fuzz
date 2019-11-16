// thin wrapper around the pg_* postgresql stuff
#ifndef FASTVALIDATE_UTF_8_H
#define FASTVALIDATE_UTF_8_H


#include <stdlib.h>


#ifdef __cplusplus
extern "C" {
#endif

int postgresql_is_valid_utf8 (size_t len, char *value);


#ifdef __cplusplus
}
#endif


#endif // FASTVALIDATE_UTF_8_H
