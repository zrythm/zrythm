MFD_FILTER(scalecc)

#ifdef MX_TTF

	mflt:scalecc
	TTF_DEFAULTDEF("MIDI Scale CC Value", "MIDI CC Scale")
	, TTF_IPORT(0, "channelf", "Filter Channel", 0, 16, 0,
			PORTENUMZ("Any")
			DOC_CHANF)
	, TTF_IPORT(1, "lower", "Parameter (Min)", 0, 127, 0,
			lv2:portProperty lv2:integer;
			rdfs:comment "lower end of parameter-range (inclusive)")
	, TTF_IPORT(2, "upper", "Parameter (Max)", 0, 127, 127,
			lv2:portProperty lv2:integer;
			rdfs:comment "upper end of parameter-range (inclusive)")
	, TTF_IPORT(3, "pmode", "Parameter Mode", 0, 2, 1,
			lv2:portProperty lv2:integer; lv2:portProperty lv2:enumeration;
			lv2:scalePoint [ rdfs:label "Bypass" ; rdf:value 0 ] ;
			lv2:scalePoint [ rdfs:label "Include Range" ; rdf:value 1 ] ;
			lv2:scalePoint [ rdfs:label "Exclude Range" ; rdf:value 2 ] ;
			rdfs:comment "")
	, TTF_IPORT(4, "valmul",  "Value Scale",   -10.0, 10.0, 1.0,
			rdfs:comment "")
	, TTF_IPORT(5, "valoff",  "Value Offset",  -64, 64, 0,
			lv2:portProperty lv2:integer;
			rdfs:comment "")
	, TTF_IPORT(6, "vmode", "Value Mode", 0, 2, 0,
			lv2:portProperty lv2:integer; lv2:portProperty lv2:enumeration;
			lv2:scalePoint [ rdfs:label "Clamp to 0..127" ; rdf:value 0 ] ;
			lv2:scalePoint [ rdfs:label "Reflect Overflow (-1 to 1, 128 to 127)"  ; rdf:value 1 ] ;
			lv2:scalePoint [ rdfs:label "Truncate Overflow (-1 to 127, 128 to 0)" ; rdf:value 2 ] ;
			rdfs:comment "")
	; rdfs:comment "Modify the value (data-byte) of a MIDI control change message."
	.

#elif defined MX_CODE

void filter_init_scalecc(MidiFilter* self) { }

void
filter_midi_scalecc(MidiFilter* self,
		uint32_t tme,
		const uint8_t* const buffer,
		uint32_t size)
{
	const int mode = RAIL(floorf(*self->cfg[3]),0, 2);
	const uint8_t chs = midi_limit_chn(floorf(*self->cfg[0]) -1);
	const uint8_t chn = buffer[0] & 0x0f;
	uint8_t mst = buffer[0] & 0xf0;

	if (size != 3
			|| !(mst == MIDI_CONTROLCHANGE)
			|| !(floorf(*self->cfg[0]) == 0 || chs == chn)
			|| mode == 0
		 )
	{
		forge_midimessage(self, tme, buffer, size);
		return;
	}

	const uint8_t low = midi_limit_val(floorf(*self->cfg[1]));
	const uint8_t upp = midi_limit_val(floorf(*self->cfg[2]));
	const uint8_t param = buffer[1] & 0x7f;
	const uint8_t value = buffer[2] & 0x7f;

	if ((param >= low && param <= upp) ^ (mode == 2)) {
		const float mul = (*self->cfg[4]);
		const float off = (*self->cfg[5]);
		const int clamp = RAIL(floorf(*self->cfg[6]),0, 2);
		uint8_t buf[3];

		int val = rintf((value * mul) + off);
		switch(clamp) {
			case 1:
				if (val < 0) {
					buf[2] = -(val%128);
				} else {
					buf[2] = val%128;
				}
				break;
			case 2:
				buf[2] = val&0x7f;
				break;
			default:
				buf[2] = RAIL(val, 0, 127);
				break;
		}
		buf[0] = buffer[0];
		buf[1] = param;
		forge_midimessage(self, tme, buf, 3);
	} else {
		forge_midimessage(self, tme, buffer, size);
	}
}

#endif
