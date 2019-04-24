MFD_FILTER(onechannelfilter)

#ifdef MX_TTF

	mflt:onechannelfilter
	TTF_DEFAULTDEF("MIDI Simple Channel Filter", "MIDI 1 Chn Flt.")
	, TTF_IPORT(0, "channel", "Channel",  1, 16,  1, PORTENUM16
			rdfs:comment "MIDI channel that can pass.")
	; rdfs:comment "MIDI channel filter. Only data for selected channel shall pass. This filter only affects midi-data which is channel relevant (ie note-on/off, control and program changes, key and channel pressure and pitchbend). MIDI-SYSEX and Realtime message are always passed on. See also 'MIDI Channel Map' and 'MIDI Channel Filter' filters." ; 
	.

#elif defined MX_CODE

void filter_init_onechannelfilter(MidiFilter* self) { }

void
filter_midi_onechannelfilter(MidiFilter* self,
		uint32_t tme,
		const uint8_t* const buffer,
		uint32_t size)
{
	if (size > 3) {
		forge_midimessage(self, tme, buffer, size);
		return;
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
			if (rintf(*(self->cfg[0])) == chn + 1) {
				forge_midimessage(self, tme, buffer, size);
			}
			break;
		default:
			forge_midimessage(self, tme, buffer, size);
			break;
	}
}

#endif
