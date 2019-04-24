MFD_FILTER(monolegato)

#ifdef MX_TTF

	mflt:monolegato
	TTF_DEFAULTDEF("MIDI Monophonic Legato", "MIDI Mono Legato")
	, TTF_IPORT(0, "channelf", "Filter Channel", 0, 16, 0,
			PORTENUMZ("Any")
			DOC_CHANF)
	; rdfs:comment "Hold a note until the next note arrives. -- Play the same note again to switch it off."
	.

#elif defined MX_CODE

static void
filter_midi_monolegato(MidiFilter* self,
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

	const int samenote = self->memCI[chn][0] == key;

	if (midi_valid(self->memCI[chn][0])) {
		uint8_t buf[3];
		buf[0] = MIDI_NOTEOFF | chn;
		buf[1] = self->memCI[chn][0];
		buf[2] = 0;
		forge_midimessage(self, tme, buf, size);
		self->memCI[chn][0] = -1000;
	}
	if (!samenote) {
		forge_midimessage(self, tme, buffer, size);
		self->memCI[chn][0] = key;
	}
}

static void
filter_init_monolegato(MidiFilter* self)
{
	int c;
	for (c=0; c < 16; ++c) {
		self->memCI[c][0] = -1000;
	}
}
#endif
