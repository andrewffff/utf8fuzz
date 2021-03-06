#include "config_defs.h"
/* Compiling for i586 with SSE */

#define U8U16_TARGET SSE_TARGET
#define BYTE_ORDER LITTLE_ENDIAN

#define S2P_ALGORITHM S2P_IDEAL
#define P2S_ALGORITHM P2S_IDEAL

#define BIT_DELETION SHIFT_TO_RIGHT8
#define DOUBLEBYTE_DELETION FROM_LEFT8

#define INBUF_READ_NONALIGNED
#define OUTBUF_WRITE_NONALIGNED


