#ifndef PARSE_ARGUMENTS_H
#define PARSE_ARGUMENTS_H

#include <stddef.h>

struct Flag {
	char* key;
	void (*setter)(void* accumulator);
};

struct Argument {
	char* key;
	void (*setter)(void* accumulator, const char* arg);
};

struct ArgumentsAndFlags {
	/** null-terminated */
	struct Flag* flags;
	/** null-terminated */
	struct Argument* arguments;
};

void parseArguments(
	int argc, char** argv,
	void* accumulator,
	const struct ArgumentsAndFlags*
);

#endif        //  #ifndef PARSE_ARGUMENTS_H
