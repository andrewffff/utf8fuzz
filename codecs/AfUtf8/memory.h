/**
 * A very vaguely stl-ish container (not in any sense that someone who 
 * really knows the C++ standard library would recognise) which references
 * a block of pre-existing memory. 
 */

#ifndef AFUTF8_MEMORY_H
#define AFUTF8_MEMORY_H

#include <iterator>

namespace AfUtf8 {

template<typename T>
class memory {
private:
	T* _data;
	size_t _size;

public:
	template<typename Container>
	memory(Container& c)
	: _data(&*c.begin()), _size(c.end() - c.begin())
	{;}

	template<typename Iterator>
	memory(Iterator begin, Iterator end)
	: _data(&*begin), _size(end - begin)
	{;}


	typedef T* iterator;
	typedef const T* const_iterator;

	inline iterator begin() { return _data; }
	inline const_iterator begin() const { return _data; }
	inline const_iterator cbegin() const { return _data; }

	inline iterator end() { return _data + _size; }
	inline const_iterator end() const { return _data + _size; }
	inline const_iterator cend() const { return _data + _size; }

	inline size_t size() const { return _size; }
	inline bool empty() const { return _size == 0; }

	inline T& operator[](size_t i) { return _data[i]; }
	inline const T& operator[](size_t i) const { return _data[i]; }
};


} // namespace AfUtf8

#endif // MEMORY_H
