MFD_FILTER(passthru)

#ifdef MX_TTF

	mflt:passthru
	TTF_DEFAULTDEF("MIDI Thru", "MIDI Thru")
	; rdfs:comment "MIDI All pass. This plugin has no effect and is intended as example."
	.

#elif defined MX_CODE

void filter_init_passthru(MidiFilter* self) { }

void
filter_midi_passthru(MidiFilter* self,
		uint32_t tme,
		const uint8_t* const buffer,
		uint32_t size)
{
	forge_midimessage(self, tme, buffer, size);
}

#endif
