#ifndef INTS_HPP
#define INTS_HPP

#include <cstdint>

struct uint1_t {
	uint8_t value : 1;
	operator uint8_t() const;
	explicit uint1_t(unsigned long long int);
};

uint1_t operator""_u1(unsigned long long int);

struct uint2_t {
	uint8_t value : 2;
	operator uint8_t() const;
	explicit uint2_t(unsigned long long int);
};

uint2_t operator""_u2(unsigned long long int);

struct uint4_t {
	uint8_t value : 4;
	operator uint8_t() const;
	explicit uint4_t(unsigned long long int);
};

uint4_t operator""_u4(unsigned long long int);

template<typename T>
constexpr unsigned bitsize;

template<>
constexpr unsigned bitsize<uint1_t> = 1;

template<>
constexpr unsigned bitsize<uint2_t> = 2;

template<>
constexpr unsigned bitsize<uint4_t> = 4;

template<>
constexpr unsigned bitsize<uint8_t> = 8;

template<>
constexpr unsigned bitsize<uint16_t> = 16;

template<>
constexpr unsigned bitsize<uint32_t> = 32;

enum Direction {
	DIRECTION_INC,
	DIRECTION_DEC,
};

template<typename WHOLE, typename PART>
concept BitsizeIsMultiple = requires {0 == (bitsize<WHOLE> % bitsize<PART>);};

#endif
