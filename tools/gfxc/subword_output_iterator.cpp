#include "subword_output_iterator.hpp"

#include <stdexcept>

subword_output_iterator::subword_output_iterator(
	std::shared_ptr<std::vector<uint32_t>> backing,
	unsigned word,
	unsigned subword,
	unsigned width)
		: _backing(backing),
			_word(word),
			_subword(subword),
			_width(width) {
}

subword_output_iterator::subword_output_iterator(unsigned width)
		: _word(0), _subword(0), _width(width) {
	_backing = std::make_shared<std::vector<uint32_t>>();
	if (0 != 32 % width) {
		throw new std::invalid_argument("subword_output_iterator only supports power-of-two widths less than 32");
	}
}

subword_output_iterator::~subword_output_iterator() {
}

subword_output_iterator& subword_output_iterator::operator++() {
	_subword += _width;
	if (_subword >= 32) {
		_subword = 0;
		_word++;
	}
	return *this;
}

subword_output_iterator subword_output_iterator::operator++(int) {
	subword_output_iterator retval = *this;
	_subword += _width;
	if (_subword >= 32) {
		_subword = 0;
		_word++;
	}
	return retval;
}

subword_output_iterator_deref subword_output_iterator::operator*() {
	return subword_output_iterator_deref(_backing, _word, _subword, _width);
}

std::vector<uint32_t> subword_output_iterator::result() {
	return *(this->_backing);
}

////

subword_output_iterator_deref::subword_output_iterator_deref(
	std::shared_ptr<std::vector<uint32_t>> backing,
	unsigned word,
	unsigned subword,
	unsigned width)
		: _backing(backing),
			_word(word),
			_subword(subword),
			_width(width) {
}

void subword_output_iterator_deref::operator=(unsigned new_value) {
	while (this->_backing->size() <= _word) {
		this->_backing->push_back(0);
	}

	uint32_t mask = (1 << _width) - 1;

	uint32_t buffer = (*(this->_backing))[_word];
	buffer &= ~(mask << _subword);
	buffer |= ((new_value & mask) << _subword);
	(*(this->_backing))[_word] = buffer;
}
