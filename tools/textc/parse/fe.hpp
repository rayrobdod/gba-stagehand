#include <string>
#include <vector>
#include <utility>
#include "fmmap.hpp"

void parseFe(std::vector<std::pair<std::string, std::string>>& parsed, const fmmap& file);
void parseFe(std::vector<std::pair<std::string, std::string>>& parsed, const std::string_view& file);
