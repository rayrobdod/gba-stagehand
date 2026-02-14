#include "compression/frit.hpp"

#include <array>
#include <algorithm>
#include <cstdio>
#include "ints.hpp"
#include "subword_output_iterator.hpp"

#define NUM_REGS (4)
#define LONGEST_RUN (1+14+16+255)


template<class WORD>
std::vector<WORD> decompressFrit(const std::vector<uint8_t> src, bool disassemble) {
	std::vector<WORD> retval;

	if (disassemble) {
		const unsigned size = ((src[3] << 16) | (src[2] << 8) | src[1]) / sizeof(WORD);
		printf("SIZE: %d\n", size);
	}
	std::array<WORD, NUM_REGS> regs = {0};

	unsigned srcPos = 4;

	while (srcPos < src.size()) {
		const unsigned initialSrcPos = srcPos;
		unsigned op = src[srcPos++];
		unsigned op_code = op >> 6;

		if (0 == op_code) {
			unsigned from = (op & 0x30) >> 4;
			unsigned to = (op & 0xC) >> 2;
			unsigned hi = (op & 0x2);
			unsigned low = (op & 0x1);
			unsigned hiValue = (hi ? src[srcPos++] << 8 : 0);
			unsigned lowValue = (low ? src[srcPos++] : 0);
			unsigned operand = hiValue | lowValue;

			if (disassemble) {
				char hiStr[8] = "N/A";
				char lowStr[8] = "N/A";
				if (hi)
					snprintf(hiStr, 8, "%02x", hiValue >> 8);
				if (low)
					snprintf(lowStr, 8, "%02x", lowValue);

				printf("  %04x %04x %04x %04x  %8d %8zd | XOR   %d->%d   %3s %3s\n",
					regs[0], regs[1], regs[2], regs[3], initialSrcPos, retval.size(), from, to, hiStr, lowStr);
			}

			regs[to] = regs[from] ^ operand;
		} else {
			signed delta = 0;
			if (1 == op_code) delta = -1;
			if (3 == op_code) delta = 1;

			unsigned reg = (op & 0x30) >> 4;
			unsigned length = (op & 0x0F) + 1;
			if (length == 0x10) {
				length = src[srcPos++] + 31;
			}

			if (disassemble) {
				char deltaStr = (delta == 0 ? '0' : delta == 1 ? '+' : '-');

				printf("  %04x %04x %04x %04x  %8d %8zd | RUN%c  %d    %3d\n",
					regs[0], regs[1], regs[2], regs[3], initialSrcPos, retval.size(), deltaStr, reg, length);
			}

			for (unsigned i = 0; i < length; i++) {
				retval.push_back(regs[reg]);
				regs[reg] += delta;
			}
		}
	}

	if (disassemble)
		printf("  %04x %04x %04x %04x  %8d %8zd | END\n\n",
			regs[0], regs[1], regs[2], regs[3], srcPos, retval.size());

	return retval;
}

template<class WORD>
struct Run {
	char delta;
	uint16_t length;
	WORD start;

	Run(char delta, uint16_t length, WORD start) : delta(delta), length(length), start(start) {}
	WORD end() {return this->start + this->length * this->delta;}
	WORD last() {return this->start + (this->length - 1) * this->delta;}
};

template<class WORD>
class run_with_start {
	const WORD needle;

public:
	run_with_start(const WORD needle)
			: needle(needle) {
	}
	bool operator()(const Run<WORD>& b) const {
		return b.start == needle;
	}
};


template<class WORD>
std::optional<std::vector<uint8_t>> compressFrit(const std::vector<WORD> src, uint8_t magic) {
	std::vector<Run<WORD>> runs;

	size_t srcPos = 0;

	while (srcPos < src.size()) {
		char delta = 0;
		uint16_t length = 1;
		const WORD start = src[srcPos];
		srcPos++;

		if (srcPos < src.size()) {
			WORD current = src[srcPos];

			if (current - start == 0) {
				while (srcPos < src.size() && src[srcPos] == start) {
					++srcPos;
					++length;
				}
			} else if (current - start == 1) {
				delta = 1;
				while (srcPos < src.size() && src[srcPos] == current) {
					++srcPos;
					++length;
					++current;
				}
			} else if (current - start == -1) {
				delta = -1;
				while (srcPos < src.size() && src[srcPos] == current) {
					++srcPos;
					++length;
					--current;
				}
			}
		}

		runs.emplace_back(delta, length, start);
	}


	/* run optimization pass */
	for (size_t runPos = 0; runPos < runs.size(); runPos++) {
		// The delta does not change the output of a 1-length run,
		// but changing the delta changes the run's end,
		// and a different end can better set up registers for future runs
		if (1 == runs[runPos].length) {
			auto next_Zero	= std::find_if(runs.begin() + 1 + runPos, runs.end(), run_with_start<WORD>(runs[runPos].start + 0));
			auto next_Pos	= std::find_if(runs.begin() + 1 + runPos, runs.end(), run_with_start<WORD>(runs[runPos].start + 1));
			auto next_Neg	= std::find_if(runs.begin() + 1 + runPos, runs.end(), run_with_start<WORD>(runs[runPos].start - 1));

			if (next_Pos < next_Neg && next_Pos < next_Zero) {
				runs[runPos].delta = 1;
			}
			if (next_Neg < next_Pos && next_Neg < next_Zero) {
				runs[runPos].delta = -1;
			}
		}

		if (runPos + 1 < runs.size() &&
			0 == runs[runPos].delta &&
			1 < runs[runPos].length &&
			0 != runs[runPos + 1].delta &&
			runs[runPos].start + runs[runPos + 1].delta == runs[runPos + 1].start &&
			true)
		{
			runs[runPos].length -= 1;
			runs[runPos+1].length += 1;
			runs[runPos+1].start -= runs[runPos+1].delta;
		}

		if (runPos + 1 < runs.size() &&
			0 != runs[runPos].delta &&
			1 < runs[runPos].length &&
			0 == runs[runPos + 1].delta &&
			runs[runPos].last() == runs[runPos + 1].start &&
			true)
		{
			runs[runPos].length -= 1;
			runs[runPos+1].length += 1;
		}
	}

	std::vector<uint8_t> retval;

	unsigned size_in_bytes = src.size() * sizeof(WORD);
	retval.push_back(magic);
	retval.push_back(size_in_bytes);
	retval.push_back(size_in_bytes >> 8);
	retval.push_back(size_in_bytes >> 16);

	std::array<WORD, NUM_REGS> regs = {0};

	for (auto run_it = runs.begin(); run_it != runs.end(); run_it++) {
		Run<WORD> run = *run_it;

		unsigned runReg;
	        // use a register that already has the correct start value
		for (runReg = 0; runReg < NUM_REGS; runReg++) {
			if (run.start == regs[runReg])
				break;
		}

	        // Otherwise, if two registers have the same value,
	        // use one of those two
		if (runReg >= NUM_REGS) {
			for (int i = 0; i < NUM_REGS; i++)
			for (int j = i + 1; j < NUM_REGS; j++) {
				if (regs[i] == regs[j]) {
					runReg = j;
				}
			}
		}

		// Otherwise, use the register whose value will be needed least soon,
		// or whose value will not be needed at all
		if (runReg >= NUM_REGS) {
			auto runRegNextUse = run_it;

			for (unsigned i = 0; i < NUM_REGS; i++) {
				auto iNextUse = std::find_if(run_it, runs.end(), run_with_start(regs[i]));
				if (iNextUse > runRegNextUse) {
					runRegNextUse = iNextUse;
					runReg = i;
				}
			}
		}

		if (runReg >= NUM_REGS) {
			runReg = 0;
		}

		if (run.start != regs[runReg]) {
			unsigned fromReg = runReg;
			for (unsigned i = 0; i < NUM_REGS; i++) {
				if ((run.start & 0xFF) == (regs[i] & 0xFF)) {
					fromReg = i;
					break;
				}
				if ((run.start & 0xFF00) == (regs[i] & 0xFF00)) {
					fromReg = i;
					break;
				}
			}

			unsigned hi = (run.start & 0xFF00) ^ (regs[fromReg] & 0xFF00);
			unsigned low = (run.start & 0xFF) ^ (regs[fromReg] & 0xFF);

			retval.push_back((fromReg << 4) | (runReg << 2) | (hi ? 2 : 0) | (low ? 1 : 0));
			if (hi)
				retval.push_back(hi >> 8);
			if (low)
				retval.push_back(low);
		}

		char opcode = (2 + run.delta) & 0x3;
		unsigned length = run.length;

		while (length > LONGEST_RUN)
		{
			retval.push_back((opcode << 6) | (runReg << 4) | 0xF);
			retval.push_back(0xFF);
			length -= LONGEST_RUN;
		}

		unsigned lengthSub1 = length - 1;

		if (lengthSub1 < 15) {
			retval.push_back((opcode << 6) | (runReg << 4) | lengthSub1);
		} else {
			unsigned lengthSub16 = lengthSub1 - 15;

			if (lengthSub16 < 15) {
				retval.push_back((opcode << 6) | (runReg << 4) | 0xE);
				retval.push_back((opcode << 6) | (runReg << 4) | lengthSub16);
			} else {
				unsigned lengthSub31 = lengthSub16 - 15;
				retval.push_back((opcode << 6) | (runReg << 4) | 0xF);
				retval.push_back(lengthSub31);
			}
		}

		regs[runReg] = run.end();
	}

	while (retval.size() % 4 != 0)
		retval.push_back(0);

	return std::make_optional(retval);
}


std::vector<uint8_t> decompressFrit16(std::vector<uint8_t> src, bool disassemble) {
	std::vector<uint16_t> words(decompressFrit<uint16_t>(src, disassemble));
	std::vector<uint8_t> retval;
	for (uint16_t word : words) {
		retval.push_back(word);
		retval.push_back(word >> 8);
	}
	return retval;
}

std::vector<uint8_t> decompressFrit8(std::vector<uint8_t> src, bool disassemble) {
	return decompressFrit<uint8_t>(src, disassemble);
}

std::optional<std::vector<uint8_t>> compressFrit16(std::vector<uint8_t> src) {
	if (0 != src.size() % 2)
		return std::nullopt;

	subword_output_iterator<uint16_t, uint8_t, DIRECTION_INC> src_to_word;
	for (uint8_t b : src) {
		*src_to_word = b;
		src_to_word++;
	}

	return compressFrit<uint16_t>(src_to_word.result(), 0x42);
}

std::optional<std::vector<uint8_t>> compressFrit8(std::vector<uint8_t> src) {
	return compressFrit<uint8_t>(src, 0x41);
}
