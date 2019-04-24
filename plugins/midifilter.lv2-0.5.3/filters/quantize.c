MFD_FILTER(quantize)

#ifdef MX_TTF

	mflt:quantize
	TTF_DEF("MIDI Quantization", "MIDI Quantize", ; atom:supports time:Position)
	, TTF_IPORT( 0, "bpmsrc",  "BPM source", 0, 1, 1,
			lv2:scalePoint [ rdfs:label "Control Port (freerun)" ; rdf:value 0 ] ;
			lv2:scalePoint [ rdfs:label "Plugin Host (if available)" ; rdf:value 1 ] ;
			lv2:portProperty lv2:integer; lv2:portProperty lv2:enumeration;
			)
	, TTF_IPORT(1, "bpm", "BPM", 1.0, 280.0,  120.0, units:unit units:bpm;
			rdfs:comment "base unit for the time (unless host provides BPM)")
	, TTF_IPORT(2, "quant", "Quantization Grid", .00390625, 4.0, .25,
			lv2:scalePoint [ rdfs:label "32th" ; rdf:value 0.125 ] ;
			lv2:scalePoint [ rdfs:label "16th" ; rdf:value 0.25 ] ;
			lv2:scalePoint [ rdfs:label "Eighth" ; rdf:value 0.5 ] ;
			lv2:scalePoint [ rdfs:label "Quarter" ; rdf:value 1.0 ] ;
			lv2:scalePoint [ rdfs:label "Half Note" ; rdf:value 2.0 ] ;
			lv2:scalePoint [ rdfs:label "Whole Note" ; rdf:value 4.0 ])
	, TTF_IPORT(3, "mindur", "Note-off behaviour", 0, 1, 1,
			lv2:scalePoint [ rdfs:label "Quantize as is (may result in zero duration)" ; rdf:value 0 ] ;
			lv2:scalePoint [ rdfs:label "Enforce minimum duration of one grid unit" ; rdf:value 1 ] ;
			lv2:portProperty lv2:integer; lv2:portProperty lv2:enumeration;
			)
	; rdfs:comment "Live event quantization. This filter aligns incoming MIDI events to a fixed time-grid. Since the effect operates on a live-stream it will introduce latency: Events will be delayed until the next 'tick'. If the plugin-host provides BBT information, the events are aligned to the host's clock otherwise the effect runs on its own time."
	.

#elif defined MX_CODE

static void
filter_cleanup_quantize(MidiFilter* self)
{
	free(self->memQ);
}

static inline void filter_quantize_panic(MidiFilter* self, const uint8_t c, const uint32_t tme) {
	int i,k;
	const int max_delay = self->memI[0];
	for (i=0; i < max_delay; ++i) {
		if (self->memQ[i].size == 3
				&& (self->memQ[i].buf[0] & 0xf0) != 0xf0
				&& (self->memQ[i].buf[0] & 0x0f) != c)
			continue;
		self->memQ[i].size = 0;
	}

	for (k=0; k < 127; ++k) {
		if (self->memCS[c][k]) {
			uint8_t buf[3];
			buf[0] = MIDI_NOTEOFF | c;
			buf[1] = k; buf[2] = 0;
			forge_midimessage(self, tme, buf, 3);
		}
		self->memCS[c][k] = 0; // count note-on per key post-delay
		self->memCM[c][k] = 0; // count note-on per key pre-delay
		self->memCI[c][k] = -1000; // last note-on time
	}
}

void
filter_midi_quantize(MidiFilter* self,
		const uint32_t tme,
		const uint8_t* const buffer,
		const uint32_t size)
{
	if (size != 3) {
		forge_midimessage(self, tme, buffer, size);
		return;
	}

	if (midi_is_panic(buffer, size)) {
		filter_quantize_panic(self, buffer[0]&0x0f, tme);
	}

	if ((self->memI[2] + 1) % self->memI[0] == self->memI[1]) {
		return; // queue full
	}

	float bpm = MAX(*self->cfg[1], 1.0);
	if (*self->cfg[0] && (self->available_info & NFO_BPM)) {
		bpm = self->bpm;
	}
	if (bpm <= 0) bpm = 60;

	const float grid = RAIL((*self->cfg[2]), 1/256.0, 4.0);
	const double samples_per_beat = 60.0 / bpm * self->samplerate;

	int delay = 0;
	uint8_t buf[3];
	memcpy(buf, buffer, 3);

	if (*self->cfg[0] && (self->available_info & (NFO_BPM | NFO_BEAT)) == (NFO_BPM | NFO_BEAT)) {
		/* align to host BBT 
		 * add /one sample/ to delay exact on-beat data by announced latency
		 */
		const double now =  self->bar_beats + (tme + 1)/ samples_per_beat;
		const double next = ((now / grid) - floor(now / grid)) * grid;
		delay = rint((grid - next) * samples_per_beat);
		//printf("DEBUG SYNC'ed now:%f  next:%f grid: %f smpl:%d\n", now, next, grid, delay);
	} else {
		/* free run */
		const double now = (double) ((self->memI[3] + tme)%MSC_MAX) / samples_per_beat; // TODO wrap-around align to SPP
		const double next = ((now / grid) - floor(now / grid)) * grid;
		delay = rint((grid - next) * samples_per_beat);
		//printf("DEBUG FREERUN now:%f  next:%f grid: %f smpl:%d\n", now, next, grid, delay);
	}

	const uint8_t chn = buffer[0] & 0x0f;
	const uint8_t key = buffer[1] & 0x0f;
	const uint8_t vel = buffer[2] & 0x7f;
	uint8_t mst = buffer[0] & 0xf0;

	/* treat note-on w/velocity==0 as note-off */
	if (mst == MIDI_NOTEON && vel == 0) {
		mst = MIDI_NOTEOFF;
		buf[0] = MIDI_NOTEOFF | chn;
	}

	/* keep track of note on/off timing */
	switch (mst) {
		case MIDI_NOTEON:
			self->memCI[chn][key] = (self->memI[3] + delay + tme)%MSC_MAX;
			self->memCM[chn][key]++;
			break;
		case MIDI_NOTEOFF:
			/* enforce minimum duration */
			if ((*self->cfg[3]) && grid > 0 && self->memCM[chn][key] > 0) {
				double tdiff =  MSC_DIFF((self->memI[3] + delay + tme)%MSC_MAX, self->memCI[chn][key]);
				if (tdiff == 0) delay += grid * samples_per_beat;
			}
			if (self->memCM[chn][key] > 0) {
				self->memCM[chn][key]--;
			}
		break;
	}

	MidiEventQueue *qm = &(self->memQ[self->memI[2]]);
	memcpy(qm->buf, buf, size);
	qm->size = size;
	qm->reltime = tme + delay;
	self->memI[2] = (self->memI[2] + 1) % self->memI[0];
}

static void
filter_preproc_quantize(MidiFilter* self)
{
	float bpm = (*self->cfg[1]);
	if (*self->cfg[0] && (self->available_info & NFO_BPM)) {
		bpm = self->bpm;
	}
	if (bpm <= 0) bpm = 60;

	self->latency = floor(self->samplerate * (*self->cfg[2]) * 60.0 / bpm);
}

static void
filter_postproc_quantize(MidiFilter* self)
{
	int i;
	const int max_delay = self->memI[0];
	const int roff = self->memI[1];
	const int woff = self->memI[2];
	const uint32_t n_samples = self->n_samples;
	int skipped = 0;

	for (i=0; i < max_delay; ++i) {
		const int off = (i + roff) % max_delay;
		if (self->memQ[off].size > 0) {
			if (self->memQ[off].reltime < n_samples) {

				if (self->memQ[off].size == 3 && (self->memQ[off].buf[0] & 0xf0) == MIDI_NOTEON) {
					const uint8_t chn = self->memQ[off].buf[0] & 0x0f;
					const uint8_t key = self->memQ[off].buf[1] & 0x7f;
					self->memCS[chn][key]++;
					if (self->memCS[chn][key] > 1) { // send a note-off first
						uint8_t buf[3];
						buf[0] = MIDI_NOTEOFF | chn;
						buf[1] = key; buf[2] = 0;
						forge_midimessage(self, self->memQ[off].reltime, buf, 3);
					}
					forge_midimessage(self, self->memQ[off].reltime, self->memQ[off].buf, self->memQ[off].size);
				}
				else if (self->memQ[off].size == 3 && (self->memQ[off].buf[0] & 0xf0) == MIDI_NOTEOFF) {
					const uint8_t chn = self->memQ[off].buf[0] & 0x0f;
					const uint8_t key = self->memQ[off].buf[1] & 0x7f;
					if (self->memCS[chn][key] > 0) {
						self->memCS[chn][key]--;
						if (self->memCS[chn][key] == 0) {
							forge_midimessage(self, self->memQ[off].reltime, self->memQ[off].buf, self->memQ[off].size);
						}
					}
				} else {
					forge_midimessage(self, self->memQ[off].reltime, self->memQ[off].buf, self->memQ[off].size);
				}

				self->memQ[off].size = 0;
				if (!skipped) self->memI[1] = (self->memI[1] + 1) % max_delay;
			} else {
				self->memQ[off].reltime -= n_samples;
				skipped = 1;
			}
		} else if (!skipped) self->memI[1] = off;

		if (off == woff) break;
	}

	self->memI[3] = (self->memI[3] + n_samples)%MSC_MAX;
}

void filter_init_quantize(MidiFilter* self) {
	int c,k;
	srandom ((unsigned int) time (NULL));

	self->memI[0] = MAX(self->samplerate / 16.0, 16);
	self->memI[1] = 0; // read-pointer
	self->memI[2] = 0; // write-pointer
	self->memQ = calloc(self->memI[0], sizeof(MidiEventQueue));

	self->memI[3] = 0; // monotonic time

	self->preproc_fn = filter_preproc_quantize;
	self->postproc_fn = filter_postproc_quantize;
	self->cleanup_fn = filter_cleanup_quantize;

	for (c=0; c < 16; ++c) for (k=0; k < 127; ++k) {
		self->memCS[c][k] = 0; // count note-on per key post-delay
		self->memCM[c][k] = 0; // count note-on per key pre-delay
		self->memCI[c][k] = -1000; // last note-on time
	}
}

#endif
