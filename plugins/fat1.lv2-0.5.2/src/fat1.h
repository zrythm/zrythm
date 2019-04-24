#define FAT1_URI "http://gareus.org/oss/lv2/fat1"

typedef enum {
	FAT_MIDI_IN = 0,
	FAT_INPUT, FAT_OUTPUT,
	FAT_MODE,
	FAT_MCHN,
	FAT_TUNE,
	FAT_BIAS,
	FAT_FILT,
	FAT_CORR,
	FAT_OFFS, // 9

	FAT_NOTE, // + 12 notes
	FAT_PANIC = 22,
	FAT_MASK,
	FAT_NSET,
	FAT_ERRR,
	FAT_LTNC,
	FAT_SCALE, // +12 notes (microtonal only)
	FAT_LAST = 39
} PortIndex;
