/* x42 jack wrapper / minimal LV2 host
 *
 * Copyright (C) 2012-2019 Robin Gareus
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef UPDATE_FREQ_RATIO
#define UPDATE_FREQ_RATIO 60 // MAX # of audio-cycles per GUI-refresh
#endif

#ifndef JACK_AUTOCONNECT
#define JACK_AUTOCONNECT 0
#endif

#ifndef UI_UPDATE_FPS
#define UI_UPDATE_FPS 25
#endif

#ifndef MAXDELAY
#define MAXDELAY 192001 // delayline max possible delay
#endif

#ifndef MAXPERIOD
#define MAXPERIOD 8192 // delayline - max period size (jack-period)
#endif

///////////////////////////////////////////////////////////////////////////////

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef WIN32
#include <pthread.h>
#include <windows.h>
#define pthread_t //< override jack.h def
#endif

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
extern void rtk_osx_api_init (void);
extern void rtk_osx_api_terminate (void);
extern void rtk_osx_api_run (void);
extern void rtk_osx_api_err (const char* msg);
#endif

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#if (defined _WIN32 && defined RTK_STATIC_INIT)
#include <glib-object.h>
#endif

#ifndef _WIN32
#include <sys/mman.h>
#else
#include <sys/timeb.h>
#endif

#ifdef USE_WEAK_JACK
#include "weakjack/weak_libjack.h"
#else
#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/ringbuffer.h>
#endif
#undef pthread_t

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/time/time.h"
#include "lv2/lv2plug.in/ns/ext/uri-map/uri-map.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/worker/worker.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#include "./gl/xternalui.h"

#ifndef WIN32
#include <pthread.h>
#include <signal.h>
#endif

#define LV2_EXTERNAL_UI_RUN(ptr) (ptr)->run (ptr)
#define LV2_EXTERNAL_UI_SHOW(ptr) (ptr)->show (ptr)
#define LV2_EXTERNAL_UI_HIDE(ptr) (ptr)->hide (ptr)

#define nan NAN

#ifndef UINT32_MAX
#define UINT32_MAX (4294967295U)
#endif

static const LV2_Descriptor*   plugin_dsp;
static const LV2UI_Descriptor* plugin_gui;

static LV2_Handle   plugin_instance = NULL;
static LV2UI_Handle gui_instance    = NULL;

static float* plugin_ports_pre  = NULL;
static float* plugin_ports_post = NULL;

static LV2_Atom_Sequence* atom_in  = NULL;
static LV2_Atom_Sequence* atom_out = NULL;

static jack_port_t** input_port  = NULL;
static jack_port_t** output_port = NULL;

static jack_port_t* midi_in  = NULL;
static jack_port_t* midi_out = NULL;

static jack_client_t* j_client     = NULL;
static uint32_t       j_samplerate = 48000;

static int _freewheeling = 0;

struct transport_position {
	jack_nframes_t position;
	float          bpm;
	bool           rolling;
} j_transport = { 0, 0, false };

static jack_ringbuffer_t* rb_ctrl_to_ui   = NULL;
static jack_ringbuffer_t* rb_ctrl_from_ui = NULL;
static jack_ringbuffer_t* rb_atom_to_ui   = NULL;
static jack_ringbuffer_t* rb_atom_from_ui = NULL;

#ifdef HAVE_LIBLO
#include <lo/lo.h>
lo_server_thread          osc_server   = NULL;
static jack_ringbuffer_t* rb_osc_to_ui = NULL;

typedef struct _osc_midi_event {
	size_t  size;
	uint8_t buffer[3];
} osc_midi_event_t;

#ifndef OSC_MIDI_QUEUE_SIZE
#define OSC_MIDI_QUEUE_SIZE (256)
#endif

static osc_midi_event_t event_queue[OSC_MIDI_QUEUE_SIZE];

static int queued_events_start = 0;
static int queued_events_end   = 0;
#endif

static pthread_mutex_t gui_thread_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  data_ready      = PTHREAD_COND_INITIALIZER;

static uint32_t uri_midi_MidiEvent     = 0;
static uint32_t uri_atom_Sequence      = 0;
static uint32_t uri_atom_EventTransfer = 0;

static uint32_t uri_time_Position       = 0;
static uint32_t uri_time_frame          = 0;
static uint32_t uri_time_speed          = 0;
static uint32_t uri_time_bar            = 0;
static uint32_t uri_time_barBeat        = 0;
static uint32_t uri_time_beatUnit       = 0;
static uint32_t uri_time_beatsPerBar    = 0;
static uint32_t uri_time_beatsPerMinute = 0;

static char**   urimap     = NULL;
static uint32_t urimap_len = 0;

enum PortType {
	CONTROL_IN = 0,
	CONTROL_OUT,
	AUDIO_IN,
	AUDIO_OUT,
	MIDI_IN,
	MIDI_OUT,
	ATOM_IN,
	ATOM_OUT
};

struct DelayBuffer {
	jack_latency_range_t port_latency;
	int   wanted_delay;
	int   c_dly; // current delay
	int   w_ptr;
	int   r_ptr;
	float out_buffer[MAXPERIOD];
	float delay_buffer[MAXDELAY];
};

struct PValue {
	uint32_t port_idx;
	float    value;
};

struct LV2Port {
	const char*   name;
	enum PortType porttype;
	float         val_default;
	float         val_min;
	float         val_max;
	const char*   doc;
};

typedef struct _RtkLv2Description {
	const LV2_Descriptor* (*lv2_descriptor) (uint32_t index);
	const LV2UI_Descriptor* (*lv2ui_descriptor) (uint32_t index);

	const uint32_t dsp_descriptor_id;
	const uint32_t gui_descriptor_id;
	const char*    plugin_human_id;

	const struct LV2Port* ports;

	const uint32_t nports_total;
	const uint32_t nports_audio_in;
	const uint32_t nports_audio_out;
	const uint32_t nports_midi_in;
	const uint32_t nports_midi_out;
	const uint32_t nports_atom_in;
	const uint32_t nports_atom_out;
	const uint32_t nports_ctrl;
	const uint32_t nports_ctrl_in;
	const uint32_t nports_ctrl_out;
	const uint32_t min_atom_bufsiz;
	const bool     send_time_info;
	const uint32_t latency_ctrl_port;
} RtkLv2Description;

static RtkLv2Description const* inst;

/* a simple state machine for this client */
static volatile enum {
	Run,
	Exit
} client_state = Run;

static struct lv2_external_ui_host extui_host;
static struct lv2_external_ui*     extui = NULL;

static LV2UI_Controller controller = NULL;

static LV2_Atom_Forge lv2_forge;

static uint32_t* portmap_a_in;
static uint32_t* portmap_a_out;
static uint32_t* portmap_rctl;
static uint32_t* portmap_ctrl;
static uint32_t  portmap_atom_to_ui   = -1;
static uint32_t  portmap_atom_from_ui = -1;

static uint32_t uri_to_id (LV2_URI_Map_Callback_Data callback_data, const char* uri);

static jack_ringbuffer_t*    worker_requests  = NULL;
static jack_ringbuffer_t*    worker_responses = NULL;
static pthread_t             worker_thread;
static pthread_mutex_t       worker_lock  = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t        worker_ready = PTHREAD_COND_INITIALIZER;
static LV2_Worker_Interface* worker_iface = NULL;

static pthread_mutex_t port_write_lock = PTHREAD_MUTEX_INITIALIZER;

static struct DelayBuffer** delayline             = NULL;
static uint32_t             worst_capture_latency = 0;
static uint32_t             plugin_latency        = 0;

/******************************************************************************
 * Delayline for latency compensation
 */
#define FADE_LEN (16)

#define INCREMENT_PTRS                      \
  dly->r_ptr = (dly->r_ptr + 1) % MAXDELAY; \
  dly->w_ptr = (dly->w_ptr + 1) % MAXDELAY;

static float*
delay_port (struct DelayBuffer* dly, uint32_t n_samples, float* in)
{
	uint32_t           pos    = 0;
	const int          delay  = dly->wanted_delay;
	const float* const input  = in;
	float* const       output = dly->out_buffer;

	if (dly->c_dly == delay && delay == 0) {
		// only copy data into buffer in case delay time changes
		for (; pos < n_samples; pos++) {
			dly->delay_buffer[dly->w_ptr] = input[pos];
			INCREMENT_PTRS;
		}
		return in;
	}

	// fade if delaytime changes
	if (dly->c_dly != delay) {
		const uint32_t fade_len = (n_samples >= FADE_LEN) ? FADE_LEN : n_samples / 2;

		// fade out
		for (; pos < fade_len; pos++) {
			const float gain              = (float)(fade_len - pos) / (float)fade_len;
			dly->delay_buffer[dly->w_ptr] = input[pos];
			output[pos]                   = dly->delay_buffer[dly->r_ptr] * gain;
			INCREMENT_PTRS;
		}

		// update read pointer
		dly->r_ptr += dly->c_dly - delay;
		if (dly->r_ptr < 0) {
			dly->r_ptr -= MAXDELAY * floor (dly->r_ptr / (float)MAXDELAY);
		}

		dly->r_ptr = dly->r_ptr % MAXDELAY;
		dly->c_dly = delay;

		// fade in
		for (; pos < 2 * fade_len; pos++) {
			const float gain              = (float)(pos - fade_len) / (float)fade_len;
			dly->delay_buffer[dly->w_ptr] = input[pos];
			output[pos]                   = dly->delay_buffer[dly->r_ptr] * gain;
			INCREMENT_PTRS;
		}
	}

	for (; pos < n_samples; pos++) {
		dly->delay_buffer[dly->w_ptr] = input[pos];
		output[pos]                   = dly->delay_buffer[dly->r_ptr];
		INCREMENT_PTRS;
	}

	return dly->out_buffer;
}

///////////////////////////
// GET INFO FROM LV2 TTL //
//     see lv2ttl2c      //
//    define _plugin     //
///////////////////////////
#include JACK_DESCRIPT ////
///////////////////////////

/******************************************************************************
 * JACK
 */

static int
process (jack_nframes_t nframes, void* arg)
{
	if (nframes > MAXPERIOD) {
		static bool warned_max_period = false;
		if (!warned_max_period) {
			warned_max_period = true;
			fprintf (stderr, "Jack Period Size > %d is not supported (current %d)\n", MAXPERIOD, nframes);
		}
		if (inst->nports_midi_out > 0) {
			void* buf = jack_port_get_buffer (midi_out, nframes);
			jack_midi_clear_buffer (buf);
		}
		for (uint32_t i = 0; i < inst->nports_audio_out; ++i) {
			float* bp = (float*)jack_port_get_buffer (output_port[i], nframes);
			memset (bp, 0, nframes * sizeof (float));
		}
		return 0;
	}

	while (jack_ringbuffer_read_space (rb_ctrl_from_ui) >= sizeof (uint32_t) + sizeof (float)) {
		uint32_t idx;
		jack_ringbuffer_read (rb_ctrl_from_ui, (char*)&idx, sizeof (uint32_t));
		jack_ringbuffer_read (rb_ctrl_from_ui, (char*)&(plugin_ports_pre[idx]), sizeof (float));
	}

	/* Get Jack transport position */
	jack_position_t pos;
	const bool      rolling           = (jack_transport_query (j_client, &pos) == JackTransportRolling);
	const bool      transport_changed = (rolling != j_transport.rolling || pos.frame != j_transport.position || ((pos.valid & JackPositionBBT) && (pos.beats_per_minute != j_transport.bpm)));

	/* atom buffers */
	if (inst->nports_atom_in > 0 || inst->nports_midi_in > 0) {
		/* start Atom sequence */
		atom_in->atom.type           = uri_atom_Sequence;
		atom_in->atom.size           = 8;
		LV2_Atom_Sequence_Body* body = &atom_in->body;
		body->unit                   = 0; // URID of unit of event time stamp LV2_ATOM__timeUnit ??
		body->pad                    = 0; // unused
		uint8_t* seq                 = (uint8_t*)(body + 1);

		if (transport_changed && inst->send_time_info) {
			uint8_t   pos_buf[256];
			LV2_Atom* lv2_pos = (LV2_Atom*)pos_buf;

			lv2_atom_forge_set_buffer (&lv2_forge, pos_buf, sizeof (pos_buf));
			LV2_Atom_Forge*      forge = &lv2_forge;
			LV2_Atom_Forge_Frame frame;
#ifdef HAVE_LV2_1_8
			lv2_atom_forge_object (&lv2_forge, &frame, 1, uri_time_Position);
#else
			lv2_atom_forge_blank (&lv2_forge, &frame, 1, uri_time_Position);
#endif
			lv2_atom_forge_property_head (forge, uri_time_frame, 0);
			lv2_atom_forge_long (forge, pos.frame);
			lv2_atom_forge_property_head (forge, uri_time_speed, 0);
			lv2_atom_forge_float (forge, rolling ? 1.0 : 0.0);
			if (pos.valid & JackPositionBBT) {
				lv2_atom_forge_property_head (forge, uri_time_barBeat, 0);
				lv2_atom_forge_float (forge, pos.beat - 1 + (pos.tick / pos.ticks_per_beat));
				lv2_atom_forge_property_head (forge, uri_time_bar, 0);
				lv2_atom_forge_long (forge, pos.bar - 1);
				lv2_atom_forge_property_head (forge, uri_time_beatUnit, 0);
				lv2_atom_forge_int (forge, pos.beat_type);
				lv2_atom_forge_property_head (forge, uri_time_beatsPerBar, 0);
				lv2_atom_forge_float (forge, pos.beats_per_bar);
				lv2_atom_forge_property_head (forge, uri_time_beatsPerMinute, 0);
				lv2_atom_forge_float (forge, pos.beats_per_minute);
			}

			uint32_t size        = lv2_pos->size;
			uint32_t padded_size = ((sizeof (LV2_Atom_Event) + size) + 7) & (~7);

			if (inst->min_atom_bufsiz > padded_size) {
				//printf("send time..\n");
				LV2_Atom_Event* aev = (LV2_Atom_Event*)seq;
				aev->time.frames    = 0;
				aev->body.size      = size;
				aev->body.type      = lv2_pos->type;
				memcpy (LV2_ATOM_BODY (&aev->body), LV2_ATOM_BODY (lv2_pos), size);
				atom_in->atom.size += padded_size;
				seq += padded_size;
			}
		}

		if (gui_instance) {
			while (jack_ringbuffer_read_space (rb_atom_from_ui) > sizeof (LV2_Atom)) {
				LV2_Atom a;
				jack_ringbuffer_read (rb_atom_from_ui, (char*)&a, sizeof (LV2_Atom));
				uint32_t padded_size = atom_in->atom.size + a.size + sizeof (int64_t);
				if (inst->min_atom_bufsiz > padded_size) {
					memset (seq, 0, sizeof (int64_t)); // LV2_Atom_Event->time
					seq += sizeof (int64_t);
					jack_ringbuffer_read (rb_atom_from_ui, (char*)seq, a.size);
					seq += a.size;
					atom_in->atom.size += a.size + sizeof (int64_t);
				}
			}
		}

		if (inst->nports_midi_in > 0) {
#ifdef HAVE_LIBLO
			/*inject OSC midi events, use time 0 */
			while (queued_events_end != queued_events_start) {
				uint32_t size        = event_queue[queued_events_end].size;
				uint32_t padded_size = ((sizeof (LV2_Atom_Event) + size) + 7) & (~7);

				if (inst->min_atom_bufsiz > padded_size) {
					LV2_Atom_Event* aev = (LV2_Atom_Event*)seq;
					aev->time.frames    = 0; // time
					aev->body.size      = size;
					aev->body.type      = uri_midi_MidiEvent;
					memcpy (LV2_ATOM_BODY (&aev->body), event_queue[queued_events_end].buffer, size);
					atom_in->atom.size += padded_size;
					seq += padded_size;
				}

				queued_events_end = (queued_events_end + 1) % OSC_MIDI_QUEUE_SIZE;
			}
#endif

			/* inject jack midi events */
			void* buf = jack_port_get_buffer (midi_in, nframes);
			for (uint32_t i = 0; i < jack_midi_get_event_count (buf); ++i) {
				jack_midi_event_t ev;
				jack_midi_event_get (&ev, buf, i);

				uint32_t size        = ev.size;
				uint32_t padded_size = ((sizeof (LV2_Atom_Event) + size) + 7) & (~7);

				if (inst->min_atom_bufsiz > padded_size) {
					LV2_Atom_Event* aev = (LV2_Atom_Event*)seq;
					aev->time.frames    = ev.time;
					aev->body.size      = size;
					aev->body.type      = uri_midi_MidiEvent;
					memcpy (LV2_ATOM_BODY (&aev->body), ev.buffer, size);
					atom_in->atom.size += padded_size;
					seq += padded_size;
				}
			}
		}
	}

	if (inst->nports_atom_out > 0 || inst->nports_midi_out > 0) {
		atom_out->atom.type = 0;
		atom_out->atom.size = inst->min_atom_bufsiz;
	}

	/* make a backup copy, to see what was changed */
	memcpy (plugin_ports_post, plugin_ports_pre, inst->nports_ctrl * sizeof (float));

	/* expected transport state in next cycle */
	j_transport.position = rolling ? pos.frame + nframes : pos.frame;
	j_transport.bpm      = pos.beats_per_minute;
	j_transport.rolling  = rolling;

	/* [re] connect jack audio buffers */
	for (uint32_t i = 0; i < inst->nports_audio_out; i++) {
		plugin_dsp->connect_port (plugin_instance, portmap_a_out[i], jack_port_get_buffer (output_port[i], nframes));
	}

	for (uint32_t i = 0; i < inst->nports_audio_in; i++) {
		delayline[i]->wanted_delay = worst_capture_latency - delayline[i]->port_latency.max;
		plugin_dsp->connect_port (
		    plugin_instance, portmap_a_in[i],
		    delay_port (delayline[i], nframes, (float*)jack_port_get_buffer (input_port[i], nframes)));
	}

	/* run the plugin */
	plugin_dsp->run (plugin_instance, nframes);

	/* handle worker emit response  - may amend Atom seq... */
	if (worker_responses) {
		uint32_t read_space = jack_ringbuffer_read_space (worker_responses);
		while (read_space) {
			uint32_t size = 0;
			char     worker_response[4096];
			jack_ringbuffer_read (worker_responses, (char*)&size, sizeof (size));
			jack_ringbuffer_read (worker_responses, worker_response, size);
			worker_iface->work_response (plugin_instance, size, worker_response);
			read_space -= sizeof (size) + size;
		}
	}

	/* create port-events for change values */

	if (gui_instance) {
		for (uint32_t p = 0; p < inst->nports_ctrl; p++) {
			if (inst->ports[portmap_rctl[p]].porttype != CONTROL_OUT)
				continue;

			if (plugin_ports_pre[p] != plugin_ports_post[p]) {
				if (inst->latency_ctrl_port != UINT32_MAX && p == portmap_ctrl[inst->latency_ctrl_port]) {
					plugin_latency = rintf (plugin_ports_pre[p]);
					// TODO handle case if there's no GUI thread to call
					// jack_recompute_total_latencies()
				}
				if (jack_ringbuffer_write_space (rb_ctrl_to_ui) >= sizeof (uint32_t) + sizeof (float)) {
					jack_ringbuffer_write (rb_ctrl_to_ui, (char*)&portmap_rctl[p], sizeof (uint32_t));
					jack_ringbuffer_write (rb_ctrl_to_ui, (char*)&plugin_ports_pre[p], sizeof (float));
				}
			}
		}
	}

	if (inst->nports_midi_out > 0) {
		void* buf = jack_port_get_buffer (midi_out, nframes);
		jack_midi_clear_buffer (buf);
	}

	/* Atom sequence port-events */
	if (inst->nports_atom_out + inst->nports_midi_out > 0 && atom_out->atom.size > sizeof (LV2_Atom)) {
		if (gui_instance && jack_ringbuffer_write_space (rb_atom_to_ui) >= atom_out->atom.size + 2 * sizeof (LV2_Atom)) {
			LV2_Atom a = { atom_out->atom.size + (uint32_t)sizeof (LV2_Atom), 0 };
			jack_ringbuffer_write (rb_atom_to_ui, (char*)&a, sizeof (LV2_Atom));
			jack_ringbuffer_write (rb_atom_to_ui, (char*)atom_out, a.size);
		}

		if (inst->nports_midi_out) {
			void*                 buf = jack_port_get_buffer (midi_out, nframes);
			LV2_Atom_Event const* ev  = (LV2_Atom_Event const*)((&(atom_out)->body) + 1); // lv2_atom_sequence_begin
			while ((const uint8_t*)ev < ((const uint8_t*)&(atom_out)->body + (atom_out)->atom.size)) {
				if (ev->body.type == uri_midi_MidiEvent) {
					jack_midi_event_write (buf, ev->time.frames, (const uint8_t*)(ev + 1), ev->body.size);
				}
				ev = (LV2_Atom_Event const*)/* lv2_atom_sequence_next() */
				    ((const uint8_t*)ev + sizeof (LV2_Atom_Event) + ((ev->body.size + 7) & ~7));
			}
		}
	}

	/* signal worker end of process run */
	if (worker_iface && worker_iface->end_run) {
		worker_iface->end_run (plugin_instance);
	}

	/* wake up UI */
	if (gui_instance && (
	                     jack_ringbuffer_read_space (rb_ctrl_to_ui) >= sizeof (uint32_t) + sizeof (float)
	                     || jack_ringbuffer_read_space (rb_atom_to_ui) > sizeof (LV2_Atom)
#ifdef HAVE_LIBLO
	                     || jack_ringbuffer_read_space (rb_osc_to_ui) >= sizeof (uint32_t) + sizeof (float)
#endif
	                    )
	   ) {
		if (pthread_mutex_trylock (&gui_thread_lock) == 0) {
			pthread_cond_signal (&data_ready);
			pthread_mutex_unlock (&gui_thread_lock);
		}
	}
	return 0;
}

static void
jack_shutdown (void* arg)
{
	fprintf (stderr, "recv. shutdown request from jackd.\n");
	client_state = Exit;
	pthread_cond_signal (&data_ready);
}

static int
jack_graph_order_cb (void* arg)
{
	worst_capture_latency = 0;
	for (uint32_t i = 0; i < inst->nports_audio_in; i++) {
		jack_port_get_latency_range (input_port[i], JackCaptureLatency, &(delayline[i]->port_latency));
		if (delayline[i]->port_latency.max > worst_capture_latency) {
			worst_capture_latency = delayline[i]->port_latency.max;
		}
	}
	return 0;
}

static void
jack_latency_cb (jack_latency_callback_mode_t mode, void* arg)
{
	// assume 1 -> 1 map
	jack_graph_order_cb (NULL); // update worst-case latency, delayline alignment
	if (mode == JackCaptureLatency) {
		for (uint32_t i = 0; i < inst->nports_audio_out; i++) {
			jack_latency_range_t r;
			if (i < inst->nports_audio_in) {
				const uint32_t port_delay = worst_capture_latency - delayline[i]->port_latency.max;
				jack_port_get_latency_range (input_port[i], JackCaptureLatency, &r);
				r.min += port_delay;
				r.max += port_delay;
			} else {
				r.min = r.max = 0;
			}
			r.min += plugin_latency;
			r.max += plugin_latency;
			jack_port_set_latency_range (output_port[i], JackCaptureLatency, &r);
		}
	} else { // JackPlaybackLatency
		for (uint32_t i = 0; i < inst->nports_audio_in; i++) {
			const uint32_t       port_delay = worst_capture_latency - delayline[i]->port_latency.max;
			jack_latency_range_t r;
			if (i < inst->nports_audio_out) {
				jack_port_get_latency_range (output_port[i], JackPlaybackLatency, &r);
			} else {
				r.min = r.max = 0;
			}
			r.min += port_delay + plugin_latency;
			r.max += port_delay + plugin_latency;
			jack_port_set_latency_range (input_port[i], JackPlaybackLatency, &r);
		}
	}
}

static void
jack_freewheel_cb (int onoff, void* arg)
{
	_freewheeling = onoff;
}

static int
init_jack (const char* client_name)
{
	jack_status_t status;
	char*         cn = strdup (client_name);
	if (strlen (cn) >= (unsigned int)jack_client_name_size () - 1) {
		cn[jack_client_name_size () - 1] = '\0';
	}
	j_client = jack_client_open (cn, JackNoStartServer, &status);
	free (cn);
	if (j_client == NULL) {
		fprintf (stderr, "jack_client_open() failed, status = 0x%2.0x\n", status);
		if (status & JackServerFailed) {
			fprintf (stderr, "Unable to connect to JACK server\n");
		}
		return (-1);
	}
	if (status & JackServerStarted) {
		fprintf (stderr, "JACK server started\n");
	}
	if (status & JackNameNotUnique) {
		client_name = jack_get_client_name (j_client);
		fprintf (stderr, "jack-client name: `%s'\n", client_name);
	}

	jack_set_process_callback (j_client, process, 0);
	jack_set_graph_order_callback (j_client, jack_graph_order_cb, 0);
	jack_set_latency_callback (j_client, jack_latency_cb, 0);
	jack_set_freewheel_callback (j_client, jack_freewheel_cb, 0);

#ifndef WIN32
	jack_on_shutdown (j_client, jack_shutdown, NULL);
#endif
	j_samplerate = jack_get_sample_rate (j_client);
	return (0);
}

static int
jack_portsetup (void)
{
	/* Allocate data structures that depend on the number of ports. */
	if (inst->nports_audio_in > 0) {
		input_port = (jack_port_t**)malloc (sizeof (jack_port_t*) * inst->nports_audio_in);
		delayline  = (struct DelayBuffer**)calloc (inst->nports_audio_in, sizeof (struct DelayBuffer*));
	}

	for (uint32_t i = 0; i < inst->nports_audio_in; i++) {
		if ((input_port[i] = jack_port_register (j_client,
		                                         inst->ports[portmap_a_in[i]].name,
		                                         JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0)) == 0) {
			fprintf (stderr, "cannot register input port \"%s\"!\n", inst->ports[portmap_a_in[i]].name);
			return (-1);
		}
		delayline[i] = (struct DelayBuffer*)calloc (1, sizeof (struct DelayBuffer));
	}

	if (inst->nports_audio_out > 0) {
		output_port = (jack_port_t**)malloc (sizeof (jack_port_t*) * inst->nports_audio_out);
	}

	for (uint32_t i = 0; i < inst->nports_audio_out; i++) {
		if ((output_port[i] = jack_port_register (j_client,
		                                          inst->ports[portmap_a_out[i]].name,
		                                          JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0)) == 0) {
			fprintf (stderr, "cannot register output port \"%s\"!\n", inst->ports[portmap_a_out[i]].name);
			return (-1);
		}
	}

	if (inst->nports_midi_in) {
		if ((midi_in = jack_port_register (j_client,
		                                   inst->ports[portmap_atom_from_ui].name,
		                                   JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0)) == 0) {
			fprintf (stderr, "cannot register midi input port \"%s\"!\n", inst->ports[portmap_atom_from_ui].name);
			return (-1);
		}
	}

	if (inst->nports_midi_out) {
		if ((midi_out = jack_port_register (j_client,
		                                    inst->ports[portmap_atom_to_ui].name,
		                                    JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0)) == 0) {
			fprintf (stderr, "cannot register midi output port \"%s\"!\n", inst->ports[portmap_atom_to_ui].name);
			return (-1);
		}
	}
	jack_graph_order_cb (NULL); // query port latencies
	jack_recompute_total_latencies (j_client);
	return (0);
}

static void
jack_portconnect (int which)
{
	if (which & 1) { // connect audio input(s)
		const char** ports = jack_get_ports (j_client, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput | JackPortIsPhysical);
		for (uint32_t i = 0; i < inst->nports_audio_in && ports && ports[i]; i++) {
			if (jack_connect (j_client, ports[i], jack_port_name (input_port[i])))
				break;
		}
		if (ports) {
			jack_free (ports);
		}
	}

	if (which & 2) { // connect audio outputs(s)
		const char** ports = jack_get_ports (j_client, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput | JackPortIsPhysical);
		for (uint32_t i = 0; i < inst->nports_audio_out && ports && ports[i]; i++) {
			if (jack_connect (j_client, jack_port_name (output_port[i]), ports[i]))
				break;
		}
		if (ports) {
			jack_free (ports);
		}
	}

	if ((which & 4) && midi_in) { // midi in
		const char** ports = jack_get_ports (j_client, NULL, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput | JackPortIsPhysical);
		if (ports && ports[0]) {
			jack_connect (j_client, ports[0], jack_port_name (midi_in));
		}
		if (ports) {
			jack_free (ports);
		}
	}

	if ((which & 8) && midi_out) { // midi out
		const char** ports = jack_get_ports (j_client, NULL, JACK_DEFAULT_MIDI_TYPE, JackPortIsInput | JackPortIsPhysical);
		if (ports && ports[0]) {
			jack_connect (j_client, jack_port_name (midi_out), ports[0]);
		}
		if (ports) {
			jack_free (ports);
		}
	}
}

/******************************************************************************
 * LV2
 */

static uint32_t
uri_to_id (LV2_URI_Map_Callback_Data callback_data, const char* uri)
{
	for (uint32_t i = 0; i < urimap_len; ++i) {
		if (!strcmp (urimap[i], uri)) {
			//printf("Found mapped URI '%s' -> %d\n", uri, i + 1);
			return i + 1;
		}
	}
	//printf("map URI '%s' -> %d\n", uri, urimap_len + 1);
	urimap             = (char**)realloc (urimap, (urimap_len + 1) * sizeof (char*));
	urimap[urimap_len] = strdup (uri);
	return ++urimap_len;
}

static void
free_uri_map ()
{
	for (uint32_t i = 0; i < urimap_len; ++i) {
		free (urimap[i]);
	}
	free (urimap);
}

static void
write_function (
    LV2UI_Controller controller,
    uint32_t         port_index,
    uint32_t         buffer_size,
    uint32_t         port_protocol,
    const void*      buffer)
{
	if (buffer_size == 0)
		return;

	if (port_protocol != 0) {
		if (jack_ringbuffer_write_space (rb_atom_from_ui) >= buffer_size + sizeof (LV2_Atom)) {
			LV2_Atom a = { buffer_size, 0 };
			jack_ringbuffer_write (rb_atom_from_ui, (char*)&a, sizeof (LV2_Atom));
			jack_ringbuffer_write (rb_atom_from_ui, (char*)buffer, buffer_size);
		}
		return;
	}
	if (buffer_size != sizeof (float)) {
		fprintf (stderr, "LV2Host: write_function() unsupported buffer\n");
		return;
	}
	if (port_index >= inst->nports_total) {
		fprintf (stderr, "LV2Host: write_function() invalid port\n");
		return;
	}
	if (portmap_ctrl[port_index] == UINT32_MAX) {
		fprintf (stderr, "LV2Host: write_function() unmapped port\n");
		return;
	}
	if (inst->ports[port_index].porttype != CONTROL_IN) {
		fprintf (stderr, "LV2Host: write_function() not a control input\n");
		return;
	}
	if (jack_ringbuffer_write_space (rb_ctrl_from_ui) >= sizeof (uint32_t) + sizeof (float)) {
		pthread_mutex_lock (&port_write_lock);
		jack_ringbuffer_write (rb_ctrl_from_ui, (char*)&portmap_ctrl[port_index], sizeof (uint32_t));
		jack_ringbuffer_write (rb_ctrl_from_ui, (char*)buffer, sizeof (float));
		pthread_mutex_unlock (&port_write_lock);
	}
}

// LV2 Worker

static LV2_Worker_Status
lv2_worker_respond (LV2_Worker_Respond_Handle unused,
                    uint32_t                  size,
                    const void*               data)
{
	jack_ringbuffer_write (worker_responses, (const char*)&size, sizeof (size));
	jack_ringbuffer_write (worker_responses, (const char*)data, size);
	return LV2_WORKER_SUCCESS;
}

static void*
worker_func (void* data)
{
	pthread_mutex_lock (&worker_lock);
	while (1) {
		char     buf[4096];
		uint32_t size = 0;

		if (jack_ringbuffer_read_space (worker_requests) <= sizeof (size)) {
			pthread_cond_wait (&worker_ready, &worker_lock);
		}

		if (client_state == Exit)
			break;

		jack_ringbuffer_read (worker_requests, (char*)&size, sizeof (size));

		if (size > 4096) {
			fprintf (stderr, "Worker information is too large. Abort.\n");
			break;
		}

		jack_ringbuffer_read (worker_requests, buf, size);
		worker_iface->work (plugin_instance, lv2_worker_respond, NULL, size, buf);
	}
	pthread_mutex_unlock (&worker_lock);
	return NULL;
}

static void
worker_init ()
{
	worker_requests  = jack_ringbuffer_create (4096);
	worker_responses = jack_ringbuffer_create (4096);
	jack_ringbuffer_mlock (worker_requests);
	jack_ringbuffer_mlock (worker_responses);
	pthread_create (&worker_thread, NULL, worker_func, NULL);
}

static LV2_Worker_Status
lv2_worker_schedule (LV2_Worker_Schedule_Handle unused,
                     uint32_t                   size,
                     const void*                data)
{
	if (_freewheeling) {
		worker_iface->work (plugin_instance, lv2_worker_respond, NULL, size, data);
		return LV2_WORKER_SUCCESS;
	}
	assert (worker_requests);
	jack_ringbuffer_write (worker_requests, (const char*)&size, sizeof (size));
	jack_ringbuffer_write (worker_requests, (const char*)data, size);
	if (pthread_mutex_trylock (&worker_lock) == 0) {
		pthread_cond_signal (&worker_ready);
		pthread_mutex_unlock (&worker_lock);
	}
	return LV2_WORKER_SUCCESS;
}

/******************************************************************************
 * OSC
 */
#ifdef HAVE_LIBLO

static void
osc_queue_midi_event (osc_midi_event_t* ev)
{
	if (((queued_events_start + 1) % OSC_MIDI_QUEUE_SIZE) == queued_events_end) {
		return;
	}
	memcpy (&event_queue[queued_events_start], ev, sizeof (osc_midi_event_t));
	queued_events_start = (queued_events_start + 1) % OSC_MIDI_QUEUE_SIZE;
}

static void
oscb_error (int num, const char* m, const char* path)
{
	fprintf (stderr, "liblo server error %d in path %s: %s\n", num, path, m);
}

#define MIDI_Q3(STATUS)                        \
  osc_midi_event_t ev;                         \
  ev.size      = 3;                            \
  ev.buffer[0] = STATUS | (argv[0]->i & 0x0f); \
  ev.buffer[1] = argv[1]->i & 0x7f;            \
  ev.buffer[2] = argv[2]->i & 0x7f;            \
  osc_queue_midi_event (&ev);

static int
oscb_noteon (const char* path, const char* types, lo_arg** argv, int argc, lo_message msg, void* user_data)
{
	MIDI_Q3 (0x90);
	return 0;
}

static int
oscb_noteoff (const char* path, const char* types, lo_arg** argv, int argc, lo_message msg, void* user_data)
{
	MIDI_Q3 (0x80);
	return 0;
}

static int
oscb_cc (const char* path, const char* types, lo_arg** argv, int argc, lo_message msg, void* user_data)
{
	MIDI_Q3 (0xb0);
	return 0;
}

static int
oscb_pc (const char* path, const char* types, lo_arg** argv, int argc, lo_message msg, void* user_data)
{
	osc_midi_event_t ev;
	ev.size      = 2;
	ev.buffer[0] = 0xc0 | (argv[0]->i & 0x0f);
	ev.buffer[1] = argv[1]->i & 0x7f;
	ev.buffer[2] = 0x00;
	osc_queue_midi_event (&ev);
	return 0;
}

static int
oscb_rawmidi (const char* path, const char* types, lo_arg** argv, int argc, lo_message msg, void* user_data)
{
	osc_midi_event_t ev;
	ev.size      = 3;
	ev.buffer[0] = argv[0]->m[1];
	ev.buffer[1] = argv[0]->m[2] & 0x7f;
	ev.buffer[2] = argv[0]->m[3] & 0x7f;
	osc_queue_midi_event (&ev);
	return 0;
}

static int
oscb_parameter (const char* path, const char* types, lo_arg** argv, int argc, lo_message msg, void* user_data)
{
	assert (argc == 2 && !strcmp (types, "if"));

	const uint32_t port = argv[0]->i;
	const float    val  = argv[1]->f;

	if (inst->nports_ctrl <= port) {
		fprintf (stderr, "OSC: Invalid Parameter 0 <= %d < %d\n", port, inst->nports_ctrl);
		return 0;
	}

	const uint32_t port_index = portmap_rctl[port];

	if (inst->ports[port_index].porttype != CONTROL_IN) {
		fprintf (stderr, "OSC: mapped port (%d) is not a control input.\n", port_index);
		return 0;
	}

	if (inst->ports[port_index].val_min < inst->ports[port_index].val_max) {
		if (val < inst->ports[port_index].val_min || val > inst->ports[port_index].val_max) {
			fprintf (stderr, "OSC: Value out of bounds %f <= %f <= %f\n",
			         inst->ports[port_index].val_min, val, inst->ports[port_index].val_max);
			return 0;
		}
	}

	//fprintf (stdout, "OSC: %d,  %d -> %f\n", port, port_index, val);

	write_function (NULL, port_index, sizeof (float), 0, (const void*)&val);

	if (jack_ringbuffer_write_space (rb_osc_to_ui) >= sizeof (uint32_t) + sizeof (float)) {
		jack_ringbuffer_write (rb_osc_to_ui, (char*)&port_index, sizeof (uint32_t));
		jack_ringbuffer_write (rb_osc_to_ui, (char*)&val, sizeof (float));
	}

	return 0;
}

struct osc_command {
	const char*       path;
	const char*       typespec;
	lo_method_handler handler;
	const char*       documentation;
};

static struct osc_command OSCC[] = {
	{ "/x42/parameter",    "if",  &oscb_parameter, "Set Control Input value. control-port, value" },
	{ "/x42/midi/raw",     "m",   &oscb_rawmidi,   "Send raw midi message to plugin" },
	{ "/x42/midi/cc",      "iii", &oscb_cc,        "Send midi control-change: channel, parameter, value" },
	{ "/x42/midi/pc",      "ii",  &oscb_pc,        "Send midi program-change: channel, program" },
	{ "/x42/midi/noteon",  "iii", &oscb_noteon,    "Send midi note on: channel, key, velocity" },
	{ "/x42/midi/noteoff", "iii", &oscb_noteoff,   "Send midi note off: channel, key, velocity" },
};

static void
print_oscdoc (void)
{
	printf ("# X42 OSC methods. Format:\n");
	printf ("# Path <space> Typespec <space> Documentation <newline>\n");
	printf ("#######################################################\n");
	for (size_t i = 0; i < sizeof (OSCC) / sizeof (struct osc_command); ++i) {
		printf ("%s %s %s\n", OSCC[i].path, OSCC[i].typespec, OSCC[i].documentation);
	}
}

static int
start_osc_server (int osc_port)
{
	char     tmp[8];
	uint32_t port = (osc_port > 100 && osc_port < 60000) ? osc_port : 9988;

#ifdef _WIN32
	sprintf (tmp, "%d", port);
#else
	snprintf (tmp, sizeof (tmp), "%d", port);
#endif
	osc_server = lo_server_thread_new (tmp, oscb_error);

	if (!osc_server) {
		fprintf (stderr, "OSC failed to listen on port %s", tmp);
		return -1;
	} else {
		char* urlstr = lo_server_thread_get_url (osc_server);
		fprintf (stderr, "OSC server: %s\n", urlstr);
		free (urlstr);
	}

	for (size_t i = 0; i < sizeof (OSCC) / sizeof (struct osc_command); ++i) {
		lo_server_thread_add_method (osc_server, OSCC[i].path, OSCC[i].typespec, OSCC[i].handler, NULL);
	}

	lo_server_thread_start (osc_server);

	return 0;
}

static void
stop_osc_server ()
{
	if (!osc_server)
		return;
	lo_server_thread_stop (osc_server);
	lo_server_thread_free (osc_server);
	fprintf (stderr, "OSC server shut down.\n");
	osc_server = NULL;
}
#endif

/******************************************************************************
 * MAIN
 */

static void
cleanup (int sig)
{
	if (j_client) {
		jack_client_close (j_client);
		j_client = NULL;
	}

	if (worker_requests) {
		pthread_mutex_lock (&worker_lock);
		pthread_cond_signal (&worker_ready);
		pthread_mutex_unlock (&worker_lock);

		pthread_join (worker_thread, NULL);

		jack_ringbuffer_free (worker_requests);
		jack_ringbuffer_free (worker_responses);
	}

#ifdef HAVE_LIBLO
	stop_osc_server ();
#endif

	if (plugin_dsp && plugin_instance && plugin_dsp->deactivate) {
		plugin_dsp->deactivate (plugin_instance);
	}
	if (plugin_gui && gui_instance && plugin_gui->cleanup) {
		plugin_gui->cleanup (gui_instance);
	}
	if (plugin_dsp && plugin_instance && plugin_dsp->cleanup) {
		plugin_dsp->cleanup (plugin_instance);
	}

	jack_ringbuffer_free (rb_ctrl_to_ui);
	jack_ringbuffer_free (rb_ctrl_from_ui);

	jack_ringbuffer_free (rb_atom_to_ui);
	jack_ringbuffer_free (rb_atom_from_ui);

#ifdef HAVE_LIBLO
	jack_ringbuffer_free (rb_osc_to_ui);
#endif

	free (input_port);
	free (output_port);

	if (delayline) {
		for (uint32_t i = 0; i < inst->nports_audio_in; i++) {
			free (delayline[i]);
		}
	}
	free (delayline);

	free (plugin_ports_pre);
	free (plugin_ports_post);
	free (portmap_a_in);
	free (portmap_a_out);
	free (portmap_ctrl);
	free (portmap_rctl);
	free (atom_in);
	free (atom_out);
	free_uri_map ();
	fprintf (stderr, "bye.\n");
}

static void
run_one (LV2_Atom_Sequence* data)
{
#ifdef HAVE_LIBLO
	while (jack_ringbuffer_read_space (rb_osc_to_ui) >= sizeof (uint32_t) + sizeof (float)) {
		uint32_t idx;
		float    val;
		jack_ringbuffer_read (rb_osc_to_ui, (char*)&idx, sizeof (uint32_t));
		jack_ringbuffer_read (rb_osc_to_ui, (char*)&val, sizeof (float));
		plugin_gui->port_event (gui_instance, idx, sizeof (float), 0, &val);
	}
#endif

	while (jack_ringbuffer_read_space (rb_ctrl_to_ui) >= sizeof (uint32_t) + sizeof (float)) {
		uint32_t idx;
		float    val;
		jack_ringbuffer_read (rb_ctrl_to_ui, (char*)&idx, sizeof (uint32_t));
		jack_ringbuffer_read (rb_ctrl_to_ui, (char*)&val, sizeof (float));
		plugin_gui->port_event (gui_instance, idx, sizeof (float), 0, &val);
		if (idx == inst->latency_ctrl_port) {
			// jack client calls cannot be done in the DSP thread with jack1
			jack_recompute_total_latencies (j_client);
		}
	}

	while (jack_ringbuffer_read_space (rb_atom_to_ui) > sizeof (LV2_Atom)) {
		LV2_Atom a;
		jack_ringbuffer_read (rb_atom_to_ui, (char*)&a, sizeof (LV2_Atom));
		assert (a.size < inst->min_atom_bufsiz);
		jack_ringbuffer_read (rb_atom_to_ui, (char*)data, a.size);
		LV2_Atom_Event const* ev = (LV2_Atom_Event const*)((&(data)->body) + 1); // lv2_atom_sequence_begin
		while ((const uint8_t*)ev < ((const uint8_t*)&(data)->body + (data)->atom.size)) {
			plugin_gui->port_event (gui_instance, portmap_atom_to_ui,
			                        ev->body.size, uri_atom_EventTransfer, &ev->body);
			ev = (LV2_Atom_Event const*)/* lv2_atom_sequence_next() */
			    ((const uint8_t*)ev + sizeof (LV2_Atom_Event) + ((ev->body.size + 7) & ~7));
		}
	}

	LV2_EXTERNAL_UI_RUN (extui);
}

#ifdef __APPLE__

static void
osx_loop (CFRunLoopTimerRef timer, void* info)
{
	if (client_state == Run) {
		run_one ((LV2_Atom_Sequence*)info);
	}
	if (client_state == Exit) {
		rtk_osx_api_terminate ();
	}
}

#else

static void
main_loop (void)
{
	struct timespec    timeout;
	LV2_Atom_Sequence* data = (LV2_Atom_Sequence*)malloc (inst->min_atom_bufsiz * sizeof (uint8_t));

	pthread_mutex_lock (&gui_thread_lock);
	while (client_state != Exit) {
		run_one (data);

		if (client_state == Exit)
			break;

#ifdef _WIN32
//Sleep(1000/UI_UPDATE_FPS);
#if (defined(__MINGW64__) || defined(__MINGW32__)) && __MSVCRT_VERSION__ >= 0x0601
		struct __timeb64 timebuffer;
		_ftime64 (&timebuffer);
#else
		struct __timeb32 timebuffer;
		_ftime (&timebuffer);
#endif
		timeout.tv_nsec = timebuffer.millitm * 1000000;
		timeout.tv_sec  = timebuffer.time;
#else // POSIX
		clock_gettime (CLOCK_REALTIME, &timeout);
#endif
		timeout.tv_nsec += 1000000000 / (UI_UPDATE_FPS);
		if (timeout.tv_nsec >= 1000000000) {
			timeout.tv_nsec -= 1000000000;
			timeout.tv_sec += 1;
		}
		pthread_cond_timedwait (&data_ready, &gui_thread_lock, &timeout);

	} /* while running */
	free (data);
	pthread_mutex_unlock (&gui_thread_lock);
}
#endif // APPLE RUNLOOP

static void
catchsig (int sig)
{
	fprintf (stderr, "caught signal - shutting down.\n");
	client_state = Exit;
	pthread_cond_signal (&data_ready);
}

static void
on_external_ui_closed (void* controller)
{
	catchsig (0);
}

#ifdef X42_MULTIPLUGIN
static void
list_plugins (void)
{
	unsigned int i;
	for (i = 0; i < sizeof (_plugins) / sizeof (RtkLv2Description); ++i) {
		const LV2_Descriptor* d = _plugins[i].lv2_descriptor (_plugins[i].dsp_descriptor_id);
		printf (" %2d   \"%s\" %s\n", i, _plugins[i].plugin_human_id, d->URI);
	}
}
#endif

static void
print_usage (void)
{
#ifdef X42_MULTIPLUGIN
	printf ("x42-%s - JACK %s\n\n", APPNAME, X42_MULTIPLUGIN_NAME);
	printf ("Usage: x42-%s [ OPTIONS ] [ Plugin-ID or URI ]\n\n", APPNAME);
	printf ("This is a standalone JACK application of a collection of LV2 plugins.\n"
	        "Use ID -1, -l or --list for a dedicated list of included plugins.\n"
	        "By default the first listed plugin (ID 0) is used.\n\n");
	printf ("List of available plugins: (ID \"Name\" URI)\n");
	list_plugins ();

#else

#if defined X42_PLUGIN_STRUCT
	inst                    = &X42_PLUGIN_STRUCT;
#else
	inst = &_plugin;
#endif
	const LV2_Descriptor* d = inst->lv2_descriptor (inst->dsp_descriptor_id);

	printf ("x42-%s - JACK %s\n\n", APPNAME, inst->plugin_human_id);
	printf ("Usage: x42-%s [ OPTIONS ]\n\n", APPNAME);

	printf ("This is a standalone JACK application of the LV2 plugin:\n"
	        "\"%s\".\n",
	        inst->plugin_human_id);
#endif

	printf ("\nUsage:\n"

"All control elements are operated in using the mouse:\n"
" Click+Drag     left/down: decrease, right/up: increase value. Hold the Ctrl key to increase sensitivity.\n"
" Shift+Click    reset to default value\n"
" Scroll-wheel    up/down by 1 step (smallest possible adjustment for given setting). Rapid continuous scrolling increases the step-size.\n"
"The application can be closed by sending a SIGTERM (CTRL+C) on the command-line of by closing the window.\n"
	);

	printf ("\nOptions:\n"

" -h, --help                Display this help and exit\n"
" -j, --jack-name <name>    Set the JACK client name\n"
"                           (defaults to plugin-name)\n"
#ifndef REQUIRE_UI
" -G, --nogui               run headless, useful for OSC remote ctrl.\n"
#endif
#ifdef X42_MULTIPLUGIN
" -l, --list                Print list of available plugins and exit\n"
#endif
" -O <port>, --osc <port>   Listen for OSC messages on the given UDP port\n"
" -p <idx>:<val>, --port <idx>:<val>\n"
"                           Set initial value for given control port\n"
" -P, --portlist            Print control port list on startup\n"
" --osc-doc                 Print available OSC commands and exit\n"
" -V, --version             Print version information and exit\n"
	);

#ifdef X42_MULTIPLUGIN_URI
	printf ("\nSee also: <%s>\n", X42_MULTIPLUGIN_URI);
#else
	printf ("\nSee also: <%s>\n", d->URI);
#endif
	printf ("Website: <http://x42-plugins.com/>\n");
}

static void
print_version (void)
{
	printf ("x42-%s version %s\n\n", APPNAME, VERSION);
	printf ("\n"
	        "Copyright (C) GPL 2013-2019 Robin Gareus <robin@gareus.org>\n"
	        "This is free software; see the source for copying conditions.  There is NO\n"
	        "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");
}

static void
dump_control_ports (void)
{
	printf ("# Input Control Port List (%d ports)\n", inst->nports_ctrl);
	for (uint32_t i = 0; i < inst->nports_ctrl; ++i) {
		const uint32_t pi = portmap_rctl[i];
		if (inst->ports[pi].porttype != CONTROL_IN) {
			continue;
		}
		printf ("%3d: %s", i, inst->ports[pi].name);

		if (inst->ports[pi].val_min < inst->ports[pi].val_max) {
			printf (" (min: %.2f, max: %.2f)", inst->ports[pi].val_min, inst->ports[pi].val_max);
		}

		if (inst->ports[pi].val_default != nan) {
			printf (" [default: %.2f]", inst->ports[pi].val_default);
		}

		if (inst->ports[pi].doc) {
			printf (" -- %s", inst->ports[pi].doc);
		}
		printf ("\n");
	}
	printf ("### End Port List\n");
}

int
main (int argc, char** argv)
{
	int            c;
	int            rv               = 0;
	int            osc_port         = 0;
	bool           dump_ports       = false;
	bool           headless         = false;
	uint32_t       c_ain            = 0;
	uint32_t       c_aout           = 0;
	uint32_t       c_ctrl           = 0;
	uint32_t       n_pval           = 0;
	struct PValue* pval             = NULL;
	char*          jack_client_name = NULL;

	const struct option long_options[] = {
		{ "help",      no_argument,       0, 'h' },
		{ "jack-name", required_argument, 0, 'j' },
		{ "list",      no_argument,       0, 'l' },
		{ "nogui",     no_argument,       0, 'G' },
		{ "osc",       required_argument, 0, 'O' },
		{ "osc-doc",   no_argument,       0, 0x100 },
		{ "port",      required_argument, 0, 'p' },
		{ "portlist",  no_argument,       0, 'P' },
		{ "version",   no_argument,       0, 'V' },
	};

	const char* optstring = "Ghj:lO:p:PV1";

	if (optind < argc && !strncmp (argv[optind], "-psn_0", 6)) {
		++optind;
	}

	while ((c = getopt_long (argc, argv, optstring, long_options, NULL)) != -1) {
		char* tmp;
		switch (c) {
			case 'h':
				print_usage ();
				return 0;
				break;
			case 'G':
#ifndef REQUIRE_UI
				headless = true;
#endif
				break;
			case 'j':
				jack_client_name = optarg;
				break;
			case '1': // catch -1, backward compat
			case 'l':
#ifdef X42_MULTIPLUGIN
				list_plugins ();
#endif
				return 0;
				break;
			case 'O':
				osc_port = atoi (optarg);
#ifndef HAVE_LIBLO
				fprintf (stderr, "This version was compiled without OSC support.\n");
#endif
				break;
			case 'p':
				if ((tmp = strchr (optarg, ':')) != NULL && *(++tmp)) {
					pval                  = (struct PValue*)realloc (pval, (n_pval + 1) * sizeof (struct PValue));
					pval[n_pval].port_idx = atoi (optarg);
					pval[n_pval].value    = atof (tmp);
					++n_pval;
				}
				break;
			case 'P':
				dump_ports = true;
				break;
			case 'V':
				print_version ();
				return 0;
				break;

			case 0x100:
#ifndef HAVE_LIBLO
				fprintf (stderr, "This version was compiled without OSC support.\n");
#else
				print_oscdoc ();
#endif
				return 0;
				break;
			default:
				// silently ignore additional session options on OSX
#ifndef __APPLE__
				fprintf (stderr, "invalid argument.\n");
				print_usage ();
				return (1);
#endif
				break;
		}
	}

	// TODO autoconnect option.

#ifdef X42_MULTIPLUGIN
	inst = NULL;
	if (optind < argc && atoi (argv[optind]) < 0) {
		list_plugins ();
		return 0;
	}
	if (optind < argc && strlen (argv[optind]) > 2 && atoi (argv[optind]) == 0) {
		unsigned int i;
		for (i = 0; i < sizeof (_plugins) / sizeof (RtkLv2Description); ++i) {
			const LV2_Descriptor* d = _plugins[i].lv2_descriptor (_plugins[i].dsp_descriptor_id);
			if (strstr (d->URI, argv[optind]) || strstr (_plugins[i].plugin_human_id, argv[optind])) {
				inst = &_plugins[i];
				break;
			}
		}
	}
	if (optind < argc && !inst && atoi (argv[optind]) >= 0) {
		unsigned int plugid = atoi (argv[optind]);
		if (plugid < (sizeof (_plugins) / sizeof (RtkLv2Description))) {
			inst = &_plugins[plugid];
		}
	}
	if (!inst) {
		inst = &_plugins[0];
	}
#elif defined X42_PLUGIN_STRUCT
	inst = &X42_PLUGIN_STRUCT;
#else
	inst = &_plugin;
#endif

#ifdef __APPLE__
	rtk_osx_api_init ();
#endif

#ifdef USE_WEAK_JACK
	if (have_libjack ()) {
		fprintf (stderr, "JACK is not available. http://jackaudio.org/\n");
#ifdef _WIN32
		MessageBox (NULL,
		            TEXT (
		                  "JACK is not available.\n"
		                  "You must have the JACK Audio Connection Kit installed to use the tools. "
		                  "Please see http://jackaudio.org/ and http://jackaudio.org/faq/jack_on_windows.html"),
		            TEXT ("Error"), MB_ICONERROR | MB_OK);
#elif __APPLE__
		rtk_osx_api_err (
		    "JACK is not available.\n"
		    "You must have the JACK Audio Connection Kit installed to use the tools. "
		    "Please see http://jackaudio.org/");
#endif
		return 1;
	}
#endif

#ifdef _WIN32
	pthread_win32_process_attach_np ();
#endif
#if (defined _WIN32 && defined RTK_STATIC_INIT)
	glib_init_static ();
	gobject_init_ctor ();
#endif

	LV2_URID_Map      uri_map       = { NULL, &uri_to_id };
	const LV2_Feature map_feature   = { LV2_URID__map, &uri_map };
	const LV2_Feature unmap_feature = { LV2_URID__unmap, NULL };

	LV2_Worker_Schedule schedule         = { NULL, lv2_worker_schedule };
	const LV2_Feature   schedule_feature = { LV2_WORKER__schedule, &schedule };

	const LV2_Feature* features[] = {
		&map_feature, &unmap_feature, &schedule_feature, NULL
	};

	const LV2_Feature external_lv_feature = { LV2_EXTERNAL_UI_URI, &extui_host };
	const LV2_Feature external_kx_feature = { LV2_EXTERNAL_UI_URI__KX__Host, &extui_host };
	LV2_Feature       instance_feature    = { "http://lv2plug.in/ns/ext/instance-access", NULL };

	const LV2_Feature* ui_features[] = {
		&map_feature, &unmap_feature,
		&instance_feature,
		&external_lv_feature,
		&external_kx_feature,
		NULL
	};

	/* check sourced settings */
	assert ((inst->nports_midi_in + inst->nports_atom_in) <= 1);
	assert ((inst->nports_midi_out + inst->nports_atom_out) <= 1);
	assert (inst->plugin_human_id);
	assert (inst->nports_total > 0);

	extui_host.plugin_human_id = inst->plugin_human_id;

	// TODO check if allocs succeeded - OOM -> exit
	/* allocate data structure */
	portmap_a_in  = (uint32_t*)malloc (inst->nports_audio_in * sizeof (uint32_t));
	portmap_a_out = (uint32_t*)malloc (inst->nports_audio_out * sizeof (uint32_t));
	portmap_rctl  = (uint32_t*)malloc (inst->nports_ctrl * sizeof (uint32_t));
	portmap_ctrl  = (uint32_t*)malloc (inst->nports_total * sizeof (uint32_t));

	plugin_ports_pre  = (float*)calloc (inst->nports_ctrl, sizeof (float));
	plugin_ports_post = (float*)calloc (inst->nports_ctrl, sizeof (float));

	atom_in  = (LV2_Atom_Sequence*)malloc (inst->min_atom_bufsiz + sizeof (uint8_t));
	atom_out = (LV2_Atom_Sequence*)malloc (inst->min_atom_bufsiz + sizeof (uint8_t));

	rb_ctrl_to_ui   = jack_ringbuffer_create ((UPDATE_FREQ_RATIO)*inst->nports_ctrl * 2 * sizeof (float));
	rb_ctrl_from_ui = jack_ringbuffer_create ((UPDATE_FREQ_RATIO)*inst->nports_ctrl * 2 * sizeof (float));

	rb_atom_to_ui   = jack_ringbuffer_create ((UPDATE_FREQ_RATIO)*inst->min_atom_bufsiz);
	rb_atom_from_ui = jack_ringbuffer_create ((UPDATE_FREQ_RATIO)*inst->min_atom_bufsiz);

#ifdef HAVE_LIBLO
	rb_osc_to_ui = jack_ringbuffer_create ((UPDATE_FREQ_RATIO)*inst->nports_ctrl * 2 * sizeof (float));
#endif

	/* resolve descriptors */
	plugin_dsp = inst->lv2_descriptor (inst->dsp_descriptor_id);
	plugin_gui = inst->lv2ui_descriptor (inst->gui_descriptor_id);

	if (!plugin_dsp) {
		fprintf (stderr, "cannot resolve LV2 descriptor\n");
		rv |= 2;
		goto out;
	}
	/* jack-open -> samlerate */
	if (init_jack (jack_client_name ? jack_client_name : extui_host.plugin_human_id)) {
		fprintf (stderr, "cannot connect to JACK.\n");
#ifdef _WIN32
		MessageBox (NULL, TEXT (
		                      "Cannot connect to JACK.\n"
		                      "Please start the JACK Server first."),
		            TEXT ("Error"), MB_ICONERROR | MB_OK);
#elif __APPLE__
		rtk_osx_api_err (
		    "Cannot connect to JACK.\n"
		    "Please start the JACK Server first.");
#else
		(void)system ("xmessage -button ok -center \"Cannot connect to JACK.\nPlease start the JACK Server first.\" &");
#endif
		rv |= 4;
		goto out;
	}

	/* init plugin */
	plugin_instance = plugin_dsp->instantiate (plugin_dsp, j_samplerate, NULL, features);
	if (!plugin_instance) {
		fprintf (stderr, "instantiation failed\n");
		rv |= 2;
		goto out;
	}

	/* connect ports */
	for (uint32_t p = 0; p < inst->nports_total; ++p) {
		portmap_ctrl[p] = UINT32_MAX;
		switch (inst->ports[p].porttype) {
			case CONTROL_IN:
				plugin_ports_pre[c_ctrl] = inst->ports[p].val_default;
			case CONTROL_OUT:
				portmap_ctrl[p]      = c_ctrl;
				portmap_rctl[c_ctrl] = p;
				plugin_dsp->connect_port (plugin_instance, p, &plugin_ports_pre[c_ctrl++]);
				break;
			case AUDIO_IN:
				portmap_a_in[c_ain++] = p;
				break;
			case AUDIO_OUT:
				portmap_a_out[c_aout++] = p;
				break;
			case MIDI_IN:
			case ATOM_IN:
				portmap_atom_from_ui = p;
				plugin_dsp->connect_port (plugin_instance, p, atom_in);
				break;
			case MIDI_OUT:
			case ATOM_OUT:
				portmap_atom_to_ui = p;
				plugin_dsp->connect_port (plugin_instance, p, atom_out);
				break;
			default:
				fprintf (stderr, "yet unsupported port..\n");
				break;
		}
	}

	assert (c_ain == inst->nports_audio_in);
	assert (c_aout == inst->nports_audio_out);
	assert (c_ctrl == inst->nports_ctrl);

	if (inst->nports_atom_out > 0 || inst->nports_atom_in > 0 || inst->nports_midi_in > 0 || inst->nports_midi_out > 0) {
		uri_atom_Sequence       = uri_to_id (NULL, LV2_ATOM__Sequence);
		uri_atom_EventTransfer  = uri_to_id (NULL, LV2_ATOM__eventTransfer);
		uri_midi_MidiEvent      = uri_to_id (NULL, LV2_MIDI__MidiEvent);
		uri_time_Position       = uri_to_id (NULL, LV2_TIME__Position);
		uri_time_frame          = uri_to_id (NULL, LV2_TIME__frame);
		uri_time_speed          = uri_to_id (NULL, LV2_TIME__speed);
		uri_time_bar            = uri_to_id (NULL, LV2_TIME__bar);
		uri_time_barBeat        = uri_to_id (NULL, LV2_TIME__barBeat);
		uri_time_beatUnit       = uri_to_id (NULL, LV2_TIME__beatUnit);
		uri_time_beatsPerBar    = uri_to_id (NULL, LV2_TIME__beatsPerBar);
		uri_time_beatsPerMinute = uri_to_id (NULL, LV2_TIME__beatsPerMinute);
		lv2_atom_forge_init (&lv2_forge, &uri_map);
	}

	if (dump_ports) {
		dump_control_ports ();
	}

	// apply user settings
	for (uint32_t i = 0; i < n_pval; ++i) {
		if (pval[i].port_idx >= inst->nports_ctrl) {
			fprintf (stderr, "Invalid Parameter 0 <= %d < %d\n", pval[i].port_idx, inst->nports_ctrl);
			continue;
		}

		const uint32_t port_index = portmap_rctl[pval[i].port_idx];

		if (inst->ports[port_index].porttype != CONTROL_IN) {
			fprintf (stderr, "mapped port (%d) for port %d is not a control input.\n", port_index, pval[i].port_idx);
			continue;
		}

		if (inst->ports[port_index].val_min < inst->ports[port_index].val_max) {
			if (pval[i].value < inst->ports[port_index].val_min || pval[i].value > inst->ports[port_index].val_max) {
				fprintf (stderr, "Value for port %d out of bounds %f <= %f <= %f\n",
				         pval[i].port_idx,
				         inst->ports[port_index].val_min, pval[i].value, inst->ports[port_index].val_max);
				continue;
			}
		}

		//fprintf (stdout, "Port: %d,  %d -> %f\n", pval[i].port_idx, port_index, pval[i].value);
		plugin_ports_pre[pval[i].port_idx] = pval[i].value;
	}

	if (jack_portsetup ()) {
		rv |= 12;
		goto out;
	}

	if (plugin_gui && !headless) {
		/* init plugin GUI */
		extui_host.ui_closed  = on_external_ui_closed;
		instance_feature.data = plugin_instance;
		gui_instance          = plugin_gui->instantiate (plugin_gui,
		                                        plugin_dsp->URI, NULL,
		                                        &write_function, controller,
		                                        (void**)&extui, ui_features);
	}

	if (!gui_instance || !extui) {
#ifdef REQUIRE_UI
		fprintf (stderr, "Error: GUI initialization failed.\n");
		rv |= 2;
		goto out;
#else
		if (!headless) {
			fprintf (stderr, "Warning: GUI initialization failed.\n");
		}
#endif
	}

#ifndef _WIN32
	if (mlockall (MCL_CURRENT | MCL_FUTURE)) {
		fprintf (stderr, "Warning: Can not lock memory.\n");
	}
#endif

	if (gui_instance) {
		for (uint32_t p = 0; p < inst->nports_ctrl; p++) {
			if (jack_ringbuffer_write_space (rb_ctrl_to_ui) >= sizeof (uint32_t) + sizeof (float)) {
				jack_ringbuffer_write (rb_ctrl_to_ui, (char*)&portmap_rctl[p], sizeof (uint32_t));
				jack_ringbuffer_write (rb_ctrl_to_ui, (char*)&plugin_ports_pre[p], sizeof (float));
			}
		}
	}

	if (plugin_dsp->extension_data) {
		worker_iface = (LV2_Worker_Interface*)plugin_dsp->extension_data (LV2_WORKER__interface);
	}
	if (worker_iface) {
		worker_init ();
	}

	if (plugin_dsp->activate) {
		plugin_dsp->activate (plugin_instance);
	}

	if (jack_activate (j_client)) {
		fprintf (stderr, "cannot activate client.\n");
		rv |= 20;
		goto out;
	}

	jack_portconnect (JACK_AUTOCONNECT);

#ifdef HAVE_LIBLO
	if (osc_port > 0) {
		start_osc_server (osc_port);
	}
#endif

#ifndef _WIN32
	signal (SIGHUP, catchsig);
	signal (SIGINT, catchsig);
#endif

	if (!gui_instance || !extui) {
		/* no GUI */
		while (client_state != Exit) {
			sleep (1);
		}
	} else {
		LV2_EXTERNAL_UI_SHOW (extui);

#ifdef __APPLE__
		LV2_Atom_Sequence*    data    = (LV2_Atom_Sequence*)malloc (inst->min_atom_bufsiz * sizeof (uint8_t));
		CFRunLoopRef          runLoop = CFRunLoopGetCurrent ();
		CFRunLoopTimerContext context = { 0, data, NULL, NULL, NULL };
		CFRunLoopTimerRef     timer   = CFRunLoopTimerCreate (kCFAllocatorDefault, 0, 1.0 / UI_UPDATE_FPS, 0, 0, &osx_loop, &context);
		CFRunLoopAddTimer (runLoop, timer, kCFRunLoopCommonModes);
		rtk_osx_api_run ();
		free (data);
#else

		main_loop ();
#endif

		LV2_EXTERNAL_UI_HIDE (extui);
	}

out:
	free (pval);
	cleanup (0);
#ifdef _WIN32
	pthread_win32_process_detach_np ();
#endif
#if (defined _WIN32 && defined RTK_STATIC_INIT)
	glib_cleanup_static ();
#endif
	return (rv);
}
