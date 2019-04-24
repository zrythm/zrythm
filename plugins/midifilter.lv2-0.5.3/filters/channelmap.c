MFD_FILTER(channelmap)

#ifdef MX_TTF

	mflt:channelmap
	TTF_DEFAULTDEF("MIDI Channel Map", "MIDI Chn Map")
	, TTF_IPORT( 0, "chn1",  "Channel  1 to", 0, 16,  1, PORTENUMZ("Off") DOC_CHANZ)
	, TTF_IPORT( 1, "chn2",  "Channel  2 to", 0, 16,  2, PORTENUMZ("Off") DOC_CHANZ)
	, TTF_IPORT( 2, "chn3",  "Channel  3 to", 0, 16,  3, PORTENUMZ("Off") DOC_CHANZ)
	, TTF_IPORT( 3, "chn4",  "Channel  4 to", 0, 16,  4, PORTENUMZ("Off") DOC_CHANZ)
	, TTF_IPORT( 4, "chn5",  "Channel  5 to", 0, 16,  5, PORTENUMZ("Off") DOC_CHANZ)
	, TTF_IPORT( 5, "chn6",  "Channel  6 to", 0, 16,  6, PORTENUMZ("Off") DOC_CHANZ)
	, TTF_IPORT( 6, "chn7",  "Channel  7 to", 0, 16,  7, PORTENUMZ("Off") DOC_CHANZ)
	, TTF_IPORT( 7, "chn8",  "Channel  8 to", 0, 16,  8, PORTENUMZ("Off") DOC_CHANZ)
	, TTF_IPORT( 8, "chn9",  "Channel  9 to", 0, 16,  9, PORTENUMZ("Off") DOC_CHANZ)
	, TTF_IPORT( 9, "chn10", "Channel 10 to", 0, 16, 10, PORTENUMZ("Off") DOC_CHANZ)
	, TTF_IPORT(10, "chn11", "Channel 11 to", 0, 16, 11, PORTENUMZ("Off") DOC_CHANZ)
	, TTF_IPORT(11, "chn12", "Channel 12 to", 0, 16, 12, PORTENUMZ("Off") DOC_CHANZ)
	, TTF_IPORT(12, "chn13", "Channel 13 to", 0, 16, 13, PORTENUMZ("Off") DOC_CHANZ)
	, TTF_IPORT(13, "chn14", "Channel 14 to", 0, 16, 14, PORTENUMZ("Off") DOC_CHANZ)
	, TTF_IPORT(14, "chn15", "Channel 15 to", 0, 16, 15, PORTENUMZ("Off") DOC_CHANZ)
	, TTF_IPORT(15, "chn16", "Channel 16 to", 0, 16, 16, PORTENUMZ("Off") DOC_CHANZ)
	; rdfs:comment "Rewrite midi-channel number. This filter only affects midi-data which is channel relevant (ie note-on/off, control and program changes, key and channel pressure and pitchbend). MIDI-SYSEX and Realtime message are always passed thru unmodified." ; 
	.

#elif defined MX_CODE

static void filter_init_channelmap(MidiFilter* self) { }

static void
filter_midi_channelmap(MidiFilter* self,
		uint32_t tme,
		const uint8_t* const buffer,
		uint32_t size)
{
	uint8_t buf[3];
	if (size > 3) {
		forge_midimessage(self, tme, buffer, size);
		return;
	} else {
		memcpy(buf, buffer, size);
	}

	int chn = buffer[0]&0x0f;
	switch (buffer[0] & 0xf0) {
		case MIDI_NOTEOFF:
		case MIDI_NOTEON:
		case MIDI_POLYKEYPRESSURE:
		case MIDI_CONTROLCHANGE:
		case MIDI_PROGRAMCHANGE:
		case MIDI_CHANNELPRESSURE:
		case MIDI_PITCHBEND:
			if(*self->cfg[chn] == 0) return;
			chn = midi_limit_chn(floorf(-1 + *(self->cfg[chn])));
			buf[0] = (buffer[0] & 0xf0) | chn;
			break;
		default:
			break;
	}
	forge_midimessage(self, tme, buf, size);
}

#endif
