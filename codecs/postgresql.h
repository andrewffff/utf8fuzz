// thin wrapper around the pg_* postgresql stuff
#ifndef POSTGRESQL_H
#define POSTGRESQL_H


#include <stdlib.h>


#ifdef __cplusplus
extern "C" {
#endif

int postgresql_is_valid_utf8 (size_t len, char *value);


#ifdef __cplusplus
}
#endif


#endif // POSTGRESQL_H
