#include "compression/huff.hpp"

#include <cstdio>
#include <iostream>
#include <optional>
#include <queue>
#include <stack>
#include "subword_input_iterator.hpp"
#include "subword_output_iterator.hpp"

class prefix_tree_print_stack_elem final {
public:
	unsigned position;
	unsigned depth;
	bool is_leaf;
	prefix_tree_print_stack_elem(unsigned _position, unsigned _depth, unsigned _is_leaf)
			: position(_position), depth(_depth), is_leaf(_is_leaf) {
	}
};

template<typename DATASIZE>
static std::vector<uint8_t> decompressHuff(std::vector<uint8_t> src, bool decompile) {
	unsigned expected_size = src[1] | src[2] << 8 | src[3] << 16;
	subword_output_iterator<uint8_t, DATASIZE, DIRECTION_INC> dest;
	const auto dest_end = dest + expected_size * bitsize<uint8_t> / bitsize<DATASIZE>;

	if (decompile) {
		std::cout << std::endl;
		std::stack<prefix_tree_print_stack_elem> nodes;
		nodes.emplace(5, 0, false);

		while (! nodes.empty()) {
			auto node = nodes.top();
			nodes.pop();

			for (unsigned i = 0; i < node.depth; i++) {
				std::cout << ' ';
			}
			if (node.is_leaf) {
				char value[10] = {0};
				snprintf(value, sizeof(value), "0x%02x '%c'", src[node.position], src[node.position]);

				std::cout << "(L) " << value << std::endl;
			} else {
				unsigned off = src[node.position] & 0x3F;
				bool left_is_leaf = src[node.position] & 0x80;
				bool right_is_leaf = src[node.position] & 0x40;
				unsigned next_pos = (node.position & ~1) + off * 2 + 2;
				std::cout << "(B) " << left_is_leaf << " " << right_is_leaf << " " << off << "(" << next_pos << ")" << std::endl;

				nodes.emplace(next_pos + 1, node.depth + 1, right_is_leaf);
				nodes.emplace(next_pos, node.depth + 1, left_is_leaf);
			}
		}
	}

	unsigned bitstream_offset = src[4];

	std::vector<uint32_t> src32;

	if (bitstream_offset & 1) {
		for (auto i = src.begin() + 4 + ((bitstream_offset + 1) * 2); i + 3 < src.end(); i += 4) {
			uint32_t v = *(i + 0) | *(i + 1) << 8 | *(i + 2) << 16 | *(i + 3) << 24;
			src32.push_back(v);
		}
	} else {
		for (auto i = src.begin() + 2 + ((bitstream_offset + 1) * 2); i + 3 < src.end(); i += 4) {
			uint32_t v = *(i + 0) << 16 | *(i + 1) << 24 | *(i + 2) << 0 | *(i + 3) << 8;
			src32.push_back(v);
		}
	}

	subword_input_iterator<uint32_t, uint1_t, DIRECTION_DEC> src1(src32);

	while (dest != dest_end) {
		bool is_leaf = false;
		unsigned code_pos = 5;

		while (! is_leaf) {
			unsigned next_node = *(src1++);
			if (decompile)
				printf("%3d [%02x] | %d\n", code_pos, src[code_pos], next_node);
			is_leaf = (next_node ? src[code_pos] & 0x40 : src[code_pos] & 0x80);
			code_pos = (code_pos & ~1) + (src[code_pos] & 0x3F) * 2 + (next_node ? 3 : 2);
		}
		if (decompile)
			printf("%3d [%02x] | LEAF\n", code_pos, src[code_pos]);

		*dest = static_cast<DATASIZE>(src[code_pos]);
		dest++;
	}

	return dest.result();
}

std::vector<uint8_t> decompressHuff8(std::vector<uint8_t> src, bool decompile) {
	return decompressHuff<uint8_t>(src, decompile);
}

std::vector<uint8_t> decompressHuff4(std::vector<uint8_t> src, bool decompile) {
	return decompressHuff<uint4_t>(src, decompile);
}

///////

class prefix_code final {
public:
	unsigned length;
	unsigned value;
	prefix_code()
			: length(0), value(0) {
	}
	prefix_code prepend(unsigned v) {
		prefix_code retval = *this;
		retval.value <<= 1;
		retval.value |= static_cast<unsigned>(v);
		retval.length++;
		return retval;
	}
};

class prefix_tree;
class prefix_tree_node;

class prefix_tree_ptr final {
private:
	class prefix_tree* _backing;
	bool _is_leaf;
	size_t _index;

public:
	prefix_tree_ptr(
		class prefix_tree* backing,
		bool is_leaf,
		size_t index)
			: _backing(backing),
				_is_leaf(is_leaf),
				_index(index) {
	}
	prefix_tree_node* operator->() const;

	bool is_leaf() const {
		return this->_is_leaf;
	}

	template<typename LEAF_FN, typename BRANCH_FN>
	void fold(LEAF_FN leaf_fn, BRANCH_FN branch_fn) const;

	friend std::ostream& operator<<(std::ostream&, prefix_tree_ptr);
	friend class prefix_tree;
};
std::ostream& operator<<(std::ostream& os, prefix_tree_ptr v) {
	return os << "{" << v._backing << ", " << (v._is_leaf ? "LEAF" : "BRANCH") << ", index: " << v._index << "}";
}

class prefix_tree_node {
public:
	virtual ~prefix_tree_node(void){};
	virtual unsigned weight(void) const = 0;
	virtual std::optional<prefix_code> code(uint8_t value) const = 0;
	virtual unsigned count(void) const = 0;
	virtual void debug_print(std::ostream& os) const = 0;
};
class prefix_tree_leaf final : public prefix_tree_node {
	unsigned _weight;
	uint8_t _value;

public:
	prefix_tree_leaf(unsigned weight, uint8_t value)
			: _weight(weight), _value(value) {
	}
	virtual ~prefix_tree_leaf(void){};
	virtual unsigned weight(void) const {
		return this->_weight;
	}
	virtual std::optional<prefix_code> code(uint8_t value) const {
		std::optional<prefix_code> retval;
		if (value == this->_value) {
			retval.emplace();
		}
		return retval;
	}
	virtual unsigned count(void) const {
		return 1;
	}
	uint8_t value(void) const {
		return this->_value;
	}

	virtual void debug_print(std::ostream& os) const {
		char value[10] = {0};
		if (_value > 0x20)
			snprintf(value, sizeof(value), "0x%02x '%c'", _value, _value);
		else
			snprintf(value, sizeof(value), "0x%02x", _value);

		os << "{weight: " << _weight << ", value: " << value << "}";
	}
};
class prefix_tree_branch final : public prefix_tree_node {
	unsigned _weight;
	prefix_tree_ptr _left;
	prefix_tree_ptr _right;

public:
	prefix_tree_branch(prefix_tree_ptr left, prefix_tree_ptr right)
			: _weight(left->weight() + right->weight()), _left(left), _right(right) {
	}
	virtual ~prefix_tree_branch(void){};
	virtual unsigned weight(void) const {
		return this->_weight;
	}
	virtual std::optional<prefix_code> code(uint8_t value) const {
		std::optional<prefix_code> retval;
		std::optional<prefix_code> left = this->_left->code(value);
		if (left) {
			retval = std::make_optional(left->prepend(0));
		} else {
			std::optional<prefix_code> right = this->_right->code(value);
			if (right) {
				retval = std::make_optional(right->prepend(1));
			}
		}
		return retval;
	}
	virtual unsigned count(void) const {
		return 1 + this->_left->count() + this->_right->count();
	}
	prefix_tree_ptr left(void) const {
		return this->_left;
	}
	prefix_tree_ptr right(void) const {
		return this->_right;
	}

	virtual void debug_print(std::ostream& os) const {
		os << "{weight: " << _weight << ", left: " << _left << ", right: " << _right << "}";
	}
};
std::ostream& operator<<(std::ostream& os, prefix_tree_node* v) {
	v->debug_print(os);
	return os;
}

struct prefix_tree_serialize_queue_elem final {
	prefix_tree_ptr value;
	unsigned position;
	prefix_tree_serialize_queue_elem(prefix_tree_ptr _value, unsigned _position)
			: value(_value), position(_position) {
	}
};

class prefix_tree final {
private:
	prefix_tree_ptr _root;
	std::vector<prefix_tree_leaf> _leaves;
	std::vector<prefix_tree_branch> _branches;
	prefix_tree()
			: _root(this, false, 0) {
	}

public:
	prefix_code code(uint8_t value) const {
		return this->_root->code(value).value_or(prefix_code());
	}

	std::vector<uint8_t> serialize(void) const {
		std::vector<uint8_t> retval(_root->count());

		std::queue<prefix_tree_serialize_queue_elem> nodes;
		nodes.emplace(_root, 0);
		unsigned next_position = 1;

		while (! nodes.empty()) {
			prefix_tree_serialize_queue_elem node = nodes.front();
			nodes.pop();

			node.value.fold(
				[&retval, &node](const prefix_tree_leaf* leaf) {
					retval[node.position] = leaf->value();
				},
				[&retval, &nodes, &node, &next_position](const prefix_tree_branch* branch) {
					unsigned diff = ((next_position - 1) - node.position) / 2;
					bool left_is_leaf = branch->left().is_leaf();
					bool right_is_leaf = branch->right().is_leaf();

					retval[node.position] = diff | (left_is_leaf ? 0x80 : 0) | (right_is_leaf ? 0x40 : 0);

					nodes.emplace(branch->left(), next_position++);
					nodes.emplace(branch->right(), next_position++);
				});
		}

		return retval;
	}

	template<typename IT>
	friend prefix_tree make_prefix_tree_from_frequencies(IT begin, IT end);

	friend class prefix_tree_ptr;

	friend std::ostream& operator<<(std::ostream&, prefix_tree);
};
std::ostream& operator<<(std::ostream& os, prefix_tree v) {
	os << "{\n  " << v._root << ",\n  _leaves: {";
	for (unsigned i = 0; i < v._leaves.size(); i++) {
		os << "\n    [" << i << "] ";
		v._leaves[i].debug_print(os);
		os << ",";
	}
	os << "\n  },\n  _branches: {";
	for (unsigned i = 0; i < v._branches.size(); i++) {
		os << "\n    [" << i << "] ";
		v._branches[i].debug_print(os);
		os << ",";
	}
	os << "\n  }\n}";
	return os;
}

template<typename IT>
prefix_tree make_prefix_tree_from_frequencies(IT begin, IT end) {
	prefix_tree retval;
	std::vector<prefix_tree_ptr> active_nodes;

	IT frequencies = begin;
	for (unsigned value = 0; frequencies != end; ++value, ++frequencies) {
		if (0 == *frequencies)
			continue;
		retval._leaves.emplace_back(*frequencies, value);
	}
	while (retval._leaves.size() < 2) {
		retval._leaves.emplace_back(0, 0);
	}
	active_nodes.reserve(retval._leaves.size() + 1);

	for (unsigned i = 0; i < retval._leaves.size(); i++) {
		active_nodes.emplace_back(&retval, true, i);
	}
	retval._branches.reserve(retval._leaves.size());
	/*
	 * with enough space reserved, and with the vectors being append-only,
	 * there should be no reallocations, and thus no pointer invalidation
	 */

	while (active_nodes.size() > 1) {
		std::vector<prefix_tree_ptr>::iterator left = active_nodes.begin();
		std::vector<prefix_tree_ptr>::iterator right = active_nodes.begin() + 1;

		for (auto i = active_nodes.begin() + 2; i != active_nodes.end(); i++) {
			if ((*i)->weight() < (*left)->weight()) {
				left = i;
			} else if ((*i)->weight() < (*right)->weight()) {
				right = i;
			}
		}

		retval._branches.emplace_back(*left, *right);
		active_nodes.emplace_back(&retval, false, retval._branches.size() - 1);

		if (left > right)
			std::swap(left, right);
		unsigned left_off = left - active_nodes.begin();
		unsigned right_off = right - active_nodes.begin();

		active_nodes.erase(active_nodes.begin() + right_off);
		active_nodes.erase(active_nodes.begin() + left_off);
	}

	retval._root = active_nodes[0];
	return retval;
}

prefix_tree_node* prefix_tree_ptr::operator->() const {
	if (this->_is_leaf) {
		return this->_backing->_leaves.data() + this->_index;
	} else {
		return this->_backing->_branches.data() + this->_index;
	}
}

template<typename LEAF_FN, typename BRANCH_FN>
void prefix_tree_ptr::fold(LEAF_FN leaf_fn, BRANCH_FN branch_fn) const {
	if (this->_is_leaf) {
		leaf_fn(this->_backing->_leaves.data() + this->_index);
	} else {
		branch_fn(this->_backing->_branches.data() + this->_index);
	}
}

std::vector<uint8_t> compressHuff8(std::vector<uint8_t> src) {
	std::array<unsigned, 256> frequencies = {0};
	for (uint8_t i : src) {
		frequencies[i] += 1;
	}

	auto tree = make_prefix_tree_from_frequencies(frequencies.begin(), frequencies.end());

	subword_output_iterator<uint32_t, uint1_t, DIRECTION_DEC> stream;
	for (uint8_t i : src) {
		prefix_code code = tree.code(i);
		for (unsigned j = 0; j < code.length; j++) {
			*stream = (code.value & (1 << j) ? 1_u1 : 0_u1);
			stream++;
		}
	}

	std::vector<uint8_t> retval;
	retval.push_back(0x28);
	retval.push_back(src.size());
	retval.push_back(src.size() >> 8);
	retval.push_back(src.size() >> 16);

	std::vector<uint8_t> tree_ser = tree.serialize();
	while (tree_ser.size() % 4 != 3) {
		tree_ser.push_back(0);
	}
	retval.push_back((tree_ser.size() - 1) / 2);

	std::copy(tree_ser.begin(), tree_ser.end(), std::back_inserter(retval));

	for (uint32_t i : stream.result()) {
		retval.push_back(i);
		retval.push_back(i >> 8);
		retval.push_back(i >> 16);
		retval.push_back(i >> 24);
	}

	return retval;
}

std::vector<uint8_t> compressHuff4(std::vector<uint8_t> src) {
	std::array<unsigned, 16> frequencies = {0};
	subword_input_iterator<uint8_t, uint4_t, DIRECTION_INC> src4_begin(src);
	subword_input_iterator<uint8_t, uint4_t, DIRECTION_INC> src4_end = src4_begin + src.size() * bitsize<uint8_t> / bitsize<uint4_t>;

	for (auto i = src4_begin; i != src4_end; i++) {
		frequencies[*i] += 1;
	}

	auto tree = make_prefix_tree_from_frequencies(frequencies.begin(), frequencies.end());

	subword_output_iterator<uint32_t, uint1_t, DIRECTION_DEC> stream;
	for (auto i = src4_begin; i != src4_end; i++) {
		prefix_code code = tree.code(*i);
		for (unsigned j = 0; j < code.length; j++) {
			*stream = (code.value & (1 << j) ? 1_u1 : 0_u1);
			stream++;
		}
	}

	std::vector<uint8_t> retval;
	retval.push_back(0x24);
	retval.push_back(src.size());
	retval.push_back(src.size() >> 8);
	retval.push_back(src.size() >> 16);

	std::vector<uint8_t> tree_ser = tree.serialize();
	while (tree_ser.size() % 4 != 3) {
		tree_ser.push_back(0);
	}
	retval.push_back((tree_ser.size() - 1) / 2);

	std::copy(tree_ser.begin(), tree_ser.end(), std::back_inserter(retval));

	for (uint32_t i : stream.result()) {
		retval.push_back(i);
		retval.push_back(i >> 8);
		retval.push_back(i >> 16);
		retval.push_back(i >> 24);
	}

	return retval;
}
