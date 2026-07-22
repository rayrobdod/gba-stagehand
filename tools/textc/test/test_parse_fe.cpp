#include <cstdio>
#include <cstdint>
#include <string>
#include <vector>

bool currentTestFailed;
unsigned total;
unsigned failed;

void TEST_ASSERT_EQUAL_PARSE_RESULT(
		const std::vector<std::pair<std::string, std::string>>& expected,
		const std::vector<std::pair<std::string, std::string>>& actual) {
	if (expected != actual) {
		currentTestFailed = 1;
		printf("\033[41mFAIL\033[0m: ");
		if (expected.size() != actual.size()) {
			printf("Expected size of %zd; was size of %zd", expected.size(), actual.size());
		} else {
			for (size_t i = 0; i < expected.size(); i++) {
				if (expected[i].first != actual[i].first) {
					printf("At %zd's key: Expected [%s]; was [%s]", i, expected[i].first.c_str(), actual[i].first.c_str());
					break;
				}
				if (expected[i].second != actual[i].second) {
					printf("At %zd's value: Expected [%s]; was [%s]", i, expected[i].second.c_str(), actual[i].second.c_str());
					break;
				}
			}
		}
	}

}

void run_test(void (*fn)(void), const char* name) {
	printf("%s: ", name);
	currentTestFailed = 0;
	fn();
	if (currentTestFailed) {
		++failed;
	} else {
		printf("\033[42mPASS\033[0m");
	}
	++total;
	printf("\n");
}
#define RUN_TEST(func) run_test(func, #func);

//////////////////////////////////////

#include "parse/fe.hpp"
using namespace std::literals;

void test_empty(void) {
	std::vector<std::pair<std::string, std::string>> expected;

	std::vector<std::pair<std::string, std::string>> result;
	std::string_view input = "";
	parseFe(result, input);

	TEST_ASSERT_EQUAL_PARSE_RESULT(expected, result);
}

void test_single(void) {
	std::vector<std::pair<std::string, std::string>> expected;
	expected.emplace_back("greeting", "Hello World!\0"s);

	std::vector<std::pair<std::string, std::string>> result;
	std::string_view input =
		"##greeting\n"
		"Hello World!";
	parseFe(result, input);

	TEST_ASSERT_EQUAL_PARSE_RESULT(expected, result);
}

void test_two_strings(void) {
	std::vector<std::pair<std::string, std::string>> expected;
	expected.emplace_back("first", "Message The Frist\0"s);
	expected.emplace_back("second", "Message The Secnod\0"s);

	std::vector<std::pair<std::string, std::string>> result;
	std::string_view input =
		"##first\n"
		"Message The Frist\n"
		"##second\n"
		"Message The Secnod";
	parseFe(result, input);

	TEST_ASSERT_EQUAL_PARSE_RESULT(expected, result);
}

void test_macro_NL(void) {
	std::vector<std::pair<std::string, std::string>> expected;
	expected.emplace_back("lines", "Line 1\nLine 2\0"s);

	std::vector<std::pair<std::string, std::string>> result;
	std::string_view input =
		"##lines\n"
		"Line 1[NL]Line 2";
	parseFe(result, input);

	TEST_ASSERT_EQUAL_PARSE_RESULT(expected, result);
}

void test_newline_collapse(void) {
	// The prefix and suffix newlines are stripped
	// The middle newlines are turned into spaces
	std::vector<std::pair<std::string, std::string>> expected;
	expected.emplace_back("greeting", "Hello World!\0"s);

	std::vector<std::pair<std::string, std::string>> result;
	std::string_view input =
		"##greeting\n"
		"\n"
		"Hello\n"
		"World!\n";
	parseFe(result, input);

	TEST_ASSERT_EQUAL_PARSE_RESULT(expected, result);
}

void test_newline_collapse_does_not_place_a_space_after_explicit_NL(void) {
	std::vector<std::pair<std::string, std::string>> expected;
	expected.emplace_back("lines", "Line 1\nLine 2\0"s);

	std::vector<std::pair<std::string, std::string>> result;
	std::string_view input =
		"##lines\n"
		"Line 1[NL]\n"
		"Line 2";
	parseFe(result, input);

	TEST_ASSERT_EQUAL_PARSE_RESULT(expected, result);
}

void test_no_implicit_NUL_after_explicit_NUL(void) {
	std::vector<std::pair<std::string, std::string>> expected;
	expected.emplace_back("greeting", "Hello World!\0"s);

	std::vector<std::pair<std::string, std::string>> result;
	std::string_view input =
		"##greeting\n"
		"Hello World![0]";
	parseFe(result, input);

	TEST_ASSERT_EQUAL_PARSE_RESULT(expected, result);
}

int main() {
	total = 0;
	failed = 0;

	RUN_TEST(test_empty);
	RUN_TEST(test_single);
	RUN_TEST(test_two_strings);
	RUN_TEST(test_macro_NL);
	RUN_TEST(test_newline_collapse);
	RUN_TEST(test_newline_collapse_does_not_place_a_space_after_explicit_NL);
	RUN_TEST(test_no_implicit_NUL_after_explicit_NUL);

	printf("Total: %d; Failing: %d\n", total, failed);
	return 0 != failed;
}
