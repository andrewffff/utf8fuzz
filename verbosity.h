
// We keep verbosity in a global variable so that we can #define
// it away for benchmarked builds

#ifndef VERBOSITY_H
#define VERBOSITY_H

#ifdef BENCHMARK_BUILD
#define g_verbosity (0)
#else
extern int g_verbosity;
#endif


#endif // VERBOSITY_H

