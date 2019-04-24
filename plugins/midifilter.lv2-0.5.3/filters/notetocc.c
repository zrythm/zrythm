MFD_FILTER(notetocc)

#ifdef MX_TTF

	mflt:notetocc
	TTF_DEFAULTDEF("MIDI Note to CC", "MIDI Note to CC")
	, TTF_IPORT(0, "channelf", "Filter Channel", 0, 16, 0,
			PORTENUMZ("Any")
			DOC_CHANF)
	, TTF_IPORT(1, "mode", "Operation Mode",  0, 4, 0,
			lv2:portProperty lv2:integer; lv2:portProperty lv2:enumeration;
			lv2:scalePoint [ rdfs:label "Fixed parameter, CC-value = velocity" ; rdf:value 0 ] ;
			lv2:scalePoint [ rdfs:label "Fixed parameter, CC-value = key" ; rdf:value 1 ] ;
			lv2:scalePoint [ rdfs:label "All keys, parameter = key, CC-value = velocity" ; rdf:value 2 ] ;
			lv2:scalePoint [ rdfs:label "Toggle, parameter = key, CC-value = 127 note-on or 0 note-off" ; rdf:value 3 ] ;
			rdfs:comment "")
	, TTF_IPORT(2, "param", "CC Parameter", 0, 127, 0,
			lv2:portProperty lv2:integer;
			rdfs:comment "unused in 'all keys' mode."
			)
	, TTF_IPORT(3, "key", "Active Key (midi-note)", 0, 127, 48,
			lv2:portProperty lv2:integer; units:unit units:midiNote ;
			rdfs:comment "only used in 'value = velocity' mode."
			)
	, TTF_IPORTTOGGLE(4, "nooff", "Ignore Note Off", 1)
	; rdfs:comment "Convert only MIDI note-on messages to control change messages (ignored in toggle mode)."
	.

#elif defined MX_CODE

void filter_init_notetocc(MidiFilter* self) { }

void
filter_midi_notetocc(MidiFilter* self,
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
	const int mode = RAIL(floorf(*self->cfg[1]),0, 4);

	const uint8_t param = midi_limit_val(floorf(*self->cfg[2]));
	const uint8_t kfltr = midi_limit_val(floorf(*self->cfg[3]));

	uint8_t buf[3];
	buf[0] = MIDI_CONTROLCHANGE | chn;

	// TODO, keep track of note-on
	// send note-off whenever configuration changes
	// and an active note will be masked..

	switch(mode) {
		case 0: // single-note, fixed param, value <- velocity
			if (key != kfltr) {
				forge_midimessage(self, tme, buffer, size);
				return;
			}
			buf[1] = param;
			buf[2] = vel;
			break;
		case 1: // fixed param, value <- note
			buf[1] = param;
			buf[2] = key;
			break;
		case 2: // param <- note,  value <- velocity
			buf[1] = key;
			buf[2] = vel;
			break;
		case 3: // toggle, param <- note,  value <- 0 or 127 
			buf[1] = key;
			if (mst == MIDI_NOTEOFF) {
				buf[2] = 0;
			} else {
				buf[2] = 127;
			}
			//no break
	}
	if (mst == MIDI_NOTEON || (*(self->cfg[4])) <= 0 || mode == 3) {
		forge_midimessage(self, tme, buf, 3);
	}
}

#endif
