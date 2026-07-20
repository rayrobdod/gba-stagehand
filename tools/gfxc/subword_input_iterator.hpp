#ifndef SUBWORD_INPUT_ITERATOR_HPP
#define SUBWORD_INPUT_ITERATOR_HPP

#include <cstdint>
#include <iterator>
#include <memory>
#include <vector>
#include "ints.hpp"

template<class IN, class OUT, enum Direction DIR>
	requires BitsizeIsMultiple<OUT, IN>
class subword_input_iterator {
private:
	std::vector<IN>::const_iterator _backing;
	unsigned _subword;

public:
	subword_input_iterator(const std::vector<IN>& backing) :
		_backing(backing.begin()), _subword(0) {}

	subword_input_iterator(const std::vector<IN>::const_iterator& backing) :
		_backing(backing), _subword(0) {}

	subword_input_iterator(const subword_input_iterator& other) :
			_backing(other._backing),
			_subword(other._subword) {}

	~subword_input_iterator() {}

	OUT operator*() {
		unsigned directional_offset;
		switch (DIR) {
		case DIRECTION_INC:
			directional_offset = _subword;
			break;
		case DIRECTION_DEC:
			directional_offset = bitsize<IN> - 1 - _subword;
			break;
		}

		const uint8_t mask = (1 << bitsize<OUT>) - 1;
		IN buffer = *(this->_backing);
		buffer &= mask << directional_offset;
		buffer >>= directional_offset;
		OUT retval(buffer);
		return retval;
	}

	subword_input_iterator<IN, OUT, DIR>& operator++() {
		_subword += bitsize<OUT>;
		if (_subword >= bitsize<IN>) {
			_subword = 0;
			++_backing;
		}
		return *this;
	}

	subword_input_iterator<IN, OUT, DIR> operator++(int) {
		subword_input_iterator<IN, OUT, DIR> retval = *this;
		_subword += bitsize<OUT>;
		if (_subword >= bitsize<IN>) {
			_subword = 0;
			++_backing;
		}
		return retval;
	}

	bool operator==(const subword_input_iterator<IN, OUT, DIR>& other) const {
		return
			this->_backing == other._backing &&
			this->_subword == other._subword;
	}

	bool operator!=(const subword_input_iterator<IN, OUT, DIR>& other) const {
		return !(*this == other);
	}

	bool operator<(const subword_input_iterator<IN, OUT, DIR>& other) const {
		if (this->_backing != other._backing) {
			return this->_backing < other._backing;
		} else {
			return this->_subword < other._subword;
		}
	}

	subword_input_iterator<IN, OUT, DIR> operator+(unsigned delta) const {
		subword_input_iterator<IN, OUT, DIR> retval(*this);
		retval += delta;
		return retval;
	}

	subword_input_iterator<IN, OUT, DIR>& operator+=(unsigned delta) {
		this->_subword += bitsize<OUT> * delta;
		this->_backing += this->_subword / bitsize<IN>;
		this->_subword %= bitsize<IN>;
		return *this;
	}
};

#endif
