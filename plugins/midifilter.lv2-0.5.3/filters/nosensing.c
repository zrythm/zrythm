MFD_FILTER(noactivesensing)

#ifdef MX_TTF

	mflt:noactivesensing
	TTF_DEFAULTDEF("MIDI Remove Active Sensing", "MIDI No Act Sens")
	; rdfs:comment "Filter to block all active sensing events. Active sensing messages are optional MIDI messages and intended to be sent repeatedly to tell a receiver that a connection is alive, however they can clutter up the MIDI channel or be inadvertently recorded when dumping raw MIDI data to disk."
	.

#elif defined MX_CODE

void filter_init_noactivesensing(MidiFilter* self) { }

void
filter_midi_noactivesensing(MidiFilter* self,
		uint32_t tme,
		const uint8_t* const buffer,
		uint32_t size)
{
	const int mst = buffer[0] & 0xf0;
	if (mst == MIDI_ACTIVESENSING) return;
	forge_midimessage(self, tme, buffer, size);
}

#endif
