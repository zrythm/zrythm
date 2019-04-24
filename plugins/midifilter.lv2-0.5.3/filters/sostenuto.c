MFD_FILTER(sostenuto)

#ifdef MX_TTF

	mflt:sostenuto
	TTF_DEFAULTDEF("MIDI Sostenuto", "MIDI Sostenuto")
	, TTF_IPORT(0, "channelf", "Filter Channel", 0, 16, 0,
			PORTENUMZ("Any")
			DOC_CHANF)
	, TTF_IPORT( 1, "sostenuto",  "Sostenuto [sec]", 0.0, 600.0,  0.0, units:unit units:s ;
			rdfs:comment "Time to delay the note-off signal.")
	, TTF_IPORT( 2, "pedal",  "Pedal Mode", 0, 2, 1,
			lv2:scalePoint [ rdfs:label "off" ; rdf:value 0 ] ;
			lv2:scalePoint [ rdfs:label "on" ; rdf:value 1 ] ;
			lv2:scalePoint [ rdfs:label "CC64" ; rdf:value 2 ] ;
			lv2:scalePoint [ rdfs:label "CC66" ; rdf:value 3 ] ;
			lv2:portProperty lv2:integer;  lv2:portProperty lv2:enumeration;
			rdfs:comment "Mode of the sustain pedal. Fixed on (pedal pressed) or off (pedal released) for notes of all MIDI channels. The on/off state can alternatively be set by CC, in which case it's per MIDI-channel.")
	; rdfs:comment "This filter delays note-off messages by a given time, emulating a piano sostenuto pedal. When the pedal is released, note-off messages that are queued will be sent immediately. The delay-time can be changed dynamically, changes do affects note-off messages that are still queued."
	.

#elif defined MX_CODE

static void
filter_cleanup_sostenuto(MidiFilter* self)
{
	free(self->memQ);
}

static int sostenuto_check_dup(MidiFilter* self,
		uint8_t chn,
		uint8_t key,
		int newdelay
		) {
	int i;
	const int max_delay = self->memI[0];
	const int roff = self->memI[1];
	const int woff = self->memI[2];

	for (i=0; i < max_delay; ++i) {
		const int off = (i + roff) % max_delay;

		if (self->memQ[off].size != 3) {
			if (off == woff) break;
			continue;
		}

		const uint8_t c = self->memQ[off].buf[0] & 0x0f;
		const uint8_t k = self->memQ[off].buf[1] & 0x7f;
		if (c != chn || k != key) {
			if (off == woff) break;
			continue;
		}

		if (newdelay >= 0)
			self->memQ[off].reltime = newdelay;
		else
			self->memQ[off].size = 0;
		return 1;
	}
	return 0;
}

static void
filter_postproc_sostenuto(MidiFilter* self)
{
	int i;
	const int max_delay = self->memI[0];
	const int roff = self->memI[1];
	const int woff = self->memI[2];
	uint32_t n_samples = self->n_samples;
	int skipped = 0;

	/* only process until given time */
	if (self->memI[3] > 0)
		n_samples = MIN(self->memI[3], n_samples);

	for (i=0; i < max_delay; ++i) {
		const int off = (i + roff) % max_delay;
		if (self->memQ[off].size > 0) {
			if (self->memQ[off].reltime < n_samples) {
				forge_midimessage(self, self->memQ[off].reltime, self->memQ[off].buf, self->memQ[off].size);
				self->memQ[off].size = 0;
				if (!skipped) self->memI[1] = (self->memI[1] + 1) % max_delay;
			} else {
				/* don't decrement time if called from filter_midi_sostenuto() */
				if (self->memI[3] < 0) {self->memQ[off].reltime -= n_samples;}
				skipped = 1;
			}
		} else if (!skipped) self->memI[1] = off;

		if (off == woff) break;
	}
}

void
filter_midi_sostenuto(MidiFilter* self,
		const uint32_t tme,
		const uint8_t* const buffer,
		const uint32_t size)
{
	const uint8_t chs = midi_limit_chn(floorf(*self->cfg[0]) -1);
	const uint32_t delay = floor(self->samplerate * RAIL((*self->cfg[1]), 0, 120));

	const int pedal = RAIL(*self->cfg[2], 0, 2);
	int state;

	const uint8_t chn = buffer[0] & 0x0f;
	const uint8_t vel = buffer[2] & 0x7f;
	uint8_t mst = buffer[0] & 0xf0;

	if (size == 3 && mst == MIDI_CONTROLCHANGE && (buffer[1]) == 64 && pedal == 2) {
		self->memI[16 + chn] = vel > 63 ? 1 : 0;
	}
	if (size == 3 && mst == MIDI_CONTROLCHANGE && (buffer[1]) == 66 && pedal == 3) {
		self->memI[16 + chn] = vel > 63 ? 1 : 0;
	}

	if (size != 3
			|| !(mst == MIDI_NOTEON || mst == MIDI_NOTEOFF)
			|| !(floorf(*self->cfg[0]) == 0 || chs == chn)
			)
	{
		forge_midimessage(self, tme, buffer, size);
		return;
	}

	switch(pedal) {
		default:
		case 0: state = 0; break;
		case 1: state = 1; break;
		case 2: // no break, fallthrough
		case 3: state = self->memI[16 + chn]; break;
	}

	if (mst == MIDI_NOTEON && vel ==0 ) {
		mst = MIDI_NOTEOFF;
	}

	if (size == 3 && mst == MIDI_NOTEON && state == 1) {
		const uint8_t chn = buffer[0] & 0x0f;
		const uint8_t key = buffer[1] & 0x7f;
		if (sostenuto_check_dup(self, chn, key, -1)) {
			/* note was already on,
			 * send note off + immediate note on, again
			 */
			uint8_t buf[3];
			buf[0] = MIDI_NOTEOFF | chn;
			buf[1] = key;
			buf[2] = 0;
			forge_midimessage(self, tme, buf, size);
		}
		forge_midimessage(self, tme, buffer, size);
	}
	else if (size == 3 && mst == MIDI_NOTEOFF && state == 1) {
		const uint8_t chn = buffer[0] & 0x0f;
		const uint8_t key = buffer[1] & 0x7f;

		if (!sostenuto_check_dup(self, chn, key, tme + delay)) {
			// queue note-off if not already queued
			MidiEventQueue *qm = &(self->memQ[self->memI[2]]);
			memcpy(qm->buf, buffer, size);
			qm->size = size;
			qm->reltime = tme + delay;
			self->memI[2] = (self->memI[2] + 1) % self->memI[0];
		}
	}
	else {
		forge_midimessage(self, tme, buffer, size);
	}

	/* only note-off are queued in the buffer.
	 * Interleave delay-buffer with messages sent in-process above
	 * to retain sequential order.
	 */
	self->memI[3] = tme + 1;
	filter_postproc_sostenuto(self);
	self->memI[3] = -1;
}

static void
filter_preproc_sostenuto(MidiFilter* self)
{
	int i;
	const int max_delay = self->memI[0];
	const int roff = self->memI[1];
	const int woff = self->memI[2];
	const int pedal = RAIL(*self->cfg[2], 0, 2);
	int state;

	if (self->lcfg[1] == *self->cfg[1]
			&& (self->lcfg[2] == *self->cfg[2] && self->lcfg[2] < 2)) {
		/* no change: same pedal state for all channels and same time */
		for (i=32; i < 48; ++i) {
			/* remember per channel pedal state */
			self->memI[i] = pedal & 1;
		}
		return;
	}

	/* adjust time if changed || per-channel pedal state */

	const float diff = *self->cfg[1] - self->lcfg[1];
	const int delay = rint(self->samplerate * diff);

	for (i=0; i < max_delay; ++i) {
		const int off = (i + roff) % max_delay;

		if (pedal == 2) {
			const uint8_t chn = self->memQ[off].buf[0] & 0x0f;
			const int ostate = self->memI[32 + chn];
			state = self->memI[16 + chn];

			if (self->lcfg[1] == *self->cfg[1] && state == ostate) {
				/* no change: same pedal state for this channels and same time */
				if (off == woff) break;
				continue;
			}
		} else {
			state = pedal & 1;
		}

		if (self->memQ[off].size > 0) {
			if (state == 0) {
				self->memQ[off].reltime = 0;
			} else {
				self->memQ[off].reltime = MAX(0, self->memQ[off].reltime + delay);
			}
		}
		if (off == woff) break;
	}

	self->memI[3] = 1;
	filter_postproc_sostenuto(self);
	self->memI[3] = -1;

	/* remember per channel pedal state */
	for (i=16; i < 32; ++i) {
		if (pedal < 2) {
			self->memI[16+i] = pedal & 1;
		} else {
			self->memI[16+i] = self->memI[i];
		}
	}
}

static void filter_init_sostenuto(MidiFilter* self) {
	srandom ((unsigned int) time (NULL));
	self->memI[0] = self->samplerate / 16.0;
	self->memI[1] = 0; // read-pointer
	self->memI[2] = 0; // write-pointer
	self->memI[3] = -1; // max time-offset
	self->memI[4] = 0;  // prev state
	for (uint32_t i = 16; i < 16; ++i) {
		self->memI[i] = 0; // per channel pedal (CC64)
		self->memI[16 + i] = 1; // previous pedal state (default = on)
	}
	self->memQ = calloc(self->memI[0], sizeof(MidiEventQueue));
	self->postproc_fn = filter_postproc_sostenuto;
	self->preproc_fn = filter_preproc_sostenuto;
	self->cleanup_fn = filter_cleanup_sostenuto;
}

#endif
