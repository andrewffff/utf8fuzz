
#ifndef ALIGNED_ALLOC
#define ALIGNED_ALLOC

#include <atomic>
#include <algorithm>
#include <assert.h>
#include <stdlib.h>

/**
 * An aligned_alloc object holds a memory allocation of bytes.
 * The bytes will be allocated from the operating system as a large
 * block, and the beginning or end may be a set number of bytes away
 * from a page boundary.
 *
 * The intended purpose is to test code against memory blocks with
 * a guard page within a certain distance of the beginning/end, in
 * combination with something like Mac OS X's malloc with MallocGuardEdges=1.
 * 
 * The page size, and minimum allocation size, may need to be tuned. Only
 * tested on Mac OS X.
 *
 * We assert rather than throw things. You must allocate at least one byte.
 */
class aligned_alloc {
private:
	static std::atomic<size_t*> global_cached_allocation;
	static size_t page_size;
	static size_t min_allocation_size;

	size_t returned_size;
	unsigned char* returned_ptr;

	unsigned char* allocation;
	size_t allocation_size;

	void _init(size_t size,int alignment_preference,int alignment_gap) {
		// normalize parameters
		if(alignment_preference != START_OF_PAGE && alignment_preference != END_OF_PAGE) {
			alignment_preference = NO_PAGE_PREFERENCE;
			alignment_gap = 0;
		}

		// fast path if we don't require guard page alignment and are small enough
		if(size == 0 ||
				(alignment_preference == NO_PAGE_PREFERENCE && size < min_allocation_size)) {
			allocation = size ? (unsigned char*)malloc(size) : nullptr;
			allocation_size = 0;
			assert(allocation);

			returned_ptr = (unsigned char*) allocation;
			returned_size = size;
			return;
		}

		// take the shared block. use if we can, otherwise discard it
		size_t* block = 0;
		block = global_cached_allocation.exchange(block);
		if(block && *block >= size + alignment_gap) {
			allocation_size = *block;
			allocation = (unsigned char*) block;
		} else {
			free(block);

			// make sure we allocate enough space to hit the large page allocator
			// and round up to the page size
			allocation_size = std::max(size + alignment_gap, min_allocation_size);
			allocation_size += (page_size - 1);
			allocation_size &= ~(page_size - 1);

			allocation = (unsigned char*) malloc(allocation_size);
		}

		assert(allocation);
		returned_ptr = (alignment_preference == END_OF_PAGE)
			? (allocation + allocation_size - alignment_gap - size)
			: (allocation + alignment_gap);
		returned_size = size;

		//printf("ALLOCATION(%p) + GAP(%0d) == %p\n", allocation, alignment_gap, returned_ptr);

		assert(returned_ptr >= allocation);
		assert(returned_ptr + returned_size <= allocation + allocation_size);

		// fill up unused bits
		memset(allocation,                   0xFE, returned_ptr - allocation);
		memset(returned_ptr + returned_size, 0xFE, (allocation + allocation_size) - (returned_ptr + returned_size));
	}

	void _finish() {
		// fast path?
		if(!allocation_size) {
			free(allocation);
			return;
		}

		// put the size of our allocation, into the allocation itself
		size_t* block = (size_t*) allocation;
		*block = allocation_size;

		// replace existing global cache (if any) with our just-freed block.
		// if there was an older global cache, truly free it
		block = std::atomic_exchange(&global_cached_allocation, block);
		free(block);
	}



public:
	const static int NO_PAGE_PREFERENCE = 0;
	const static int START_OF_PAGE = 1;
	const static int END_OF_PAGE = 2;

	aligned_alloc(size_t size, int alignment_preference = NO_PAGE_PREFERENCE, int alignment_gap = 0)
	{
		_init(size,alignment_preference,alignment_gap);
		memset(begin(), 0, this->size());
	}

	template<typename _SourceContainerT>
	aligned_alloc(const _SourceContainerT& c, int alignment_preference = NO_PAGE_PREFERENCE, int alignment_gap = 0)
	{
		_init(c.size(),alignment_preference,alignment_gap);
		memcpy(begin(), &*c.begin(), size());
	}

	~aligned_alloc()
	{
		_finish();
	}

	unsigned char* begin() { return returned_ptr; }
	unsigned char* end()   { return returned_ptr + returned_size; }
	const unsigned char* begin() const { return returned_ptr; }
	const unsigned char* end()   const { return returned_ptr + returned_size; }

	size_t size() const { return returned_size; }
};



#endif // ALIGNED_ALLOC


