#ifndef OBJECT_H
#define OBJECT_H

#include <elf.h>

struct Object;

#ifdef __cplusplus
extern "C" {
#endif

struct Object* object_start(const char* out_file_name);

void object_finish(struct Object*);

void object_push_file_copy_section(
	struct Object*,
	const char* src_filename,
	const char* variable_name);

#ifdef __cplusplus
}
#endif

#endif        //  #ifndef OBJECT_H
