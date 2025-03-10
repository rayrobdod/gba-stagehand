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
	const std::vector<IN>& _backing;
	unsigned _word;
	unsigned _subword;
	subword_input_iterator(
		std::vector<IN>& backing,
		unsigned word,
		unsigned subword) :
			_backing(backing),
			_word(word),
			_subword(subword) {}

public:
	subword_input_iterator(const std::vector<IN>& backing) :
		_backing(backing), _word(0), _subword(0) {}

	subword_input_iterator(const subword_input_iterator& other) :
			_backing(other._backing),
			_word(other._word),
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
		IN buffer = (this->_backing)[_word];
		buffer &= mask << directional_offset;
		buffer >>= directional_offset;
		OUT retval(buffer);
		return retval;
	}

	subword_input_iterator<IN, OUT, DIR>& operator++() {
		_subword += bitsize<OUT>;
		if (_subword >= bitsize<IN>) {
			_subword = 0;
			++_word;
		}
		return *this;
	}

	subword_input_iterator<IN, OUT, DIR> operator++(int) {
		subword_input_iterator<IN, OUT, DIR> retval = *this;
		_subword += bitsize<OUT>;
		if (_subword >= bitsize<IN>) {
			_subword = 0;
			++_word;
		}
		return retval;
	}

	bool operator==(const subword_input_iterator<IN, OUT, DIR>& other) const {
		return
			this->_backing == other._backing &&
			this->_word == other._word &&
			this->_subword == other._subword;
	}

	bool operator!=(const subword_input_iterator<IN, OUT, DIR>& other) const {
		return !(*this == other);
	}

	subword_input_iterator<IN, OUT, DIR> operator+(unsigned delta) const {
		subword_input_iterator<IN, OUT, DIR> retval(*this);
		retval._subword += bitsize<OUT> * delta;
		retval._word += retval._subword / bitsize<IN>;
		retval._subword %= bitsize<IN>;
		return retval;
	}
};

#endif
