#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define GUARD "GUARD_DMG_MUSIC_H"
#define arraycount(a) (sizeof(a) / sizeof(a[0]))

enum mode {
	MODE_INVALID = 0,
	MODE_NOTE_NUMBER,
	MODE_FREQUENCIES,
	MODE_STAFF_POSITION,
};

const char* pitch_classes[] = {
	"C", "Cis", "D", "Dis", "E", "F", "Fis", "G", "Gis", "A", "Ais", "B",
};

int main(int argc, char* argv[]) {
	unsigned staff_position = 140;

	if (argc < 2) {
		printf("TODO \n");
		return 0;
	}
	enum mode mode = MODE_INVALID;
	if (0 == strcmp(argv[1], "--note-numbers")) {
		mode = MODE_NOTE_NUMBER;
	}
	if (0 == strcmp(argv[1], "--frequencies")) {
		mode = MODE_FREQUENCIES;
	}
	if (0 == strcmp(argv[1], "--staff-position")) {
		mode = MODE_STAFF_POSITION;
	}
	if (mode == MODE_INVALID) {
		printf("Expected mode to be one of ...");
		return -1;
	}

	switch (mode) {
	case MODE_INVALID:
		break;
	case MODE_NOTE_NUMBER:
		printf("#ifndef %s\n", GUARD);
		printf("#define %s\n", GUARD);
		printf("\n");
		printf("#include <stdint.h>\n");
		printf("\n");
		printf("enum pitch {\n");
		printf("\tPITCH_REST\t= 0,\n");
		break;
	case MODE_FREQUENCIES:
		printf("#include \"dmg_music.h\"\n");
		printf("\n");
		printf("const uint16_t pitch_frequencies[] = {\n");
		break;
	case MODE_STAFF_POSITION:
		printf("#include \"dmg_music.h\"\n");
		printf("\n");
		printf("const uint8_t staff_positions[] = {\n");
		break;
	}

	for (unsigned octave = 2; octave < 7; octave++) {
		for (unsigned pitch_class_index = 0; pitch_class_index < arraycount(pitch_classes); pitch_class_index++) {
			unsigned midi_note_number = 12 + pitch_class_index + octave * arraycount(pitch_classes);

			switch (mode) {
			case MODE_INVALID:
				break;
			case MODE_NOTE_NUMBER:
				printf("\tPITCH_%s%d\t= %d,\n", pitch_classes[pitch_class_index], octave, midi_note_number);
				break;
			case MODE_FREQUENCIES:
				double hz = 440.0 * pow(2.0, (midi_note_number - 69.0) / 12.0);
				unsigned dmg_freq = (unsigned)(2048 - ((4194304) / (hz * 32)));
				printf("\t[PITCH_%s%d]\t= %d,\t// %.2f\n", pitch_classes[pitch_class_index], octave, dmg_freq, hz);
				break;
			case MODE_STAFF_POSITION:
				if (1 == strlen(pitch_classes[pitch_class_index])) {
					staff_position -= 4;
				}
				printf("\t[PITCH_%s%d]\t= %d,\n", pitch_classes[pitch_class_index], octave, staff_position);
				break;
			}
		}
	}

	switch (mode) {
	case MODE_INVALID:
		break;
	case MODE_NOTE_NUMBER:
		printf("};\n");
		printf("#define PITCH_MAX (%ld)\n", 12 + 7 * arraycount(pitch_classes));
		printf("\n");
		printf("extern const uint16_t pitch_frequencies[];\n");
		printf("extern const uint8_t staff_positions[];\n");
		printf("\n");
		printf("#endif\n");
		break;
	case MODE_FREQUENCIES:
		printf("\t[PITCH_REST]\t= 0,\n");
		printf("};\n");
		break;
	case MODE_STAFF_POSITION:
		printf("\t[PITCH_REST]\t= 56,\n");
		printf("};\n");
		break;
	}

	return 0;
}
