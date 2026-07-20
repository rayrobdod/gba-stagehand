#ifndef SUBWORD_OUTPUT_ITERATOR_HPP
#define SUBWORD_OUTPUT_ITERATOR_HPP

#include <cstdint>
#include <iterator>
#include <memory>
#include <vector>
#include "ints.hpp"

template<class OUT, class IN, enum Direction DIR>
	requires BitsizeIsMultiple<OUT, IN>
class subword_output_iterator_deref;

// : public std::iterator<std::output_iterator_tag, void, void, void, void>
template<class OUT, class IN, enum Direction DIR>
	requires BitsizeIsMultiple<OUT, IN>
class subword_output_iterator {
private:
	std::shared_ptr<std::vector<OUT>> _backing;
	unsigned _word;
	unsigned _subword;
	subword_output_iterator(
		std::shared_ptr<std::vector<OUT>> backing,
		unsigned word,
		unsigned subword) :
			_backing(backing),
			_word(word),
			_subword(subword){}

public :
	subword_output_iterator() : _word(0), _subword(0) {
		_backing = std::make_shared<std::vector<OUT>>();
	}

	subword_output_iterator(const subword_output_iterator& other) :
			_backing(other._backing),
			_word(other._word),
			_subword(other._subword) {}

	~subword_output_iterator() {
	}

	subword_output_iterator_deref<OUT, IN, DIR> operator*() {
		return subword_output_iterator_deref<OUT, IN, DIR>(_backing, _word, _subword);
	}
	subword_output_iterator<OUT, IN, DIR>& operator++() {
		_subword += bitsize<IN>;
		if (_subword >= bitsize<OUT>) {
			_subword = 0;
			_word++;
		}
		return *this;
	}
	subword_output_iterator<OUT, IN, DIR> operator++(int) {
		subword_output_iterator retval = *this;
		_subword += bitsize<IN>;
		if (_subword >= bitsize<OUT>) {
			_subword = 0;
			_word++;
		}
		return retval;
	}

	bool operator==(const subword_output_iterator<OUT, IN, DIR>& other) const {
		return
			this->_backing == other._backing &&
			this->_word == other._word &&
			this->_subword == other._subword;
	}

	bool operator!=(const subword_output_iterator<OUT, IN, DIR>& other) const {
		return !(*this == other);
	}

	subword_output_iterator<OUT, IN, DIR> operator+(unsigned delta) const {
		subword_output_iterator<OUT, IN, DIR> retval = *this;
		retval += delta;
		return retval;
	}

	subword_output_iterator<OUT, IN, DIR>& operator+=(unsigned delta) {
		this->_subword += bitsize<IN> * delta;
		this->_word += this->_subword / bitsize<OUT>;
		this->_subword %= bitsize<OUT>;
		return *this;
	}

	std::vector<OUT> result() {
		return *(this->_backing);
	}

	std::vector<OUT>::size_type size() {
		return this->_backing->size();
	}
};

template<class OUT, class IN, enum Direction DIR>
	requires BitsizeIsMultiple<OUT, IN>
class subword_output_iterator_deref {
private:
	std::shared_ptr<std::vector<OUT>> _backing;
	unsigned _word;
	unsigned _subword;
	subword_output_iterator_deref(
		std::shared_ptr<std::vector<OUT>> backing,
		unsigned word,
		unsigned subword)
			: _backing(backing),
			_word(word),
			_subword(subword) {
	}

public:
	void operator=(IN new_value) {
		while (this->_backing->size() <= _word) {
			this->_backing->push_back(0);
		}

		unsigned directional_offset;
		switch (DIR) {
		case DIRECTION_INC:
			directional_offset = _subword;
			break;
		case DIRECTION_DEC:
			directional_offset = bitsize<OUT> - 1 - _subword;
			break;
		}

		const OUT mask = (1 << bitsize<IN>)-1;

		OUT buffer = (*(this->_backing))[_word];
		buffer &= ~(mask << directional_offset);
		OUT new_value2(new_value);
		buffer |= ((new_value2 & mask) << directional_offset);
		(*(this->_backing))[_word] = buffer;
	}

	friend class subword_output_iterator<OUT, IN, DIR>;
};

#endif
