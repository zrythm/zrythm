MFD_FILTER(mididelay)

#ifdef MX_TTF

	mflt:mididelay
	TTF_DEF("MIDI Delayline", "MIDI Delayline", ; atom:supports time:Position)
	, TTF_IPORT( 0, "bpmsrc",  "BPM source", 0, 1, 1,
			lv2:scalePoint [ rdfs:label "Control Port" ; rdf:value 0 ] ;
			lv2:scalePoint [ rdfs:label "Plugin Host (if available)" ; rdf:value 1 ] ;
			lv2:portProperty lv2:integer; lv2:portProperty lv2:enumeration;
			)

	, TTF_IPORT(1, "delayBPM",  "BPM", 1.0, 280.0,  120.0, units:unit units:bpm;
			rdfs:comment "base unit for the delay-time")
	, TTF_IPORT(2, "delayBeats", "Delay Beats 4/4", 0.0, 16.0,  1.0,
			lv2:scalePoint [ rdfs:label "No Delay" ; rdf:value 0.0 ] ;
			lv2:scalePoint [ rdfs:label "Eighth" ; rdf:value 0.5 ] ;
			lv2:scalePoint [ rdfs:label "Quarter" ; rdf:value 1.0 ] ;
			lv2:scalePoint [ rdfs:label "Half Note" ; rdf:value 2.0 ] ;
			lv2:scalePoint [ rdfs:label "Whole Note" ; rdf:value 4.0 ] ;
			lv2:scalePoint [ rdfs:label "Two Bars" ; rdf:value 8.0 ] ;
			lv2:scalePoint [ rdfs:label "Four Bars" ; rdf:value 16.0 ] ;
			rdfs:comment "delay length in base-unit")
	, TTF_IPORT( 3, "delayRandom", "Randomize [Beats]", 0.0, 1.0,  0.0,
			rdfs:comment "randomization factor")
	; rdfs:comment "MIDI delay line. Delay all MIDI events by a given time which is given as BPM and beats. If the delay includes a random factor, this effect takes care of always keeping note on/off events sequential regardless of the randomization."
	.

#elif defined MX_CODE

static void
filter_cleanup_mididelay(MidiFilter* self)
{
	free(self->memQ);
}

static int mididelay_noteon_mindelay(MidiFilter* self,
		uint8_t chn,
		uint8_t key) {
	int i;
	const int max_delay = self->memI[0];
	const int roff = self->memI[1];
	const int woff = self->memI[2];

	int min_delay = 0;
	for (i=0; i < max_delay; ++i) {
		const int off = (i + roff) % max_delay;
		if (self->memQ[off].size != 3) continue;
		const uint8_t s = self->memQ[off].buf[0] & 0xf0;
		const uint8_t c = self->memQ[off].buf[0] & 0x0f;
		const uint8_t k = self->memQ[off].buf[1] & 0x7f;
		if (c != chn || k != key) continue;
		if (s != MIDI_NOTEON && s != MIDI_NOTEOFF) continue;
		min_delay = MAX(min_delay, self->memQ[off].reltime);
		if (off == woff) break;
	}
	return min_delay;
}

void
filter_midi_mididelay(MidiFilter* self,
		const uint32_t tme,
		const uint8_t* const buffer,
		const uint32_t size)
{
	float bpm = (*self->cfg[1]);
	if (*self->cfg[0] && (self->available_info & NFO_BPM)) {
		bpm = self->bpm;
	}
	if (bpm <= 0) bpm = 60;

	int delay = floor(self->samplerate * (*self->cfg[2]) * 60.0 / bpm);
	float rnd_val = self->samplerate * (*self->cfg[3]) * 60.0 / bpm;
	float rnd_off = 0;

	if (delay < 0) delay = 0;

#ifdef SYMMETRIC_RND
	delay += rnd_val;
#endif


	if (rnd_val > 0 && delay > 0) {
		rnd_off -=  MIN(rnd_val, delay);
		rnd_val +=  MIN(rnd_val, delay);
	}
	if (rnd_val > 0) {
		delay += rintf(rnd_off + rnd_val * (float)random() / (float)RAND_MAX);
	}
	
	if ((self->memI[2] + 1) % self->memI[0] == self->memI[1]) {
		return;
	}

#if 1 /* keep track of note on/off */
	uint8_t mst = buffer[0] & 0xf0;
	const uint8_t vel = buffer[2] & 0x7f;

	if (mst == MIDI_NOTEON && vel ==0 ) {
		mst = MIDI_NOTEOFF;
	}

	if (size == 3 && mst == MIDI_NOTEON) {
		// TODO handle consecuitve Note On -> no overlap
		// keep track of absolute time of latest note-on/off
		const uint8_t chn = buffer[0] & 0x0f;
		const uint8_t key = buffer[1] & 0x7f;
		delay = MAX(delay, mididelay_noteon_mindelay(self, chn, key)); // XXX SPEED
		self->memCI[chn][key] = delay;
	}

	else if (size == 3 && mst == MIDI_NOTEOFF) {
		const uint8_t chn = buffer[0] & 0x0f;
		const uint8_t key = buffer[1] & 0x7f;
		if(self->memCI[chn][key] >= 0) {
			// TODO negative randomization, just not to before note-start
			delay = MAX(delay, self->memCI[chn][key]);
		}
		self->memCI[chn][key] = -1;
	}
#endif

	if (size > 3) {
		//forge_midimessage(self, tme, buffer, size);  // XXX may arrive out of order
		return;
	}

	MidiEventQueue *qm = &(self->memQ[self->memI[2]]);
	memcpy(qm->buf, buffer, size);
	qm->size = size;
	qm->reltime = tme + delay; /// *  1.0/SPEED
	self->memI[2] = (self->memI[2] + 1) % self->memI[0];
}

static void
filter_preproc_mididelay(MidiFilter* self)
{
#ifdef SYMMETRIC_RND
	// TODO unify -- calc once per cycle -- not for every midi-note
	float bpm = (*self->cfg[1]);
	if (*self->cfg[0] && (self->available_info & NFO_BPM)) {
		bpm = self->bpm;
	}
	if (bpm <= 0) bpm = 60;

	// use random -fact as latency..
	self->latency = self->samplerate * (*self->cfg[3]) * 60.0 / bpm;
#endif
}

static void
filter_postproc_mididelay(MidiFilter* self)
{
	int i;
	const int max_delay = self->memI[0];
	const int roff = self->memI[1];
	const int woff = self->memI[2];
	const uint32_t n_samples = self->n_samples;  /// * SPEED
	int skipped = 0;

#if 0
	if ((self->available_info & (NFO_BEAT|NFO_SPEED)) == (NFO_BEAT|NFO_SPEED)) {
		if (self->memI[3] != self->elapsed_len && self->speed !=0) {
			printf("pos changed %d -> %d\n", self->memI[3], self->elapsed_len);
			self->memI[3] = self->elapsed_len;
		}
	}
#endif

	for (i=0; i < max_delay; ++i) {
		const int off = (i + roff) % max_delay;
		if (self->memQ[off].size > 0) {
			if (self->memQ[off].reltime < n_samples) {
				forge_midimessage(self, self->memQ[off].reltime, self->memQ[off].buf, self->memQ[off].size);
				self->memQ[off].size = 0;
				if (!skipped) self->memI[1] = (self->memI[1] + 1) % max_delay;
			} else {
				self->memQ[off].reltime -= n_samples;
				skipped = 1;
			}
		} else if (!skipped) self->memI[1] = off;

		if (off == woff) break;
	}

#if 0
	// TODO check overflow
	self->memI[3] += self->n_samples;
#endif
}

void filter_init_mididelay(MidiFilter* self) {
	int c,k;
	srandom ((unsigned int) time (NULL));
	self->memI[0] = MAX(self->samplerate / 16.0, 16);
	self->memI[1] = 0; // read-pointer
	self->memI[2] = 0; // write-pointer
	self->memQ = calloc(self->memI[0], sizeof(MidiEventQueue));
	self->preproc_fn = filter_preproc_mididelay;
	self->postproc_fn = filter_postproc_mididelay;
	self->cleanup_fn = filter_cleanup_mididelay;

	for (c=0; c < 16; ++c) for (k=0; k < 127; ++k) {
		self->memCI[c][k] = -1; // current key delay
	}
#if 0
	self->memI[3] = 0; // tme
#endif
}

#endif
