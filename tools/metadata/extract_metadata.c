#include "extract_metadata.h"

#include <png.h>
#include <stdbool.h>
#include <string.h>

static const unsigned magic_size = 8;

struct metadata_result extract_metadata_from_png(const char* name) {
	struct metadata_result retval = {0};

	FILE *fp = fopen(name, "rb");
	if (!fp) {
		retval.error_message = "Could not open file";
		return retval;
	}

	png_byte magic[magic_size];
	if (magic_size != fread(magic, 1, magic_size, fp)) {
		retval.error_message = "Could not read";
		return retval;
	};

	bool is_png = !png_sig_cmp(magic, 0, magic_size);
	if (!is_png) {
		retval.error_message = "Not PNG";
		return retval;
	}

	png_structp png_ptr = png_create_read_struct(
		PNG_LIBPNG_VER_STRING,
		(png_voidp)NULL,
		NULL,
		NULL
	);
	if (!png_ptr) {
		retval.error_message = "Could not allocate";
		return retval;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_read_struct(
			&png_ptr,
			(png_infopp)NULL,
			(png_infopp)NULL
		);
		retval.error_message = "Could not allocate";
		return retval;
	}

	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(fp);
		retval.error_message = "Could not set setjmp";
		return retval;
	}


	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, magic_size);
	png_set_user_limits(png_ptr, 0x4000, 0x4000);

	png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

	png_text *texts;
	unsigned num_text = png_get_text(png_ptr, info_ptr, &texts, NULL);

	for (unsigned i = 0; i < num_text; i++) {
		if (0 == strcmp("Title", texts[i].key)) {
			retval.value.tasl.title = strdup(texts[i].text);
		}
		else if (0 == strcmp("Author", texts[i].key)) {
			retval.value.tasl.author = strdup(texts[i].text);
		}
		else if (0 == strcmp("Author URL", texts[i].key)) {
			retval.value.tasl.author_url = strdup(texts[i].text);
		}
		else if (0 == strcmp("Retrieved From", texts[i].key)) {
			retval.value.tasl.retrieved_from = strdup(texts[i].text);
		}
		else if (0 == strcmp("Copyright", texts[i].key)) {
			retval.value.tasl.licensed_under = strdup(texts[i].text);
		}
		else if (0 == strcmp("Derived From Title", texts[i].key)) {
			retval.value.derived_from_tasl.title = strdup(texts[i].text);
		}
		else if (0 == strcmp("Derived From Author", texts[i].key)) {
			retval.value.derived_from_tasl.author = strdup(texts[i].text);
		}
		else if (0 == strcmp("Derived From Author URL", texts[i].key)) {
			retval.value.derived_from_tasl.author_url = strdup(texts[i].text);
		}
		else if (0 == strcmp("Derived From Retrieved From", texts[i].key)) {
			retval.value.derived_from_tasl.retrieved_from = strdup(texts[i].text);
		}
		else if (0 == strcmp("Derived From Copyright", texts[i].key)) {
			retval.value.derived_from_tasl.licensed_under = strdup(texts[i].text);
		}
		else if (0 == strcmp("Type", texts[i].key)) {
			// don't care
		}
		else if (0 == strcmp("Palette Tag", texts[i].key)) {
			// don't care
		}
		else if (0 == strcmp("Tileset Tag", texts[i].key)) {
			// don't care
		}
		else if (0 == strcmp("Software", texts[i].key)) {
			// don't care
		}
		else {
			fprintf(stderr, "%s: %s\n", texts[i].key, texts[i].text);
		}
	}

	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	fclose(fp);
	return retval;
}

static inline void free_tasl_fields(struct tasl tasl) {
	free(tasl.title);
	free(tasl.author);
	free(tasl.author_url);
	free(tasl.retrieved_from);
	free(tasl.licensed_under);
}

void free_metadata_fields(struct metadata metadata) {
	free_tasl_fields(metadata.tasl);
	free_tasl_fields(metadata.derived_from_tasl);
}

static bool equal_nullable_str(const char* a, const char* b) {
	if (a == NULL && b == NULL) {
		return true;
	} else if (a == NULL || b == NULL) {
		return false;
	} else {
		return 0 == strcmp(a, b);
	}
}

static inline bool equal_tasl_fields(const struct tasl* a, const struct tasl* b) {
	return
		equal_nullable_str(a->title, b->title) &&
		equal_nullable_str(a->author, b->author) &&
		equal_nullable_str(a->author_url, b->author_url) &&
		equal_nullable_str(a->retrieved_from, b->retrieved_from) &&
		equal_nullable_str(a->licensed_under, b->licensed_under) &&
		true;
}

bool equal_metadata(const struct metadata* a, const struct metadata* b) {
	return
		equal_tasl_fields(&a->tasl, &b->tasl) &&
		equal_tasl_fields(&a->derived_from_tasl, &b->derived_from_tasl) &&
		true;
}
