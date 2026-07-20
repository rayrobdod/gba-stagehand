#include <algorithm>
#include <cstddef>
#include <vector>

template<
	class A,
	class Compare = std::equal_to<A>>
class partially_applied_equal_to {
	const A& a;

public:
	partially_applied_equal_to(const A& a)
			: a(a) {
	}
	bool operator()(const A& b) const {
		Compare cmp;
		return cmp(a, b);
	}
};

template<
	class Key,
	class Compare = std::equal_to<Key>,
	class Allocator = std::allocator<Key>>
class indexed_insert_only_set {
private:
	std::vector<Key, Allocator> _data;

public:
	indexed_insert_only_set(void) {
	}

	ssize_t find(const Key& a) const {
		auto it = std::find_if(this->_data.begin(), this->_data.end(), partially_applied_equal_to(a));
		if (it == this->_data.end())
			return -1;
		else
			return it - this->_data.begin();
	}

	size_t find_or_push_back(const Key& a) {
		ssize_t pos = this->find(a);
		if (pos < 0) {
			size_t retval = this->_data.size();
			this->_data.push_back(a);
			return retval;
		} else
			return pos;
	}

	size_t size() const {
		return this->_data.size();
	}
	Key operator[](unsigned index) const {
		return this->_data[index];
	}

	typename std::vector<Key, Allocator>::const_iterator cbegin() const {
		return this->_data.cbegin();
	}
	typename std::vector<Key, Allocator>::const_iterator cend() const {
		return this->_data.cend();
	}
	typename std::vector<Key, Allocator>::const_iterator begin() const {
		return this->_data.cbegin();
	}
	typename std::vector<Key, Allocator>::const_iterator end() const {
		return this->_data.cend();
	}
};
