MFD_FILTER(cctonote)

#ifdef MX_TTF

	mflt:cctonote
	TTF_DEFAULTDEF("MIDI CC to Note", "MIDI CC to Note")
	, TTF_IPORT(0, "channelf", "Filter Channel",  0, 16,  0,
			PORTENUMZ("Any")
			DOC_CHANF)
	, TTF_IPORT(1, "mode", "Operation Mode",  0, 3, 1,
			lv2:portProperty lv2:integer; lv2:portProperty lv2:enumeration;
			lv2:scalePoint [ rdfs:label "Fixed key, velocity = CC-value" ; rdf:value 0 ] ;
			lv2:scalePoint [ rdfs:label "key = CC-value, fixed velocity (64)" ; rdf:value 1 ] ;
			lv2:scalePoint [ rdfs:label "All keys, key = parameter, velocity = CC-value" ; rdf:value 2 ] ;
			rdfs:comment "")
	, TTF_IPORT(2, "param", "CC Parameter to intercept",  0, 127,  0,
			lv2:portProperty lv2:integer;
			rdfs:comment "unused in 'all keys' mode."
			)
	, TTF_IPORT(3, "key", "Key (midi-note) to use with fixed-key mode",  0, 127,  48,
			lv2:portProperty lv2:integer; units:unit units:midiNote ;
			rdfs:comment "only used in 'velocity = value' mode."
			)
	; rdfs:comment "Convert MIDI control change messages to note-on/off messages. Note off is queued 10msec later."
	.

#elif defined MX_CODE


/* check queued messages for existing note-off for given key/channel
 * if found, send it immediately
 */
static int cctonote_check_queue(MidiFilter* self,
		uint8_t chn,
		uint8_t key,
		uint32_t tme
		) {
	int i;
	const int qsize = self->memI[0];
	const int roff = self->memI[1];
	const int woff = self->memI[2];

	for (i=0; i < qsize; ++i) {
		const int off = (i + roff) % qsize;

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
		// Found matching queued note-off, send it now
		forge_midimessage(self, tme, self->memQ[off].buf, 3);
		// ...and remove it from the queue
		self->memQ[off].size = 0;
	}
	return 0;
}

static void
filter_cleanup_cctonote(MidiFilter* self)
{
	free(self->memQ);
}

static void
filter_postproc_cctonote(MidiFilter* self)
{
	int i;
	const int qsize = self->memI[0];
	const int roff = self->memI[1];
	const int woff = self->memI[2];
	uint32_t n_samples = self->n_samples;
	int skipped = 0;

	/* only process until given time */
	if (self->memI[3] > 0)
		n_samples = MIN(self->memI[3], n_samples);

	/* process the queue */
	for (i=0; i < qsize; ++i) {
		const int off = (i + roff) % qsize;
		if (self->memQ[off].size > 0) {
			if (self->memQ[off].reltime < n_samples) {
				forge_midimessage(self, self->memQ[off].reltime, self->memQ[off].buf, self->memQ[off].size);
				self->memQ[off].size = 0;
				if (!skipped) self->memI[1] = (self->memI[1] + 1) % qsize;
			} else {
				/* only decrement relative-time at end of cycle */
				if (self->memI[3] < 0) {self->memQ[off].reltime -= n_samples;}
				skipped = 1;
			}
		} else if (!skipped) self->memI[1] = off;

		if (off == woff) break;
	}
}

static void
filter_preproc_cctonote(MidiFilter* self)
{
	// check if operation mode changed
	if (self->lcfg[1] == *self->cfg[1]) {
		return;
	}
	/* reset cached values */
	int c,k;
	for (c=0; c < 16; ++c) for (k=0; k < 127; ++k) {
		self->memCI[c][k] = 0;
	}
}

void filter_init_cctonote(MidiFilter* self) {
	/* setup queue for note-off events */
	self->memI[0] = self->samplerate / 16.0;
	self->memI[1] = 0; // read-pointer
	self->memI[2] = 0; // write-pointer
	self->memI[3] = -1; // max time-offset
	self->memI[4] = self->samplerate * .01; // note-off delay in samples
	self->memQ = calloc(self->memI[0], sizeof(MidiEventQueue));

	int c,k;
	for (c=0; c < 16; ++c) for (k=0; k < 127; ++k) {
		self->memCI[c][k] = 0;
	}

	/* register hooks */
	self->postproc_fn = filter_postproc_cctonote;
	self->preproc_fn = filter_preproc_cctonote;
	self->cleanup_fn = filter_cleanup_cctonote;
}

void
filter_midi_cctonote(MidiFilter* self,
		uint32_t tme,
		const uint8_t* const buffer,
		uint32_t size)
{
	const uint8_t chs = midi_limit_chn(floorf(*self->cfg[0]) -1);
	const uint8_t chn = buffer[0] & 0x0f;
	const uint8_t mst = buffer[0] & 0xf0;

	const bool filter_dups = false;
	// TODO intercept CC-121 -> 'reset all controllers'
	// -> clear previous state for dup-filter

	if (size != 3
			|| mst != MIDI_CONTROLCHANGE
			|| !(floorf(*self->cfg[0]) == 0 || chs == chn)
		 )
	{
		/* bypass all message that we're not supposed to filter */
		forge_midimessage(self, tme, buffer, size);
		return;
	}

	const uint8_t param = buffer[1] & 0x7f;
	const uint8_t value = buffer[2] & 0x7f;
	const int mode = RAIL(floorf(*self->cfg[1]),0, 3);

	const uint8_t kprm = midi_limit_val(floorf(*self->cfg[2]));

	uint8_t buf[3];

	switch(mode) {
		case 0: // Fixed key, velocity = CC-value
			if (param != kprm) {
				forge_midimessage(self, tme, buffer, size);
				return;
			}
			buf[1] = midi_limit_val(floorf(*self->cfg[3]));
			buf[2] = value;
			break;
		case 1: // key = CC-value, fixed velocity (64)
			if (param != kprm) {
				forge_midimessage(self, tme, buffer, size);
				return;
			}
			buf[1] = value;
			buf[2] = 64;
			break;
		case 2: // All CCs, key = parameter, velocity = value
			buf[1] = param;
			buf[2] = value;
			break;
	}

	/* process queued messages until now */
	self->memI[3] = tme + 1;
	filter_postproc_cctonote(self);
	self->memI[3] = -1;

	/* check if key/value changed */
	if (!filter_dups || self->memCI[chn][param] != value) {
		/* remember key/value */
		self->memCI[chn][param] = value;

		/* check if Note-off is queued -> send immediately */
		// TODO: think: flush _all_ for this channel if mode==1 !?
		cctonote_check_queue(self, chn, buf[1], tme);

		/* send note on */
		buf[0] = MIDI_NOTEON | chn;
		forge_midimessage(self, tme, buf, 3);

		/* queue note-off */
		buf[0] = MIDI_NOTEOFF | chn;
		buf[2] = 0;
		MidiEventQueue *qm = &(self->memQ[self->memI[2]]);
		memcpy(qm->buf, buf, size);
		qm->size = size;
		qm->reltime = tme + self->memI[4];
		self->memI[2] = (self->memI[2] + 1) % self->memI[0];
	}
}

#endif
