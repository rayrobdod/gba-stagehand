#ifndef OBJECT_H
#define OBJECT_H

#include <stddef.h>
#include <elf.h>

struct variable_template {
	const char* name;
	uint8_t binding;
};

struct relocation_template {
	Elf32_Addr offset;
	unsigned char type;
	const char* symbol_name;
};

struct Object;

#ifdef __cplusplus
extern "C" {
#endif

struct Object* object_start(const char* out_file_name);

void object_finish(struct Object*);

void object_push_file_copy_section(
	struct Object*,
	const char* src_filename,
	struct variable_template);

void object_push_bytes_section(
	struct Object*,
	const void* data, Elf32_Word size,
	struct variable_template);

void push_relocation_section(
	struct Object* self,
	char* target_name,
	struct relocation_template* rels, size_t rel_count);

#ifdef __cplusplus
}
#endif

#endif        //  #ifndef OBJECT_H
