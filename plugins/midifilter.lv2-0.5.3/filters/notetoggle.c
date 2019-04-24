MFD_FILTER(notetoggle)

#ifdef MX_TTF

	mflt:notetoggle
	TTF_DEFAULTDEF("MIDI Note Toggle", "MIDI Note Toggle")
	, TTF_IPORT(0, "channelf", "Filter Channel", 0, 16, 0,
			PORTENUMZ("Any")
			DOC_CHANF)
	; rdfs:comment "Toggle Notes: play a note to turn it on, play it again to turn it off."
	.

#elif defined MX_CODE

static void
filter_midi_notetoggle(MidiFilter* self,
		uint32_t tme,
		const uint8_t* const buffer,
		uint32_t size)
{
	const uint8_t chs = midi_limit_chn(floorf(*self->cfg[0]) -1);
	const uint8_t chn = buffer[0] & 0x0f;
	const uint8_t mst = buffer[0] & 0xf0;

	if (size != 3
			|| !(mst == MIDI_NOTEON || mst == MIDI_NOTEOFF)
			|| !(floorf(*self->cfg[0]) == 0 || chs == chn)
		 )
	{
		forge_midimessage(self, tme, buffer, size);
		return;
	}

	const uint8_t key = buffer[1] & 0x7f;
	const uint8_t vel = buffer[2] & 0x7f;

	if (mst == MIDI_NOTEOFF || (mst == MIDI_NOTEON && vel ==0)) {
		return;
	}

	if (!self->memCI[chn][key]) {
		forge_midimessage(self, tme, buffer, size);
		self->memCI[chn][key] = 1;
	} else {
		uint8_t buf[3];
		buf[0] = MIDI_NOTEOFF | chn;
		buf[1] = key;
		buf[2] = 0;
		forge_midimessage(self, tme, buf, size);
		self->memCI[chn][key] = 0;
	}
}

static void
filter_init_notetoggle(MidiFilter* self)
{
	int c,k;
	for (c=0; c < 16; ++c) for (k=0; k < 127; ++k) {
		self->memCI[c][k] = 0;
	}
}
#endif
