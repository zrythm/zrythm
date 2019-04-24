MFD_FILTER(miditranspose)

#ifdef MX_TTF

	mflt:miditranspose
	TTF_DEFAULTDEF("MIDI Chromatic Transpose", "MIDI Transpose")
	, TTF_IPORT(0, "channelf", "Filter Channel",  0, 16, 0,
			PORTENUMZ("Any")
			DOC_CHANF)
	, TTF_IPORTINT(1, "transpose", "Transpose",  -63, 64, 0)
	, TTF_IPORT(2, "inversion", "Inversion point",  0, 127, 0,
			lv2:scalePoint [ rdfs:label "Off"; rdf:value  0 ] ; \
			lv2:portProperty lv2:integer; units:unit units:midiNote;
			rdfs:comment "Chromatic Inversion. Mirror chromatic scale around this point before transposing.")
	; rdfs:comment "Chromatic transpose of midi notes and key-pressure. If an inversion point is set, the scale is mirrored around this point before transposing. Notes that end up outside the valid range 0..127 are discarded.";
	.

#elif defined MX_CODE

static void filter_preproc_miditranspose(MidiFilter* self) {
	if (rintf(self->lcfg[1]) == rintf(*self->cfg[1])
			&& rintf(self->lcfg[2]) == rintf(*self->cfg[2])
			) return;

	const int transp = rintf(*(self->cfg[1]));
	const int invers = rintf(*(self->cfg[2]));

	int c,k;
	uint8_t buf[3];
	buf[2] = 0;
	for (c=0; c < 16; ++c) {
#if 0 /* send "all notes off" */
		buf[0] = MIDI_CONTROLCHANGE | c;
		buf[1] = 123;
		forge_midimessage(self, tme, buf, 3);

		// send "all sound off"
		buf[0] = 0xb0 | i;
		buf[1] = 120;
		forge_midimessage(self, tme, buf, 3);
#else /* re-transpose playing notes */
		for (k=0; k < 127; ++k) {
			if (!self->memCM[c][k]) continue;

			buf[0] = MIDI_NOTEOFF | c;
			buf[1] = midi_limit_val(k + self->memCI[c][k]);
			buf[2] = 0;
			forge_midimessage(self, 0, buf, 3);

			int note;
			if (invers <= 0) {
				note = k;
			} else {
				note = invers - (k - invers);
			}
			note += transp;

			buf[0] = MIDI_NOTEON | c;
			buf[1] = midi_limit_val(note);
			buf[2] = self->memCM[c][k];
			self->memCI[c][k] = note - k;
			forge_midimessage(self, 0, buf, 3);
		}
#endif
	}
}

static void filter_init_miditranspose(MidiFilter* self) {
	int c,k;

	for (c=0; c < 16; ++c) for (k=0; k < 127; ++k) {
		self->memCI[c][k] = -1000; // current key transpose
		self->memCM[c][k] = 0; // velocity
	}
	self->preproc_fn = filter_preproc_miditranspose;
}

static void
filter_midi_miditranspose(MidiFilter* self,
		uint32_t tme,
		const uint8_t* const buffer,
		uint32_t size)
{
	const int chs = midi_limit_chn(floorf(*self->cfg[0]) -1);
	const int transp = rintf(*(self->cfg[1]));
	const int invers = rintf(*(self->cfg[2]));

	const uint8_t chn = buffer[0] & 0x0f;
	const uint8_t key = buffer[1] & 0x7f;
	const uint8_t vel = buffer[2] & 0x7f;
	uint8_t mst = buffer[0] & 0xf0;


	if (size != 3
			|| !(mst == MIDI_NOTEON || mst == MIDI_NOTEOFF || mst == MIDI_POLYKEYPRESSURE)
			|| !(floorf(*self->cfg[0]) == 0 || chs == chn)
		 )
	{
		forge_midimessage(self, tme, buffer, size);
		return;
	}

	if (mst == MIDI_NOTEON && vel ==0 ) {
		mst = MIDI_NOTEOFF;
	}

	int note;
	uint8_t buf[3];

	buf[0] = buffer[0];
	buf[1] = buffer[1];
	buf[2] = buffer[2];

	switch (mst) {
		case MIDI_NOTEON:
			if (invers <= 0) {
				note = key;
			} else {
				note = invers - (key - invers);
			}
			note += transp;
			if (midi_valid(note)) {
				buf[1] = note;
				forge_midimessage(self, tme, buf, size);
			}
			self->memCM[chn][key] = vel;
			self->memCI[chn][key] = note - key;
			break;
		case MIDI_NOTEOFF:
			note = key + self->memCI[chn][key];
			if (midi_valid(note)) {
				buf[1] = note;
				forge_midimessage(self, tme, buf, size);
			}
			self->memCM[chn][key] = 0;
			self->memCI[chn][key] = -1000;
			break;
		case MIDI_POLYKEYPRESSURE:
			if (invers < 0) {
				note = key;
			} else {
				note = invers - (key - invers);
			}
			note += transp;
			if (midi_valid(note)) {
				buf[1] = note;
				forge_midimessage(self, tme, buf, size);
			}
			break;
	}
}

#endif
