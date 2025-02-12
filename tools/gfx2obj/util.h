#ifndef UTIL_H
#define UTIL_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

#define arraycount(a) (sizeof(a) / sizeof(a[0]))

static inline uint32_t file_size(const char* filename) {
	struct stat st;
	stat(filename, &st);
	return st.st_size;
}

static inline char* strdupcat(const char* first, const char* second) {
	size_t first_len = strlen(first);
	size_t second_len = strlen(second);
	char* retval = malloc(first_len + second_len + 1);
	strcpy(retval, first);
	strcpy(retval + first_len, second);
	retval[first_len + second_len] = '\0';
	return retval;
}

#endif        //  #ifndef UTIL_H
