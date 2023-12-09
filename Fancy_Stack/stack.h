#ifndef STACK_H
#define STACK_H

#include <list>
#include <map>

namespace cxx 
{
	using std::map;
	using std::list;
	template <typename K, typename V> class stack
	{
		map<K, V> key_value_mapping;
		list<K> key_stack;

	public:
		stack() = default;
		stack(stack const&);
		stack(stack&&);

		V& front(K const&);

		size_t size() const;

		V& front(K const&);
		V const& front(K const&) const;

		stack& operator=(stack);
	};

	template<typename K, typename V>
	stack <K, V> (stack const& other)
	{

	}
}

#endif
