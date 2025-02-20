#include <cstdint>
#include <iterator>
#include <vector>

class bpp4_output_iterator_deref;

// : public std::iterator<std::output_iterator_tag, ????>
class bpp4_output_iterator {
private:
	std::vector<uint32_t> _backing;
	unsigned _word;
	unsigned _subword;

public:
	bpp4_output_iterator();

	bpp4_output_iterator_deref operator*();
	void operator++();

	std::vector<uint32_t> result();
};

class bpp4_output_iterator_deref {
private:
	std::vector<uint32_t>* _backing;
	unsigned _word;
	unsigned _subword;
	bpp4_output_iterator_deref(std::vector<uint32_t>* backing, unsigned word, unsigned subword);

public:
	void operator=(unsigned);

	friend bpp4_output_iterator_deref bpp4_output_iterator::operator*();
};
