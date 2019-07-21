
#include "aligned_alloc.h"

std::atomic<size_t*> my_aligned_alloc::global_cached_allocation(nullptr);

size_t my_aligned_alloc::page_size = 4096;

// compare to LARGE_THRESHOLD_LARGEMEM in 
// http://www.opensource.apple.com/source/Libc/Libc-825.25/gen/magazine_malloc.c
size_t my_aligned_alloc::min_allocation_size = 128 * 1024;

