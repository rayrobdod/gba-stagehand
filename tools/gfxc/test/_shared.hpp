#include <cstdint>
#include <string>
#include <vector>

void TEST_ASSERT_EQUAL_VECTOR_HEX8(std::vector<uint8_t> expected, std::vector<uint8_t> actual);
void TEST_ASSERT_EQUAL_UNSIGNED(unsigned expected, unsigned actual);
void TEST_FAIL(std::string_view reason);

extern unsigned total;
extern unsigned failed;

void run_test(void (*fn)(void), const char* name);

#define RUN_TEST(func) run_test(func, #func);
