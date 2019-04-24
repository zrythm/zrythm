MFD_FILTER(mapcc)

#ifdef MX_TTF

	mflt:mapcc
	TTF_DEFAULTDEF("MIDI CC Map", "MIDI CC Map")
	, TTF_IPORT(0, "channelf", "Filter Channel", 0, 16, 0,
			PORTENUMZ("Any")
			DOC_CHANF)
	, TTF_IPORT(1, "ccin", "CC Input", 0, 127, 0,
			lv2:portProperty lv2:integer;
			rdfs:comment "The control to change"
			)
	, TTF_IPORT(2, "ccout", "CC Output", 0, 127, 0,
			lv2:portProperty lv2:integer;
			rdfs:comment "The target controller"
			)
	; rdfs:comment "Change one control message into another -- combine with scalecc to modify/scale the actual value."
	.

#elif defined MX_CODE

void filter_init_mapcc(MidiFilter* self) { }

void
filter_midi_mapcc(MidiFilter* self,
		uint32_t tme,
		const uint8_t* const buffer,
		uint32_t size)
{
	const uint8_t chs = midi_limit_chn(floorf(*self->cfg[0]) -1);
	const uint8_t chn = buffer[0] & 0x0f;
	const uint8_t mst = buffer[0] & 0xf0;

	const uint8_t ccin  = midi_limit_val(floorf(*self->cfg[1]));
	const uint8_t ccout = midi_limit_val(floorf(*self->cfg[2]));

	if (size != 3
			|| !(mst == MIDI_CONTROLCHANGE)
			|| !(floorf(*self->cfg[0]) == 0 || chs == chn)
			|| !(ccin == (buffer[1] & 0x7f))
			||  (ccin == ccout)
		 )
	{
		forge_midimessage(self, tme, buffer, size);
		return;
	}

	uint8_t buf[3];
	buf[0] = buffer[0];
	buf[1] = ccout;
	buf[2] = buffer[2];

	forge_midimessage(self, tme, buf, 3);
}

#endif
