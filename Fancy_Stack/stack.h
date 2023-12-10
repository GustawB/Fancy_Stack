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
	using std::move;
	using std::shared_ptr;
	using std::make_shared;
	using std::pair;

	// Every stack will be pointing to the stack data object,
	// and if they share it and one modified it, then we 
	// create a new data for it.
	template <typename K, typename V> class stack_data
	{
		using element_list = list<pair<K, V>>;
		using element_iterator = typename element_list::iterator;
	public:
		map < const K*,
			list<element_iterator>,
			decltype([](auto* a, auto* b) { return *a < *b; }) > elements_by_key;
		element_list elements;

	public:
		stack_data();
		~stack_data() = default;

		// Used when we need to split memory.
		stack_data(const stack_data& other);
	};

	template <typename K, typename V>
	stack_data<K, V>::stack_data() : elements_by_key{},
		elements{}
	{}

	template <typename K, typename V>
	stack_data<K, V>::stack_data(const stack_data<K, V>& other)
		: elements_by_key{},
		elements{ other.elements }
	{
		for (auto it = elements.begin(); it != elements.end(); it++)
		{
			elements_by_key[&it->first].push_back(it);
		}
	}


	template <typename K, typename V> class stack
	{
		shared_ptr<stack_data<K, V>> data_wrapper;
		// Flag used to determine whetherwe can share memory or not.
		bool bIsShareable = true;
	public:
		stack();
		stack(stack const&); //copy constructor;
		stack(stack&&); // move constructor;

		stack& operator=(stack);

		void push(K const&, V const&);

		void pop();

		void pop(K const&);

		void clear();

		size_t size() const;
		size_t count(K const&) const;

		std::pair<K const&, V&> front();
		std::pair<K const&, V const&> front() const;

		V& front(K const&);
		V const& front(K const&) const;

	private:
		void aboutToModify(bool);
	};

	template<typename K, typename V>
	stack<K, V>::stack()
		: data_wrapper{make_shared<stack_data<K, V>>()}
	{}

	template<typename K, typename V>
	stack<K, V>::stack(stack const& other)
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
				(*other.data_wrapper.get());
		}
	}

	template<typename K, typename V>
	inline stack<K, V>::stack(stack&& other)
		: data_wrapper{ move(other.data_wrapper) }
	{}

	template<typename K, typename V>
	inline void stack<K, V>::push(K const& k, V const& v)
	{
		aboutToModify(false);
		auto& elements = data_wrapper->elements;
		auto it = elements.insert(elements.end(), { k, v });
		data_wrapper->elements_by_key[&it->first].push_back(it);
	}

	template<typename K, typename V>
	inline void stack<K, V>::pop() {
		aboutToModify(false);
		if (data_wrapper->elements.empty())
		{
			throw std::invalid_argument("can't pop from empty stack");
		}
		auto& element = data_wrapper->elements.back();
		data_wrapper->elements_by_key[&element.first].pop_back();
		data_wrapper->elements.pop_back();
	}

	template<typename K, typename V>
	inline void stack<K, V>::clear()
	{
		aboutToModify(false);
		data_wrapper->elements.clear();
		data_wrapper->elements_by_key.clear();
	}

	template<typename K, typename V>
	inline size_t stack<K, V>::size() const
	{
		return data_wrapper->elements.size();
	}

	template<typename K, typename V>
	inline size_t stack<K, V>::count(K const& key) const {
		return data_wrapper->elements_by_key.at(&key).size();
	}

	template<typename K, typename V>
	inline std::pair<K const&, V&> stack<K, V>::front()
	{
		if (data_wrapper->elements.size() == 0)
		{
			throw std::invalid_argument("no element in the stack");
		}
		aboutToModify(true);
		std::pair<K const&, V&> result{ data_wrapper->elements.back().first,
		data_wrapper->elements.back().second };

		return result;
	}

	template<typename K, typename V>
	inline std::pair<K const&, V const&> stack<K, V>::front() const
	{
		if (data_wrapper->elements.size() == 0)
		{
			throw std::invalid_argument("no element in the stack");
		}
		std::pair<K const&, V& const> result
		{ data_wrapper->elements.back().first,
		data_wrapper->elements.back().second };

		return result;
	}

	template<typename K, typename V>
	inline V& stack<K, V>::front(K const& key)
	{
		auto it = data_wrapper->elements_by_key.find(&key);
		if (it == data_wrapper->elements_by_key.end())
		{
			throw std::invalid_argument("no element of given key in the stack");
		}
		aboutToModify(true);

		return it->second.back()->second;
	}

	template<typename K, typename V>
	inline V const& stack<K, V>::front(K const& key) const
	{
		auto it = data_wrapper->elements_by_key.find(&key);
		if (it == data_wrapper->elements_by_key.end())
		{
			throw std::invalid_argument("no element of given key in the stack");
		}

		return it->second.back()->second;
	}

	template<typename K, typename V>
	inline stack<K, V>& stack<K, V>::operator=(stack other)
	{
		if (this == &other) { return *this; } // check for self-assignment.
		if (other.bIsShareable)
		{
			data_wrapper = other.data_wrapper;
		}
		else
		{
			data_wrapper = make_shared<stack_data<K, V>>
				(*other.data_wrapper.get());
		}

		return * this;
	}

	template<typename K, typename V>
	inline void stack<K, V>::aboutToModify(bool bIsStillShareable)
	{
		if (data_wrapper.use_count() > 1 && bIsShareable)
		{
			// Make new wrapper. This should make the previous
			// wrapper object to go out of scope and call its 
			// destructor (RAII).
			data_wrapper = make_shared<stack_data<K, V>>
				(*data_wrapper.get());
		}
		bIsShareable = bIsStillShareable ? false : true;
	}
}

#endif
