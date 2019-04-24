MFD_FILTER(keysplit)

#ifdef MX_TTF

	mflt:keysplit
	TTF_DEFAULTDEF("MIDI Keysplit", "MIDI Keysplit")
	, TTF_IPORT(0, "channelf", "Filter Channel",  0, 16, 0, PORTENUMZ("Any") DOC_CHANF)
	, TTF_IPORT(1, "split", "Splitpoint",  0, 127,  48,
			lv2:portProperty lv2:integer; units:unit units:midiNote ;
			rdfs:comment "Given note and all higher notes are sent to 'upper-manual'."
			)
	, TTF_IPORT(2, "channel0", "Channel Lower",  1, 16,  1, PORTENUM16
			rdfs:comment "MIDI channel of 'lower-manual'.")
	, TTF_IPORT(3, "transp0", "Transpose Lower",  -48, 48,  0, lv2:portProperty lv2:integer; units:unit units:midiNote)
	, TTF_IPORT(4, "channel1", "Channel Upper",  1, 16, 2, PORTENUM16
			rdfs:comment "MIDI channel of 'upper-manual'.")
	, TTF_IPORT(5, "transp1", "Transpose Upper",  -48, 48, 0, lv2:portProperty lv2:integer; units:unit units:midiNote)
	; rdfs:comment "Change midi-channel number depending on note. The plugin keeps track of transposed midi-notes in case and sends note-off events accordingly if the range is changed even if a note is active. However the split-point and channel-assignments for each manual should only be changed when no notes are currently played. "
	.

#elif defined MX_CODE

static void
filter_init_keysplit(MidiFilter* self)
{
	int i;
	for (i=0; i < 127; ++i) {
		self->memI[i] = -1000;
	}
}

static void
filter_midi_keysplit(MidiFilter* self,
		uint32_t tme,
		const uint8_t* const buffer,
		uint32_t size)
{
	const int chs = midi_limit_chn(floor(*self->cfg[0]) -1);
	const uint8_t chn = buffer[0] & 0x0f;
	uint8_t mst = buffer[0] & 0xf0;

	if (size != 3
			|| !(mst == MIDI_NOTEON || mst == MIDI_NOTEOFF || mst == MIDI_POLYKEYPRESSURE || mst == MIDI_CONTROLCHANGE)
			|| !(floor(*self->cfg[0]) == 0 || chs == chn)
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

	const uint8_t split = midi_limit_val(floorf(*self->cfg[1]));
	const int ch0 = midi_limit_chn(floorf(*self->cfg[2]) -1);
	const int ch1 = midi_limit_chn(floorf(*self->cfg[4]) -1);
	const int transp0 = rintf(*self->cfg[3]);
	const int transp1 = rintf(*self->cfg[5]);

	uint8_t buf[3];
	buf[2] = buffer[2];

	switch (mst) {
		case MIDI_NOTEON:
			if (key < split) {
				buf[0] = mst | ch0;
				buf[1] = midi_limit_val(key + transp0);
				self->memI[key] = transp0;
			} else {
				buf[0] = mst | ch1;
				buf[1] = midi_limit_val(key + transp1);
				self->memI[key] = transp1;
			}
			break;
		case MIDI_NOTEOFF:
			if (key < split) {
				buf[0] = mst | ch0;
				buf[1] = midi_limit_val(key + self->memI[key]);
				self->memI[key] = -1000;
			} else {
				buf[0] = mst | ch1;
				buf[1] = midi_limit_val(key + self->memI[key]);
				self->memI[key] = -1000;
			}
			break;
		case MIDI_POLYKEYPRESSURE:
			if (key < split) {
				buf[0] = mst | ch0;
				buf[1] = midi_limit_val(key + transp0);
			} else {
				buf[0] = mst | ch1;
				buf[1] = midi_limit_val(key + transp1);
			}
			break;
		case MIDI_CONTROLCHANGE:
			buf[1] = buffer[1];
			if (ch0 != ch1) {
				buf[0] = mst | ch0;
				forge_midimessage(self, tme, buf, size);
			}
			buf[0] = mst | ch1;
			break;
	}
	forge_midimessage(self, tme, buf, size);
}

#endif
