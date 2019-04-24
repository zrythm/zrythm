MFD_FILTER(mapkeychannel)

#ifdef MX_TTF

	mflt:mapkeychannel
	TTF_DEFAULTDEF("MIDI Note/Channel Map", "MIDI Key/Chn Map")
	, TTF_IPORT( 0, "k0",  "C",  0, 16, 1, PORTENUMZ("Off") DOC_CHANZ)
	, TTF_IPORT( 1, "k1",  "C#", 0, 16, 1, PORTENUMZ("Off") DOC_CHANZ)
	, TTF_IPORT( 2, "k2",  "D",  0, 16, 1, PORTENUMZ("Off") DOC_CHANZ)
	, TTF_IPORT( 3, "k3",  "D#", 0, 16, 1, PORTENUMZ("Off") DOC_CHANZ)
	, TTF_IPORT( 4, "k4",  "E",  0, 16, 1, PORTENUMZ("Off") DOC_CHANZ)
	, TTF_IPORT( 5, "k5",  "F",  0, 16, 1, PORTENUMZ("Off") DOC_CHANZ)
	, TTF_IPORT( 6, "k6",  "F#", 0, 16, 1, PORTENUMZ("Off") DOC_CHANZ)
	, TTF_IPORT( 7, "k7",  "G",  0, 16, 1, PORTENUMZ("Off") DOC_CHANZ)
	, TTF_IPORT( 8, "k8",  "G#", 0, 16, 1, PORTENUMZ("Off") DOC_CHANZ)
	, TTF_IPORT( 9, "k9",  "A",  0, 16, 1, PORTENUMZ("Off") DOC_CHANZ)
	, TTF_IPORT(10, "k10", "A#", 0, 16, 1, PORTENUMZ("Off") DOC_CHANZ)
	, TTF_IPORT(11, "k11", "B",  0, 16, 1, PORTENUMZ("Off") DOC_CHANZ)
	; rdfs:comment "12-tone channel map. Allow to change midi-channel per note. (Events other than note-on/off will be passed through as-is; currently there is no channel panic forwarding, nor note-off events when changing the channel-assignments dynamically)";
	.

#elif defined MX_CODE

static void
filter_midi_mapkeychannel(MidiFilter* self,
		uint32_t tme,
		const uint8_t* const buffer,
		uint32_t size)
{
	int i;
	int keymap[12];
	uint8_t buf[3];
	for (i = 0; i < 12; ++i) {
		keymap[i] = RAIL(floorf(*self->cfg[i]), 0, 16);
	}

	uint8_t mst = buffer[0] & 0xf0;
	if (size != 3 || !(mst == MIDI_NOTEON || mst == MIDI_NOTEOFF))
	{
		forge_midimessage(self, tme, buffer, size);
		return;
	}

	memcpy(buf, buffer, size);
	const uint8_t key = (buffer[1] & 0x7f) % 12;
	if (keymap [key] == 0) {
		return;
	}
	buf[0] = mst | (keymap [key] - 1);
	forge_midimessage(self, tme, buf, size);
}

static void filter_init_mapkeychannel(MidiFilter* self) { }

#endif
