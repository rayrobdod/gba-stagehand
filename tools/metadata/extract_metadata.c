#include "extract_metadata.h"

#include <png.h>
#include <stdbool.h>
#include <string.h>

static const unsigned magic_size = 8;

struct metadata_result extract_metadata_from_png(const char* name) {
	struct metadata_result retval = {0};
	retval.value.file_name = strdup(name);

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

	for (int i = 0; i < num_text; i++) {
		if (0 == strcmp("Title", texts[i].key)) {
			retval.value.title = strdup(texts[i].text);
		}
		else if (0 == strcmp("Author", texts[i].key)) {
			retval.value.author = strdup(texts[i].text);
		}
		else if (0 == strcmp("Author URL", texts[i].key)) {
			retval.value.author_url = strdup(texts[i].text);
		}
		else if (0 == strcmp("Retrieved From", texts[i].key)) {
			retval.value.retrieved_from = strdup(texts[i].text);
		}
		else if (0 == strcmp("Copyright", texts[i].key)) {
			retval.value.licensed_under = strdup(texts[i].text);
		}
		else if (0 == strcmp("Type", texts[i].key)) {
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
