MFD_FILTER(randvelocity)

#ifdef MX_TTF

	mflt:randvelocity
	TTF_DEFAULTDEF("MIDI Velocity Randomization", "MIDI Rand Vel.")
	, TTF_IPORT(0, "channel", "Filter Channel",  0, 16, 0,
			PORTENUMZ("Any")
			DOC_CHANF)
	, TTF_IPORTFLOAT(1, "randfact", "Velocity Randomization", 0.0, 127.0,  8.0)
	, TTF_IPORT( 2, "mode",  "Random Mode", 0, 1, 1,
			lv2:scalePoint [ rdfs:label "Absolute (equal distribution)" ; rdf:value 0 ] ;
			lv2:scalePoint [ rdfs:label "Normalized (standard deviation)" ; rdf:value 1 ] ;
			lv2:portProperty lv2:integer; lv2:portProperty lv2:enumeration;
			rdfs:comment ""
			)
	; rdfs:comment "Randomize Velocity of MIDI notes (both note on and note off)."
	.

#elif defined MX_CODE

static void filter_init_randvelocity(MidiFilter* self) {
	srandom ((unsigned int) time (NULL));
}

static void
filter_midi_randvelocity(MidiFilter* self,
		uint32_t tme,
		const uint8_t* const buffer,
		uint32_t size)
{
	const int chs = midi_limit_chn(floorf(*self->cfg[0]) -1);
	const int chn = buffer[0] & 0x0f;
	int mst = buffer[0] & 0xf0;

	if (size != 3
			|| !(mst == MIDI_NOTEON || (mst == MIDI_NOTEOFF))
			|| !(floorf(*self->cfg[0]) == 0 || chs == chn)
			)
	{
		forge_midimessage(self, tme, buffer, size);
		return;
	}

	const uint8_t vel = buffer[2] & 0x7f;

	if (mst == MIDI_NOTEON && vel ==0 ) {
		mst = MIDI_NOTEOFF;
	}

	uint8_t buf[3];
	buf[0] = buffer[0];
	buf[1] = buffer[1];

	const float rf = *(self->cfg[1]);
	float rnd;

	if (*(self->cfg[2])) {
		rnd = -rf + 2.0 * rf * random() / (float)RAND_MAX;
	} else {
		rnd = normrand(rf);
	}

	switch (mst) {
		case MIDI_NOTEON:
			buf[2] = RAIL(rintf(buffer[2] + rnd), 1, 127);
			break;
		case MIDI_NOTEOFF:
			buf[2] = RAIL(rintf(buffer[2] + rnd), 0, 127);
			break;
	}
	forge_midimessage(self, tme, buf, size);
}

#endif
