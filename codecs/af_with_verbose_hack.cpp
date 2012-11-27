
// Build AfUtf8.cpp with AFUTF8_VERBOSE hung off of our g_verbosity

#include "../verbosity.h"

#define AFUTF8_VERBOSE g_verbosity

#include "AfUtf8/AfUtf8.cpp"
