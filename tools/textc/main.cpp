#include <cstdio>
#include <iostream>
#include <fstream>
#include "fmmap.hpp"
#include "object.hpp"
#include "object_x8664.hpp"
#include "parse/fe.hpp"

int main(int argc, char** argv) {
	if (argc < 5) {
		printf("%s objfile headerfile hostobjfile out", argv[0]);
		return 0;
	}

	std::vector<std::pair<std::string, std::string>> strings;
	for (int i = 4; i < argc; i++) {
		std::filesystem::path filename(argv[i]);
		fmmap filecontent(filename);
		if (".fescript" == filename.extension()) {
			parseFe(strings, filecontent);
		} else {
			std::string key = filename.stem().string();
			std::string value(filecontent.begin(), filecontent.end());
			strings.emplace_back(key, value);
		}
	}

	Object elf(argv[1]);
	std::ofstream headerstream(argv[2]);
	Object_x8664 elf_x8664(argv[3]);

	for (auto string : strings) {
		headerstream << "extern const char " << string.first << "[];\n";

		variable_template comp_var(string.first, STB_GLOBAL);
		elf.push_single_variable_rodata_sections(comp_var, string.second);
		elf_x8664.push_single_variable_rodata_sections(comp_var, string.second);
	}

	return 0;
}
