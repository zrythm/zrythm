MFD_FILTER(mididup)

#ifdef MX_TTF

	mflt:mididup
	TTF_DEFAULTDEF("MIDI Channel Unisono", "MIDI Chn Unisono")
	, TTF_IPORTINT(0, "chs", "Source Channel",  1, 16,  1)
	, TTF_IPORTINT(1, "chd", "Duplicate to Channel", 1, 16, 2)
	; rdfs:comment "Duplicate MIDI events from one channel to another."
	.

#elif defined MX_CODE

static void filter_init_mididup(MidiFilter* self) { }

static void
filter_midi_mididup(MidiFilter* self,
		uint32_t tme,
		const uint8_t* const buffer,
		uint32_t size)
{
	const int chs = midi_limit_chn(floorf(*self->cfg[0]) -1);
	const int chd = midi_limit_chn(floorf(*self->cfg[1]) -1);

	const int chn = buffer[0] & 0x0f;
	const int msg = buffer[0] & 0xf0;

	forge_midimessage(self, tme, buffer, size);

	if (chs == chd
			|| msg == 0xf0 
			|| msg < 0x80 
			|| chn != chs
			|| size > 3
			) {
		return;
	}

	uint8_t buf[3];
	memcpy(buf, buffer, size);
	buf[0] = msg | chd;
	forge_midimessage(self, tme, buf, size);
}

#endif
