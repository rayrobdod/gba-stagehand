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

	const std::locale locale = std::locale();
	const auto& facet = std::use_facet<std::ctype<char>>(locale);

	std::string key;
	std::string value;
	std::string macro;
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
				if (!value.empty() && !facet.is(std::ctype_base::space, value.back()))
					value.push_back(' ');
				value += c;
				state = parseFeState::VALUE;
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
				value += c;
				break;
			}
			break;
		case parseFeState::FIRST_HASH :
			switch (c) {
			case '#':
				state = parseFeState::HEADER;
				if ('\0' != value.back())
					value.push_back('\0');
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
					key += c;
				}
				break;
			case '\n':
				state = parseFeState::START_OF_LINE;
				break;
			default:
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
	if ('\0' != value.back())
		value.push_back('\0');
	if (! key.empty())
		parsed.emplace_back(key, value);
}

void parseFe(std::vector<std::pair<std::string, std::string>>& parsed, const fmmap& file) {
	parseFeImpl(parsed, file);
}

void parseFe(std::vector<std::pair<std::string, std::string>>& parsed, const std::string_view& file) {
	parseFeImpl(parsed, file);
}
