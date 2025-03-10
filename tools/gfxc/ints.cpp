#include "ints.hpp"

uint1_t::operator uint8_t() const {
	return this->value;
}
uint1_t::uint1_t(unsigned long long int x)
		: value(x & 1) {
}
uint1_t operator""_u1(unsigned long long int x) {
	uint1_t retval(x);
	return retval;
}

/////

uint2_t::operator uint8_t() const {
	return this->value;
}
uint2_t::uint2_t(unsigned long long int x)
		: value(x & 0x3) {
}
uint2_t operator""_u2(unsigned long long int x) {
	uint2_t retval(x);
	return retval;
}

/////

uint4_t::operator uint8_t() const {
	return this->value;
}
uint4_t::uint4_t(unsigned long long int x)
		: value(x & 0xF) {
}

uint4_t operator""_u4(unsigned long long int x) {
	uint4_t retval(x);
	return retval;
}
