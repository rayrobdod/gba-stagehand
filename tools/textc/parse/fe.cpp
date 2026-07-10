#include "parse/fe.hpp"

#include <map>

using namespace std::literals;

static const std::initializer_list<std::pair<std::string_view, std::string_view>> macros_raw = {
	{"0"sv, "\x00"sv},
	{"NL"sv, "\n"sv},
	{"FF"sv, "\f"sv},
	{"HASH"sv, "#"sv},
};

enum class parseFeState {
	START_OF_LINE,
	VALUE,
	FIRST_HASH,
	HEADER,
	COMMENT,
	MACRO,
};

template<class R>
static void parseFeImpl(std::vector<std::pair<std::string, std::string>>& parsed, R file) {
	const std::map<std::string, std::string> macros(macros_raw.begin(), macros_raw.end());

	std::string key;
	std::string value;
	std::string macro;
	std::string pending;
	parseFeState state = parseFeState::START_OF_LINE;

	for (char c : file) {
		switch (state) {
		case parseFeState::START_OF_LINE :
			switch (c) {
			case '#':
				state = parseFeState::FIRST_HASH;
				break;
			case '\n':
				state = parseFeState::START_OF_LINE;
				break;
			case '[':
				state = parseFeState::MACRO;
				break;
			default:
				value += pending;
				pending.clear();
				value += c;
				break;
			}
			break;
		case parseFeState::VALUE :
			switch (c) {
			case '\n':
				state = parseFeState::START_OF_LINE;
				break;
			case '[':
				state = parseFeState::MACRO;
				break;
			default:
				value += pending;
				pending.clear();
				value += c;
				break;
			}
			break;
		case parseFeState::FIRST_HASH :
			switch (c) {
			case '#':
				state = parseFeState::HEADER;
				if (! key.empty())
					parsed.emplace_back(key, value);
				key.clear();
				value.clear();
				break;
			case '\n':
				state = parseFeState::START_OF_LINE;
				break;
			default:
				state = parseFeState::COMMENT;
				break;
			}
			break;
		case parseFeState::COMMENT :
			switch (c) {
			case '\n':
				state = parseFeState::START_OF_LINE;
				break;
			default:
				break;
			}
			break;
		case parseFeState::HEADER :
			switch (c) {
			case ' ':
				if (!key.empty()) {
					key += pending;
					pending.clear();
					key += c;
				}
				break;
			case '\n':
				state = parseFeState::START_OF_LINE;
				break;
			default:
				key += pending;
				pending.clear();
				key += c;
				break;
			}
			break;
		case parseFeState::MACRO :
			switch (c) {
			case ']':
				value += macros.at(macro);
				macro.clear();
				state = parseFeState::VALUE;
				break;
			default:
				macro += c;
				break;
			}
			break;
		}
	}
	if (! key.empty())
		parsed.emplace_back(key, value);
}

void parseFe(std::vector<std::pair<std::string, std::string>>& parsed, const fmmap& file) {
	parseFeImpl(parsed, file);
}
