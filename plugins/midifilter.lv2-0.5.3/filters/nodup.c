MFD_FILTER(nodup)

#ifdef MX_TTF

	mflt:nodup
	TTF_DEFAULTDEF("MIDI Duplicate Blocker", "MIDI Dup. Block")
	, TTF_IPORT(0, "channelf", "Filter Channel", 0, 16, 0,
			PORTENUMZ("Any")
			DOC_CHANF)
	; rdfs:comment "MIDI Duplicate Blocker. Filter out overlapping note on/off and duplicate messages."
	.

#elif defined MX_CODE

#define DUP_PURGE_OLD(WHAT) \
	if (MSC_DIFF(self->memI[0], self->memCI[c][WHAT]) > (MSC_MAX>>2)) { \
		self->memCI[c][WHAT] = -1; \
		self->memCI[c][WHAT|1] = -1; \
	}

static void
filter_postproc_nodup(MidiFilter* self)
{
	int c;
	self->memI[0] = (self->memI[0] + self->n_samples)%MSC_MAX;

	for (c=0; c < 16; ++c) {
		DUP_PURGE_OLD(MIDI_PITCHBEND)
		DUP_PURGE_OLD(MIDI_CHANNELPRESSURE)
		DUP_PURGE_OLD(MIDI_PROGRAMCHANGE)
		DUP_PURGE_OLD(MIDI_CONTROLCHANGE)
		DUP_PURGE_OLD(MIDI_POLYKEYPRESSURE)
	}
}

void filter_init_nodup(MidiFilter* self) {
	int c,k;
	for (c=0; c < 16; ++c) for (k=0; k < 127; ++k) {
		self->memCS[c][k] = 0; // count note-on per key
		self->memCI[c][k] = -1;
	}
	self->memI[0] = 0; // monotonic time
	self->postproc_fn = filter_postproc_nodup;
}

static inline void filter_nodup_panic(MidiFilter* self, const uint8_t c, const uint32_t tme) {
	int k;
	for (k=0; k < 127; ++k) {
		if (self->memCS[c][k]) {
			uint8_t buf[3];
			buf[0] = MIDI_NOTEOFF | c;
			buf[1] = k; buf[2] = 0;
			forge_midimessage(self, tme, buf, 3);
		}
		self->memCS[c][k] = 0;
		self->memCI[c][k] = -1;
	}
}

void
filter_midi_nodup(MidiFilter* self,
		uint32_t tme,
		const uint8_t* const buffer,
		uint32_t size)
{
	const uint8_t chs = midi_limit_chn(floorf(*self->cfg[0]) -1);
	const uint8_t chn = buffer[0] & 0x0f;
	uint8_t mst = buffer[0] & 0xf0;

	if (size != 3
			|| mst == MIDI_SYSEX
			|| !(floorf(*self->cfg[0]) == 0 || chs == chn)
			)
	{
		forge_midimessage(self, tme, buffer, size);
		return;
	}

	if (midi_is_panic(buffer, size)) {
		filter_nodup_panic(self, chn, tme);
	}

	const uint8_t key = buffer[1] & 0x7f;
	const int dupDeltaT = RAIL(0 /*TODO *self->cfg[1] */, 0, (MSC_MAX>>2));

	switch (mst) {
		case MIDI_NOTEON:
			self->memCS[chn][key]++;
			if (self->memCS[chn][key] > 1) {
				return;
			}
			break;
		case MIDI_NOTEOFF:
			if (self->memCS[chn][key] > 0) {
				self->memCS[chn][key]--;
			}
			if (self->memCS[chn][key] > 0) {
				return;
			}
			break;
		case MIDI_PITCHBEND:
		case MIDI_CHANNELPRESSURE:
		case MIDI_PROGRAMCHANGE:
		case MIDI_CONTROLCHANGE:
		case MIDI_POLYKEYPRESSURE:
			if (midi_14bit(buffer) == self->memCI[chn][(mst|1)]
					&& MSC_DIFF((self->memI[0] + tme)%MSC_MAX, self->memCI[chn][mst]) <= dupDeltaT) {
				return;
			}
			self->memCI[chn][mst] = (tme + self->memI[0])%MSC_MAX;
			self->memCI[chn][(mst|1)] = midi_14bit(buffer);
			break;
	}

	forge_midimessage(self, tme, buffer, size);
}

#endif
