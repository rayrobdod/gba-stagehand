#ifndef SUBWORD_OUTPUT_ITERATOR_HPP
#define SUBWORD_OUTPUT_ITERATOR_HPP

#include <cstdint>
#include <iterator>
#include <memory>
#include <vector>

class subword_output_iterator_deref;

// : public std::iterator<std::output_iterator_tag, void, void, void, void>
class subword_output_iterator {
private:
	std::shared_ptr<std::vector<uint8_t>> _backing;
	unsigned _word;
	unsigned _subword;
	unsigned _width;
	subword_output_iterator(
		std::shared_ptr<std::vector<uint8_t>> backing,
		unsigned word,
		unsigned subword,
		unsigned width);

public:
	subword_output_iterator(unsigned width);
	~subword_output_iterator();

	subword_output_iterator_deref operator*();
	subword_output_iterator& operator++();
	subword_output_iterator operator++(int);

	std::vector<uint8_t> result();
};

class subword_output_iterator_deref {
private:
	std::shared_ptr<std::vector<uint8_t>> _backing;
	unsigned _word;
	unsigned _subword;
	unsigned _width;
	subword_output_iterator_deref(
		std::shared_ptr<std::vector<uint8_t>> backing,
		unsigned word,
		unsigned subword,
		unsigned width);

public:
	void operator=(unsigned);

	friend subword_output_iterator_deref subword_output_iterator::operator*();
};

#endif
