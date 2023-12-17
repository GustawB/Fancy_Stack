#ifndef STACK_H
#define STACK_H

#include <iterator>
#include <cstddef>  // ptrdiff_t
#include <memory>
#include <stdexcept> // required for VS
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
	using std::ptrdiff_t;
	using std::forward_iterator_tag;

	// Every stack will be pointing to the stack data object,
	// and if they share it and one modified it, then we 
	// create a new data for it.
	template <typename K, typename V> class stack_data
	{
		//using element_list = list<pair<K, V>>;
		//using element_list = list<V>;
		using element_map = map<K, list<V>>;
		using element_iterator = typename list<V>::iterator;
		using element_by_key_iterator = typename element_map::iterator;
		using element_list = list<pair<element_by_key_iterator, element_iterator>>;
		using element_list_iterator = element_list::iterator;
	public:
		element_map elements_by_key;
		element_list elements;
		map < element_by_key_iterator, list<element_list_iterator>,
			decltype([](element_by_key_iterator a, element_by_key_iterator b)
				{ return a->first < b->first; }) > key_to_list_map;

		stack_data();
		~stack_data() = default;

		// Used when we need to split memory.
		stack_data(const stack_data& other);
	};

	template <typename K, typename V>
	stack_data<K, V>::stack_data() : elements_by_key{},
		elements{}, key_to_list_map{}
	{}

	template <typename K, typename V>
	stack_data<K, V>::stack_data(const stack_data<K, V>& other)
		: elements_by_key{}, elements{}, key_to_list_map{}
	{
		for (auto iter = other.elements.begin();
			iter != other.elements.end(); ++iter)
		{
			auto map_pair = iter->first;
			elements_by_key[map_pair->first].push_back(*(iter->second));
			auto key_iter = elements_by_key.find(map_pair->first);
			auto value_iter = elements_by_key[map_pair->first].end();
			--value_iter;
			elements.push_back(pair{ key_iter, value_iter });
			auto list_iter = elements.end();
			--list_iter;
			key_to_list_map[key_iter].push_back(list_iter);
		}
	}

	template<typename Stack, typename StackData>
	class modify_guard;

	template <typename K, typename V> class stack
	{
		shared_ptr<stack_data<K, V>> data_wrapper;
		// Flag used to determine whetherwe can share memory or not.
		bool bIsShareable = true;
		friend modify_guard<stack<K, V>, stack_data<K, V>>;
	public:
		stack();
		stack(stack const&); //copy constructor;
		stack(stack&&) noexcept; // move constructor;
		~stack() noexcept = default;

		stack& operator=(stack);

		void push(K const&, V const&);

		void pop();

		void pop(K const&);

		void clear();

		size_t size() const noexcept;
		size_t count(K const&) const noexcept;

		std::pair<K const&, V&> front();
		std::pair<K const&, V const&> front() const;

		V& front(K const&);
		V const& front(K const&) const;

	public:
		class const_iterator
		{
		public:
			using iterator_category = forward_iterator_tag;
			using value_type = K;
			using difference_type = ptrdiff_t;
			using pointer = const value_type*;
			using reference = const value_type&;

		private:
			map<K, list<V>>::iterator ptr;

		public:
			const_iterator(map<K, list<V>>::iterator p) : ptr(p)
			{}

			reference operator*() noexcept
			{
				return ptr->first;
			}

			pointer operator->() noexcept
			{
				return *ptr->first;
			}

			const_iterator& operator++() noexcept // ++iterator;
			{
				++ptr;
				return *this;
			}

			const_iterator operator++(int) noexcept // iterator++
			{
				const_iterator result(*this);
				operator++();
				return result;
			}

			friend bool operator==(const const_iterator& a,
				const const_iterator& b) noexcept
			{
				return a.ptr == b.ptr;
			}

			friend bool operator!= (const const_iterator& a,
				const const_iterator& b) noexcept
			{
				return !(a == b);
			}
		};

		const_iterator cbegin() noexcept
		{
			auto map_beg = data_wrapper->elements_by_key.begin();
			return const_iterator(map_beg);
		}

		const_iterator cend() noexcept
		{
			auto map_end = data_wrapper->elements_by_key.end();
			return const_iterator(map_end);
		}
	};

	template<typename K, typename V>
	stack<K, V>::stack()
		: data_wrapper{ make_shared<stack_data<K, V>>() }
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
			data_wrapper = make_shared<stack_data<K, V>>(*other.data_wrapper);
		}
	}

	template<typename K, typename V>
	inline stack<K, V>::stack(stack&& other) noexcept
		: data_wrapper{ move(other.data_wrapper) }
	{}

	static bool map_access_throw = false;
	static bool push_back_throw = false;
	static bool modify_guard_throw = false;

	template<typename Map>
	class map_access_guard {
		using K = Map::key_type;
		using V = Map::mapped_type;
	public:
		map_access_guard(Map& map, K const& key) : map(map)
		{
			if (map_access_throw) throw std::bad_alloc();
			auto p = map.insert({ key, V() });
			it = p.first;
			rollback = p.second;
		}
		~map_access_guard()
		{
			if (rollback)
			{
				map.erase(it);
			}
		}
		V& operator()() noexcept
		{
			return it->second;
		}
		Map::iterator iter() noexcept
		{
			return it;
		}
		void drop_rollback() noexcept
		{
			rollback = false;
		}

	private:
		bool rollback;
		Map& map;
		Map::iterator it;
	};

	template<typename Container>
	class push_back_guard {
		using V = Container::value_type;
	public:
		push_back_guard(Container& container, V const& value)
			: container(container)
		{
			if (push_back_throw) throw std::bad_alloc();
			container.push_back(value);
			rollback = true;
		}
		~push_back_guard()
		{
			if (rollback)
			{
				container.pop_back();
			}
		}
		void drop_rollback() noexcept
		{
			rollback = false;
		}
	private:
		bool rollback;
		Container& container;
	};

	template<typename Stack, typename StackData>
	class modify_guard {
	public:
		modify_guard(Stack& stack, bool bIsStillShareable)
			: stack(stack)
		{
			if (modify_guard_throw) throw std::bad_alloc();
			this->rollback = true;
			this->data = stack.data_wrapper;
			this->bIsShareable = stack.bIsShareable;
			if (stack.data_wrapper.use_count() > 2 && bIsShareable)
			{
				// Make new wrapper. This should make the previous
				// wrapper object to go out of scope and call its 
				// destructor (RAII).
				stack.data_wrapper =
					make_shared<StackData>(*stack.data_wrapper);
			}
			stack.bIsShareable = bIsStillShareable ? true : false;
		}
		~modify_guard()
		{
			if (rollback)
			{
				stack.bIsShareable = bIsShareable;
				stack.data_wrapper = data;
			}
		}
		void drop_rollback()
		{
			rollback = false;
		}
	private:
		Stack& stack;
		shared_ptr<StackData> data;
		bool bIsShareable;
		bool rollback;
	};

	template<typename K, typename V>
	inline void stack<K, V>::push(K const& key, V const& value)
	{
		modify_guard<stack<K, V>, stack_data<K, V>> guard(*this, true);
		map_access_guard elements_by_key(
			data_wrapper->elements_by_key,
			key
		);
		push_back_guard push_value(
			elements_by_key(),
			value
		);
		//THANK GOD find() works in log(n);
		auto value_iter = elements_by_key().end();
		// this will get us the iterator to the last element in the list.
		--value_iter;
		push_back_guard push_element(
			data_wrapper->elements,
			pair{ elements_by_key.iter(), value_iter }
		);
		auto list_iter = data_wrapper->elements.end();
		--list_iter;
		map_access_guard key_to_list_map(
			data_wrapper->key_to_list_map,
			elements_by_key.iter()
		);
		push_back_guard push_list(
			key_to_list_map(),
			list_iter
		);
		guard.drop_rollback();
		elements_by_key.drop_rollback();
		push_value.drop_rollback();
		push_element.drop_rollback();
		key_to_list_map.drop_rollback();
		push_list.drop_rollback();
	}

	template<typename K, typename V>
	inline void stack<K, V>::pop() {
		if (data_wrapper->elements.empty())
		{
			throw std::invalid_argument("can't pop from empty stack");
		}
		modify_guard<stack<K, V>, stack_data<K, V>> guard(*this, true);
		auto elements_last_item = data_wrapper->elements.back();
		auto map_iter = elements_last_item.first;
		auto value_iter = elements_last_item.second;
		K key = map_iter->first;
		auto pop_iter = data_wrapper->key_to_list_map[map_iter].end();
		--pop_iter;
		data_wrapper->key_to_list_map[map_iter].erase(pop_iter);
		if (data_wrapper->key_to_list_map[map_iter].empty())
		{
			data_wrapper->key_to_list_map.erase(map_iter);
		}

		data_wrapper->elements_by_key[key].erase(value_iter);
		if (data_wrapper->elements_by_key[key].empty())
		{
			data_wrapper->elements_by_key.erase(key);
		}

		data_wrapper->elements.pop_back();
		guard.drop_rollback();
	}

	template<typename K, typename V>
	inline void stack<K, V>::pop(K const& key) {
		if (data_wrapper->elements_by_key[key].empty())
		{
			throw std::invalid_argument("no element with given key");
		}
		modify_guard<stack<K, V>, stack_data<K, V>> guard(*this, true);
		auto map_iter = data_wrapper->elements_by_key.find(key);
		auto pop_iter = data_wrapper->key_to_list_map[map_iter].back();
		data_wrapper->elements.erase(pop_iter);

		auto key_to_list_end = data_wrapper->key_to_list_map[map_iter].end();
		--key_to_list_end;
		data_wrapper->key_to_list_map[map_iter].erase(key_to_list_end);
		if (data_wrapper->key_to_list_map[map_iter].empty())
		{
			data_wrapper->key_to_list_map.erase(map_iter);
		}

		auto by_key_end = data_wrapper->elements_by_key[key].end();
		--by_key_end;
		data_wrapper->elements_by_key[key].erase(by_key_end);
		if (data_wrapper->elements_by_key[key].empty())
		{
			data_wrapper->elements_by_key.erase(key);
		}
		guard.drop_rollback();
	}

	template<typename K, typename V>
	inline void stack<K, V>::clear()
	{
		modify_guard<stack<K, V>, stack_data<K, V>> guard(*this, true);
		data_wrapper->elements.clear();
		data_wrapper->elements_by_key.clear();
		data_wrapper->key_to_list_map.clear();
		guard.drop_rollback();
	}

	template<typename K, typename V>
	inline size_t stack<K, V>::size() const noexcept
	{
		return data_wrapper->elements.size();
	}

	template<typename K, typename V>
	inline size_t stack<K, V>::count(K const& key) const noexcept {
		if (!data_wrapper->elements_by_key.contains(key))
		{
			return 0;
		}
		return data_wrapper->elements_by_key[key].size();
	}

	template<typename K, typename V>
	inline std::pair<K const&, V&> stack<K, V>::front()
	{
		if (data_wrapper->elements.size() == 0)
		{
			throw std::invalid_argument("no element in the stack");
		}
		modify_guard<stack<K, V>, stack_data<K, V>> guard(*this, false);
		const K& key = data_wrapper->elements.back().first->first;
		std::pair<K const&, V&> result{ key,
			*(data_wrapper->elements.back().second) };
		guard.drop_rollback();
		return result;
	}

	template<typename K, typename V>
	inline std::pair<K const&, V const&> stack<K, V>::front() const
	{
		if (data_wrapper->elements.size() == 0)
		{
			throw std::invalid_argument("no element in the stack");
		}
		const K& key = data_wrapper->elements.back().first->first;
		std::pair<K const&, V const&> result{ key,
			*(data_wrapper->elements.back().second) };

		return result;
	}

	template<typename K, typename V>
	inline V& stack<K, V>::front(K const& key)
	{
		if (data_wrapper->elements_by_key[key].empty())
		{
			throw std::invalid_argument("no element of given key in the stack");
		}
		modify_guard<stack<K, V>, stack_data<K, V>> guard(*this, false);
		guard.drop_rollback();
		return data_wrapper->elements_by_key[key].back();
	}

	template<typename K, typename V>
	inline V const& stack<K, V>::front(K const& key) const
	{
		if (data_wrapper->elements_by_key[key].empty())
		{
			throw std::invalid_argument("no element of given key in the stack");
		}

		return data_wrapper->elements_by_key[key].back();
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
			data_wrapper = make_shared<stack_data<K, V>>(*other.data_wrapper);
		}

		return *this;
	}
}

#endif