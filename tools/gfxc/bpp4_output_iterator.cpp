#include "bpp4_output_iterator.hpp"

bpp4_output_iterator::bpp4_output_iterator()
		: _word(0), _subword(0) {
}

void bpp4_output_iterator::operator++() {
	_subword += 4;
	if (_subword >= 32) {
		_subword = 0;
		_word++;
	}
}

bpp4_output_iterator_deref bpp4_output_iterator::operator*() {
	return bpp4_output_iterator_deref(&_backing, _word, _subword);
}

std::vector<uint32_t> bpp4_output_iterator::result() {
	return this->_backing;
}

////

bpp4_output_iterator_deref::bpp4_output_iterator_deref(std::vector<uint32_t>* backing, unsigned word, unsigned subword)
		: _backing(backing), _word(word), _subword(subword) {
}

void bpp4_output_iterator_deref::operator=(unsigned new_value) {
	while (this->_backing->size() <= _word) {
		this->_backing->push_back(0);
	}

	uint32_t buffer = (*(this->_backing))[_word];
	buffer &= ~(0xF << _subword);
	buffer |= ((new_value & 0xF) << _subword);
	(*(this->_backing))[_word] = buffer;
}
