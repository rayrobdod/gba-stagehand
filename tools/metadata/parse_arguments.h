#ifndef PARSE_ARGUMENTS_H
#define PARSE_ARGUMENTS_H

struct Arguments {
	unsigned inPngFileCount;
	const char** inPngFiles;
	const char* outCreditsObject;
	const char* outCreditsHeader;
	const char* errorMsg;
};

struct Arguments parseArguments(int argc, char** argv);

#endif        //  #ifndef PARSE_ARGUMENTS_H
