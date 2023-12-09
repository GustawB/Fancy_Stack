#ifndef STACK_H
#define STACK_H

#include <iostream>
#include <memory>
#include <limits.h>
#include <list>
#include <map>

namespace cxx 
{
	using std::map;
	using std::list;
	using std::shared_ptr;
	using std::make_shared;

	// Every stack will be pointing to the stack data object,
	// and if they share it and one modified it, then we 
	// create a new data for it.
	template <typename K, typename V> class stack_data
	{
		map<K, V> key_value_mapping;
		list<K> key_stack;

	public:
		stack_data();
		~stack_data() = default;

		// Used when we need to split memory.
		stack_data(const stack_data& other);
	};

	template <typename K, typename V>
	stack_data<K, V>::stack_data <K, V>() : key_value_mapping<K, V>{},
		key_stack<K, V>{}
	{}

	template <typename K, typename V>
	stack_data<K, V>::stack_data<K, V>(const stack_data<K, V>& other)
		: key_value_mapping<K, V>{other.key_value_mapping},
		key_stack<K, V>{other.key_stack}
	{}


	template <typename K, typename V> class stack
	{
		shared_ptr<stack_data<K, V>> data_wrapper;
		// Flag used to determine whetherwe can share memory or not.
		static constinit bool bIsShareable = true;
	public:
		stack();
		stack(stack const&); //copy constructor;
		stack(stack&&); // move constructor;

		V& front(K const&);

		size_t size() const;

		V& front(K const&);
		V const& front(K const&) const;

		stack& operator=(stack);

	private:
		void aboutToModify(bool);
	};

	template<typename K, typename V>
	stack<K, V>::stack <K, V>() 
		: data_wrapper{make_shared<stack_data<K, V>>},
		unshareable{0}
	{}

	template<typename K, typename V>
	stack<K, V>::stack <K, V> (stack const& other)
	{
		if (other.bIsShareable)
		{
			//I think this should increment the ref_count.
			data_wrapper = other.data_wrapper;
		}
		else
		{
			//Create new data object.
			data_wrapper = make_shared<stack_data<K, V>>
				(other.data_wrapper.get());
		}
	}
	
	template<typename K, typename V>
	inline void stack<K, V>::aboutToModify(bool bIsStillShareable)
	{
		if (data_wrapper.use_count > 1 && bIsShareable)
		{
			// Make new wrapper. This should make the previous
			// wrapper object to go out of scope and call its 
			// destructor (RAII).
			data_wrapper = make_shared<stack_data<K, V>>
				(data_wrapper.get());
		}
		bIsShareable = bIsStillShareable ? true : false;
	}

}

#endif
