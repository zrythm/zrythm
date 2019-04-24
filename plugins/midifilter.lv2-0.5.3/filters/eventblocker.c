MFD_FILTER(eventblocker)

#ifdef MX_TTF

	mflt:eventblocker
	TTF_DEFAULTDEF("MIDI Event Filter", "MIDI Event Flt.")
	, TTF_IPORTTOGGLE( 0, "blkcc",   "Block Control Changes", 0)
	, TTF_IPORTTOGGLE( 1, "blknote", "Block Notes", 0)
	, TTF_IPORTTOGGLE( 2, "blkpc",   "Block Program Changes", 0)
	, TTF_IPORTTOGGLE( 3, "blkpp",   "Block Polykey-Pressure", 0)
	, TTF_IPORTTOGGLE( 4, "blkcp",   "Block Channel-Pressure", 0)
	, TTF_IPORTTOGGLE( 5, "blkpb",   "Block Pitch Bend", 0)
	, TTF_IPORTTOGGLE( 6, "blksx",   "Block Sysex/RT messages", 0)
	, TTF_IPORTTOGGLE( 7, "blkcm",   "Block custom message", 0)
	, TTF_IPORT(8, "cmt", "Custom Message Type",  0, 6, 0,
			lv2:portProperty lv2:integer; lv2:portProperty lv2:enumeration;
			lv2:scalePoint [ rdfs:label "Note Off (0x8x)"  ; rdf:value 0 ] ;
			lv2:scalePoint [ rdfs:label "Note On (0x9x)"  ; rdf:value 1 ] ;
			lv2:scalePoint [ rdfs:label "Polykey Pressure (0xAx)"  ; rdf:value 2 ] ;
			lv2:scalePoint [ rdfs:label "Control Change (0xBx)"  ; rdf:value 3 ] ;
			lv2:scalePoint [ rdfs:label "Program Change (0xCx)"  ; rdf:value 4 ] ;
			lv2:scalePoint [ rdfs:label "Channel Pressure (0xDx)"  ; rdf:value 5 ] ;
			lv2:scalePoint [ rdfs:label "Pitchbend (0xEx)"  ; rdf:value 6 ] ;
			)
	, TTF_IPORT(9, "cmf", "Custom message Channel", 0, 16, 0,
			PORTENUMZ("Any")
			DOC_CHANF)
	, TTF_IPORT(10, "cm1", "Custom message Data1",  -1, 127, -1, lv2:portProperty lv2:integer;
			lv2:scalePoint [ rdfs:label "Any" ; rdf:value -1 ] ;
			)
	, TTF_IPORT(11, "cm2", "Custom message Data2",  -1, 127, -1, lv2:portProperty lv2:integer;
			lv2:scalePoint [ rdfs:label "Any" ; rdf:value -1 ] ;
			)
	; rdfs:comment "Notch style message filter. Suppress specific messages. For flexible note-on/off range see also 'keyrange' and 'velocityrange'."
	.

#elif defined MX_CODE

void filter_init_eventblocker(MidiFilter* self) { }

void
filter_midi_eventblocker(MidiFilter* self,
		uint32_t tme,
		const uint8_t* const buffer,
		uint32_t size)
{
	const uint8_t mst = buffer[0] & 0xf0;

	if (mst == MIDI_NOTEOFF          && (*self->cfg[1]) > 0) return;
	if (mst == MIDI_NOTEON           && (*self->cfg[1]) > 0) return;
	if (mst == MIDI_PROGRAMCHANGE    && (*self->cfg[2]) > 0) return;
	if (mst == MIDI_CONTROLCHANGE    && (*self->cfg[0]) > 0) return;
	if (mst == MIDI_POLYKEYPRESSURE  && (*self->cfg[3]) > 0) return;
	if (mst == MIDI_CHANNELPRESSURE  && (*self->cfg[4]) > 0) return;
	if (mst == MIDI_PITCHBEND        && (*self->cfg[5]) > 0) return;
	if (mst == MIDI_SYSEX            && (*self->cfg[6]) > 0) return;

	if (size != 3 || !(*self->cfg[7])) {
		/* long sysex messages */
		forge_midimessage(self, tme, buffer, size);
		return;
	}

	/* custom filter */
	const uint8_t chn = buffer[0] & 0x0f;

	const uint8_t chs = midi_limit_chn(floorf(*self->cfg[9]) -1);

	int block = 1;

	if (floorf(*self->cfg[10]) != -1) {
		const uint8_t df1 = midi_limit_val(floorf(*self->cfg[10]));
		const uint8_t dd1 = buffer[1] & 0x7f;
		if (dd1 != df1) block = 0;
	}

	if (floorf(*self->cfg[11]) != -1) {
		const uint8_t df2 = midi_limit_val(floorf(*self->cfg[11]));
		const uint8_t dd2 = buffer[2] & 0x7f;
		if (dd2 != df2) block = 0;
	}

	/* pass trhu messages which don't match the channel or data */
	if (!block || !(floorf(*self->cfg[9]) == 0 || chs == chn)) {
		forge_midimessage(self, tme, buffer, size);
		return;
	}

	switch ((int)floorf(*self->cfg[8])) {
		case 0: if (mst == MIDI_NOTEOFF) return; break;
		case 1: if (mst == MIDI_NOTEON) return; break;
		case 2: if (mst == MIDI_POLYKEYPRESSURE) return; break;
		case 3: if (mst == MIDI_CONTROLCHANGE) return; break;
		case 4: if (mst == MIDI_PROGRAMCHANGE) return; break;
		case 5: if (mst == MIDI_CHANNELPRESSURE) return; break;
		case 6: if (mst == MIDI_PITCHBEND) return; break;
		default: break;
	}

	forge_midimessage(self, tme, buffer, size);
}

#endif
