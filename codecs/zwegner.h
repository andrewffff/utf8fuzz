#ifndef ZWEGNER_UTF_8_H
#define ZWEGNER_UTF_8_H


#include <stdlib.h>


#ifdef __cplusplus
extern "C" {
#endif

int zwegner_avx2_is_valid_utf8 (size_t len, char *value);
int zwegner_sse4_is_valid_utf8 (size_t len, char *value);


#ifdef __cplusplus
}
#endif


#endif // ZWEGNER_UTF_8_H
