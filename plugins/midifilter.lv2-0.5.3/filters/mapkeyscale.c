MFD_FILTER(mapkeyscale)

#ifdef MX_TTF

	mflt:mapkeyscale
	TTF_DEFAULTDEF("MIDI Note Transpose", "MIDI Note Transp")
	, TTF_IPORT( 0, "channelf", "Filter Channel", 0, 16, 0, PORTENUMZ("Any") DOC_CHANF)
	, TTF_IPORT( 1, "k0",  "C",  -13, 12, 0, lv2:portProperty lv2:integer; lv2:portProperty lv2:enumeration; SPX("Off", -13) SPX("-12 (C-1)", -12) SPX("-11 (C#-1)", -11) SPX("-10 (D-1)", -10) SPX("-9 (D#-1)", -9) SPX("-8 (E-1)", -8) SPX("-7 (F-1)", -7) SPX("-6 (F#-1)", -6) SPX("-5 (G-1)", -5) SPX("-4 (G#-1)", -4) SPX("-3 (A-1)", -3) SPX("-2 (A#-1)", -2) SPX("-1 (B-1)", -1) SPX("+-0 (C+0)", 0) SPX("+1 (C#+0)", 1) SPX("+2 (D+0)", 2) SPX("+3 (D#+0)", 3) SPX("+4 (E+0)", 4) SPX("+5 (F+0)", 5) SPX("+6 (F#+0)", 6) SPX("+7 (G+0)", 7) SPX("+8 (G#+0)", 8) SPX("+9 (A+0)", 9) SPX("+10 (A#+0)", 10) SPX("+11 (B+0)", 11) SPX("+12 (C+1)", 12) units:unit units:semitone12TET)
	, TTF_IPORT( 2, "k1",  "C#", -13, 12, 0, lv2:portProperty lv2:integer; lv2:portProperty lv2:enumeration; SPX("Off", -13) SPX("-12 (C#-1)", -12) SPX("-11 (D-1)", -11) SPX("-10 (D#-1)", -10) SPX("-9 (E-1)", -9) SPX("-8 (F-1)", -8) SPX("-7 (F#-1)", -7) SPX("-6 (G-1)", -6) SPX("-5 (G#-1)", -5) SPX("-4 (A-1)", -4) SPX("-3 (A#-1)", -3) SPX("-2 (B-1)", -2) SPX("-1 (C+0)", -1) SPX("+-0 (C#+0)", 0) SPX("+1 (D+0)", 1) SPX("+2 (D#+0)", 2) SPX("+3 (E+0)", 3) SPX("+4 (F+0)", 4) SPX("+5 (F#+0)", 5) SPX("+6 (G+0)", 6) SPX("+7 (G#+0)", 7) SPX("+8 (A+0)", 8) SPX("+9 (A#+0)", 9) SPX("+10 (B+0)", 10) SPX("+11 (C+1)", 11) SPX("+12 (C#+1)", 12) units:unit units:semitone12TET)
	, TTF_IPORT( 3, "k2",  "D",  -13, 12, 0, lv2:portProperty lv2:integer; lv2:portProperty lv2:enumeration; SPX("Off", -13) SPX("-12 (D-1)", -12) SPX("-11 (D#-1)", -11) SPX("-10 (E-1)", -10) SPX("-9 (F-1)", -9) SPX("-8 (F#-1)", -8) SPX("-7 (G-1)", -7) SPX("-6 (G#-1)", -6) SPX("-5 (A-1)", -5) SPX("-4 (A#-1)", -4) SPX("-3 (B-1)", -3) SPX("-2 (C+0)", -2) SPX("-1 (C#+0)", -1) SPX("+-0 (D+0)", 0) SPX("+1 (D#+0)", 1) SPX("+2 (E+0)", 2) SPX("+3 (F+0)", 3) SPX("+4 (F#+0)", 4) SPX("+5 (G+0)", 5) SPX("+6 (G#+0)", 6) SPX("+7 (A+0)", 7) SPX("+8 (A#+0)", 8) SPX("+9 (B+0)", 9) SPX("+10 (C+1)", 10) SPX("+11 (C#+1)", 11) SPX("+12 (D-1)", 12) units:unit units:semitone12TET)
	, TTF_IPORT( 4, "k3",  "D#", -13, 12, 0, lv2:portProperty lv2:integer; lv2:portProperty lv2:enumeration; SPX("Off", -13) SPX("-12 (D#-1)", -12) SPX("-11 (E-1)", -11) SPX("-10 (F-1)", -10) SPX("-9 (F#-1)", -9) SPX("-8 (G-1)", -8) SPX("-7 (G#-1)", -7) SPX("-6 (A-1)", -6) SPX("-5 (A#-1)", -5) SPX("-4 (B-1)", -4) SPX("-3 (C+0)", -3) SPX("-2 (C#+0)", -2) SPX("-1 (D+0)", -1) SPX("+-0 (D#+0)", 0) SPX("+1 (E+0)", 1) SPX("+2 (F+0)", 2) SPX("+3 (F#+0)", 3) SPX("+4 (G+0)", 4) SPX("+5 (G#+0)", 5) SPX("+6 (A+0)", 6) SPX("+7 (A#+0)", 7) SPX("+8 (B+0)", 8) SPX("+9 (C+1)", 9) SPX("+10 (C#+1)", 10) SPX("+11 (D+1)", 11) SPX("+12 (D#+1)", 12) units:unit units:semitone12TET)
	, TTF_IPORT( 5, "k4",  "E",  -13, 12, 0, lv2:portProperty lv2:integer; lv2:portProperty lv2:enumeration; SPX("Off", -13) SPX("-12 (E-1)", -12) SPX("-11 (F-1)", -11) SPX("-10 (F#-1)", -10) SPX("-9 (G-1)", -9) SPX("-8 (G#-1)", -8) SPX("-7 (A-1)", -7) SPX("-6 (A#-1)", -6) SPX("-5 (B-1)", -5) SPX("-4 (C+0)", -4) SPX("-3 (C#+0)", -3) SPX("-2 (D+0)", -2) SPX("-1 (D#+0)", -1) SPX("+-0 (E+0)", 0) SPX("+1 (F+0)", 1) SPX("+2 (F#+0)", 2) SPX("+3 (G+0)", 3) SPX("+4 (G#+0)", 4) SPX("+5 (A+0)", 5) SPX("+6 (A#+0)", 6) SPX("+7 (B+0)", 7) SPX("+8 (C+1)", 8) SPX("+9 (C#+1)", 9) SPX("+10 (D+1)", 10) SPX("+11 (D#+1)", 11) SPX("+12 (E+1)", 12) units:unit units:semitone12TET)
	, TTF_IPORT( 6, "k5",  "F",  -13, 12, 0, lv2:portProperty lv2:integer; lv2:portProperty lv2:enumeration; SPX("Off", -13) SPX("-12 (F-1)", -12) SPX("-11 (F#-1)", -11) SPX("-10 (G-1)", -10) SPX("-9 (G#-1)", -9) SPX("-8 (A-1)", -8) SPX("-7 (A#-1)", -7) SPX("-6 (B-1)", -6) SPX("-5 (C+0)", -5) SPX("-4 (C#+0)", -4) SPX("-3 (D+0)", -3) SPX("-2 (D#+0)", -2) SPX("-1 (E+0)", -1) SPX("+-0 (F+0)", 0) SPX("+1 (F#+0)", 1) SPX("+2 (G+0)", 2) SPX("+3 (G#+0)", 3) SPX("+4 (A+0)", 4) SPX("+5 (A#+0)", 5) SPX("+6 (B+0)", 6) SPX("+7 (C+1)", 7) SPX("+8 (C#+1)", 8) SPX("+9 (D+1)", 9) SPX("+10 (D#+1)", 10) SPX("+11 (E+1)", 11) SPX("+12 (F+1)", 12) units:unit units:semitone12TET)
	, TTF_IPORT( 7, "k6",  "F#", -13, 12, 0, lv2:portProperty lv2:integer; lv2:portProperty lv2:enumeration; SPX("Off", -13) SPX("-12 (F#-1)", -12) SPX("-11 (G-1)", -11) SPX("-10 (G#-1)", -10) SPX("-9 (A-1)", -9) SPX("-8 (A#-1)", -8) SPX("-7 (B-1)", -7) SPX("-6 (C+0)", -6) SPX("-5 (C#+0)", -5) SPX("-4 (D+0)", -4) SPX("-3 (D#+0)", -3) SPX("-2 (E+0)", -2) SPX("-1 (F+0)", -1) SPX("+-0 (F#+0)", 0) SPX("+1 (G+0)", 1) SPX("+2 (G#+0)", 2) SPX("+3 (A+0)", 3) SPX("+4 (A#+0)", 4) SPX("+5 (B+0)", 5) SPX("+6 (C+1)", 6) SPX("+7 (C#+1)", 7) SPX("+8 (D+1)", 8) SPX("+9 (D#+1)", 9) SPX("+10 (E+1)", 10) SPX("+11 (F+1)", 11) SPX("+12 (F#+1)", 12) units:unit units:semitone12TET)
	, TTF_IPORT( 8, "k7",  "G",  -13, 12, 0, lv2:portProperty lv2:integer; lv2:portProperty lv2:enumeration; SPX("Off", -13) SPX("-12 (G-1)", -12) SPX("-11 (G#-1)", -11) SPX("-10 (A-1)", -10) SPX("-9 (A#-1)", -9) SPX("-8 (B-1)", -8) SPX("-7 (C+0)", -7) SPX("-6 (C#+0)", -6) SPX("-5 (D+0)", -5) SPX("-4 (D#+0)", -4) SPX("-3 (E+0)", -3) SPX("-2 (F+0)", -2) SPX("-1 (F#+0)", -1) SPX("+-0 (G+0)", 0) SPX("+1 (G#+0)", 1) SPX("+2 (A+0)", 2) SPX("+3 (A#+0)", 3) SPX("+4 (B+0)", 4) SPX("+5 (C+1)", 5) SPX("+6 (C#+1)", 6) SPX("+7 (D+1)", 7) SPX("+8 (D#+1)", 8) SPX("+9 (E+1)", 9) SPX("+10 (F+1)", 10) SPX("+11 (F#+1)", 11) SPX("+12 (G+1)", 12) units:unit units:semitone12TET)
	, TTF_IPORT( 9, "k8",  "G#", -13, 12, 0, lv2:portProperty lv2:integer; lv2:portProperty lv2:enumeration; SPX("Off", -13) SPX("-12 (G#-1)", -12) SPX("-11 (A-1)", -11) SPX("-10 (A#-1)", -10) SPX("-9 (B-1)", -9) SPX("-8 (C+0)", -8) SPX("-7 (C#+0)", -7) SPX("-6 (D+0)", -6) SPX("-5 (D#+0)", -5) SPX("-4 (E+0)", -4) SPX("-3 (F+0)", -3) SPX("-2 (F#+0)", -2) SPX("-1 (G+0)", -1) SPX("+-0 (G#+0)", 0) SPX("+1 (A+0)", 1) SPX("+2 (A#+0)", 2) SPX("+3 (B+0)", 3) SPX("+4 (C+1)", 4) SPX("+5 (C#+1)", 5) SPX("+6 (D+1)", 6) SPX("+7 (D#+1)", 7) SPX("+8 (E+1)", 8) SPX("+9 (F+1)", 9) SPX("+10 (F#+1)", 10) SPX("+11 (G+1)", 11) SPX("+12 (G#+1)", 12) units:unit units:semitone12TET)
	, TTF_IPORT(10, "k9",  "A",  -13, 12, 0, lv2:portProperty lv2:integer; lv2:portProperty lv2:enumeration; SPX("Off", -13) SPX("-12 (A-1)", -12) SPX("-11 (A#-1)", -11) SPX("-10 (B-1)", -10) SPX("-9 (C+0)", -9) SPX("-8 (C#+0)", -8) SPX("-7 (D+0)", -7) SPX("-6 (D#+0)", -6) SPX("-5 (E+0)", -5) SPX("-4 (F+0)", -4) SPX("-3 (F#+0)", -3) SPX("-2 (G+0)", -2) SPX("-1 (G#+0)", -1) SPX("+-0 (A+0)", 0) SPX("+1 (A#+0)", 1) SPX("+2 (B+0)", 2) SPX("+3 (C+1)", 3) SPX("+4 (C#+1)", 4) SPX("+5 (D+1)", 5) SPX("+6 (D#+1)", 6) SPX("+7 (E+1)", 7) SPX("+8 (F+1)", 8) SPX("+9 (F#+1)", 9) SPX("+10 (G+1)", 10) SPX("+11 (G#+1)", 11) SPX("+12 (A+1)", 12) units:unit units:semitone12TET)
	, TTF_IPORT(11, "k10", "A#", -13, 12, 0, lv2:portProperty lv2:integer; lv2:portProperty lv2:enumeration; SPX("Off", -13) SPX("-12 (A#-1)", -12) SPX("-11 (B-1)", -11) SPX("-10 (C+0)", -10) SPX("-9 (C#+0)", -9) SPX("-8 (D+0)", -8) SPX("-7 (D#+0)", -7) SPX("-6 (E+0)", -6) SPX("-5 (F+0)", -5) SPX("-4 (F#+0)", -4) SPX("-3 (G+0)", -3) SPX("-2 (G#+0)", -2) SPX("-1 (A+0)", -1) SPX("+-0 (A#+0)", 0) SPX("+1 (B+0)", 1) SPX("+2 (C+1)", 2) SPX("+3 (C#+1)", 3) SPX("+4 (D+1)", 4) SPX("+5 (D#+1)", 5) SPX("+6 (E+1)", 6) SPX("+7 (F+1)", 7) SPX("+8 (F#+1)", 8) SPX("+9 (G+1)", 9) SPX("+10 (G#+1)", 10) SPX("+11 (A+1)", 11) SPX("+12 (A#+1)", 12) units:unit units:semitone12TET)
	, TTF_IPORT(12, "k11", "B",  -13, 12, 0, lv2:portProperty lv2:integer; lv2:portProperty lv2:enumeration; SPX("Off", -13) SPX("-12 (B-1)", -12) SPX("-11 (C+0)", -11) SPX("-10 (C#+0)", -10) SPX("-9 (D+0)", -9) SPX("-8 (D#+0)", -8) SPX("-7 (E+0)", -7) SPX("-6 (F+0)", -6) SPX("-5 (F#+0)", -5) SPX("-4 (G+0)", -4) SPX("-3 (G#+0)", -3) SPX("-2 (A+0)", -2) SPX("-1 (A#+0)", -1) SPX("+-0 (B+0)", 0) SPX("+1 (C+1)", 1) SPX("+2 (C#+1)", 2) SPX("+3 (D+1)", 3) SPX("+4 (D#+1)", 4) SPX("+5 (E+1)", 5) SPX("+6 (F+1)", 6) SPX("+7 (F#+1)", 7) SPX("+8 (G+1)", 8) SPX("+9 (G#+1)", 9) SPX("+10 (A+1)", 10) SPX("+11 (A#+1)", 11) SPX("+12 (B+1)", 12) units:unit units:semitone12TET)
	; rdfs:comment "Flexible 12-tone map. Allow to map a note within an octave to another note in the same octave-range +- 12 semitones. Alternatively notes can also be masked (disabled). If two keys are mapped to the same note, the corresponding note on/events are latched: only the first note on and last note off will be sent. The settings can be changed dynamically: Note-on/off events will be sent accordingly.";
	.

#elif defined MX_CODE

static inline void filter_mapkeyscale_panic(MidiFilter* self, const uint8_t c, const uint32_t tme) {
	int k;
	for (k=0; k < 127; ++k) {
		if (self->memCS[c][k] > 0) {
			uint8_t buf[3];
			buf[0] = MIDI_NOTEOFF | c;
			buf[1] = k;
			buf[2] = 0;
			forge_midimessage(self, tme, buf, 3);
		}
		self->memCI[c][k] = -1000; // current key transpose
		self->memCM[c][k] = 0; // remember last velocity
		self->memCS[c][k] = 0; // count note-on per key
	}
}

static void
filter_midi_mapkeyscale(MidiFilter* self,
		uint32_t tme,
		const uint8_t* const buffer,
		uint32_t size)
{
	int i;
	const int chs = midi_limit_chn(floorf(*self->cfg[0]) -1);
	int keymap[12];
	for (i=0; i < 12; ++i) {
		keymap[i] = RAIL(floorf(*self->cfg[i+1]), -13, 12);
	}

	const uint8_t chn = buffer[0] & 0x0f;
	uint8_t mst = buffer[0] & 0xf0;

	if (midi_is_panic(buffer, size)) {
		filter_mapkeyscale_panic(self, chn, tme);
	}

	if (size != 3
			|| !(mst == MIDI_NOTEON || mst == MIDI_NOTEOFF || mst == MIDI_POLYKEYPRESSURE)
			|| !(floorf(*self->cfg[0]) == 0 || chs == chn)
			)
	{
		forge_midimessage(self, tme, buffer, size);
		return;
	}

	const uint8_t key = buffer[1] & 0x7f;
	const uint8_t vel = buffer[2] & 0x7f;

	if (mst == MIDI_NOTEON && vel ==0 ) {
		mst = MIDI_NOTEOFF;
	}

	int note;
	uint8_t buf[3];
	memcpy(buf, buffer, 3);

	switch (mst) {
		case MIDI_NOTEON:
			if (keymap[key%12] < -12) return;
			note = key + keymap[key%12];
			// TODO keep track of dup result note-on -- see enforcescale.c
			if (midi_valid(note)) {
				buf[1] = note;
				self->memCS[chn][note]++;
				if (self->memCS[chn][note] == 1) {
					forge_midimessage(self, tme, buf, size);
				}
				self->memCM[chn][key] = vel;
				self->memCI[chn][key] = note - key;
			}
			break;
		case MIDI_NOTEOFF:
			note = key + self->memCI[chn][key];
			if (midi_valid(note)) {
				buf[1] = note;
				if (self->memCS[chn][note] > 0) {
					self->memCS[chn][note]--;
					if (self->memCS[chn][note] == 0)
						forge_midimessage(self, tme, buf, size);
				}
			}
			self->memCM[chn][key] = 0;
			self->memCI[chn][key] = -1000;
			break;
		case MIDI_POLYKEYPRESSURE:
			if (keymap[key%12] < -12) return;
			note = key + keymap[key%12];
			if (midi_valid(note)) {
				buf[1] = note;
				forge_midimessage(self, tme, buf, size);
			}
			break;
	}
}

static void filter_preproc_mapkeyscale(MidiFilter* self) {
	int i;
	int identical_cfg = 1;
	int keymap[12];
	for (i=0; i < 12; ++i) {
		keymap[i] = RAIL(floorf(*self->cfg[i+1]), -13, 12);
		if (floorf(self->lcfg[i+1]) != floorf(*self->cfg[i+1])) {
			identical_cfg = 0;
		}
	}
	if (identical_cfg) return;

	int c,k;
	uint8_t buf[3];
	buf[2] = 0;
	for (c=0; c < 16; ++c) {
		for (k=0; k < 127; ++k) {
			int note;
			const int n = 1 + k%12;
			if (!self->memCM[c][k]) continue;
			if (floorf(self->lcfg[n]) == floorf(*self->cfg[n])) continue;

			note = k + self->memCI[c][k];

			if (midi_valid(note)) {
				note = midi_limit_val(note);
				if (self->memCS[c][note] > 0) {
					self->memCS[c][note]--;
					if (self->memCS[c][note] == 0) {
						buf[0] = MIDI_NOTEOFF | c;
						buf[1] = note;
						buf[2] = 0;
						forge_midimessage(self, 0, buf, 3);
					}
				}
			}

			note = k + keymap[k%12];

			if (midi_valid(note)) {
				note = midi_limit_val(note);
				buf[0] = MIDI_NOTEON | c;
				buf[1] = note;
				buf[2] = self->memCM[c][k];
				self->memCI[c][k] = note - k;
				self->memCS[c][note]++;
				if (self->memCS[c][note] == 1) {
					forge_midimessage(self, 0, buf, 3);
				}
			} else {
				self->memCM[c][k] = 0;
				self->memCI[c][k] = -1000;
			}
		}
	}
}

static void filter_init_mapkeyscale(MidiFilter* self) {
	int c,k;
	for (c=0; c < 16; ++c) for (k=0; k < 127; ++k) {
		self->memCI[c][k] = -1000; // current key transpose
		self->memCM[c][k] = 0; // remember last velocity
		self->memCS[c][k] = 0; // count note-on per key
	}
	self->preproc_fn = filter_preproc_mapkeyscale;
}

#endif
