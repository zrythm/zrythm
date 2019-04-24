#ifndef _MIDIFILTER_H_
#define _MIDIFILTER_H_

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/time/time.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"

#define MFP_URI "http://gareus.org/oss/lv2/midifilter"

#ifndef MIN
#define MIN(a,b) ( (a) < (b) ? (a) : (b) )
#endif

#ifndef MAX
#define MAX(a,b) ( (a) > (b) ? (a) : (b) )
#endif

#ifndef RAIL
#define RAIL(v, min, max) (MIN((max), MAX((min), (v))))
#endif

#ifndef SQUARE
#define SQUARE(a) ( (a) * (a) )
#endif

#ifndef ABSDIFF
#define ABSDIFF(a,b) ( (a) > (b) ? ((a) - (b)) : ((b) - (a)) )
#endif

#define ROUND_PARTIAL_BEATS(X,Y) ( rint( (Y) * (X)) / (Y) - floor(rint( (Y) * (X)) / (Y)) )

/* /monotonic/sample count */
#define MSC_MAX (1<<29)
#define MSC_DIFF(NOW, THEN) ( ABSDIFF(NOW, THEN) < (MSC_MAX>>1) \
		? ( (NOW) - (THEN) ) \
		: ( (NOW) > (THEN) ? ((NOW) - (THEN) - MSC_MAX) : (MSC_MAX - (THEN) + (NOW)) ) \
		)

#define MAXCFG 16

#define LOOP_CFG(FN) \
	FN(0)  FN(1)  FN(2)  FN(3) \
	FN(4)  FN(5)  FN(6)  FN(7) \
	FN(8)  FN(9)  FN(10) FN(11) \
	FN(12) FN(13) FN(14) FN(15) \

typedef struct {
	LV2_URID atom_Blank;
	LV2_URID atom_Object;
	LV2_URID midi_MidiEvent;
	LV2_URID atom_Sequence;
	LV2_URID atom_Float;
	LV2_URID atom_Long;
	LV2_URID time_Position;
	LV2_URID time_barBeat;
	LV2_URID time_beatsPerMinute;
	LV2_URID time_speed;
	LV2_URID time_frame;
	LV2_URID time_fps;
} MidiFilterURIs;


typedef struct {
	uint8_t buf[3];
	int size;
	int reltime;
} MidiEventQueue;

enum {
	NFO_BPM = 1,
	NFO_SPEED = 2,
	NFO_BEAT = 4,
	NFO_FRAME = 8,
	NFO_FPS = 16
};

typedef struct _MidiFilter{
	LV2_Atom_Forge forge;
	LV2_Atom_Forge_Frame frame;

	LV2_URID_Map* map;
	MidiFilterURIs uris;

	const LV2_Atom_Sequence* midiin;
	LV2_Atom_Sequence* midiout;
	float* latency_port;

	float  latency;

	float *cfg[MAXCFG];
	float lcfg[MAXCFG];

	float   memF[16];
	int     memI[127];
	int     memCI[16][256];
	short   memCS[16][127];
	uint8_t memCM[16][127];

	int    available_info; // bit-f
	float  bpm;
	float  speed;  // Transport speed (usually 0=stop, 1=play)
	double   bar_beats;
	double   beat_beats;
	uint32_t pos_bbt;
	int64_t  pos_frame;
	float    frames_per_second;

	MidiEventQueue *memQ;
	MidiEventQueue *memS;

	uint32_t n_samples;
	double samplerate;

	void (*filter_fn) (struct _MidiFilter*, uint32_t, const uint8_t* const, uint32_t);
	void (*preproc_fn)  (struct _MidiFilter*);
	void (*postproc_fn) (struct _MidiFilter*);
	void (*cleanup_fn)  (struct _MidiFilter*);
} MidiFilter;

void forge_midimessage(MidiFilter* self,
		uint32_t tme,
		const uint8_t* const buffer,
		uint32_t size);

#define MF_DESCRIPTOR(ID, URLSUFFIX) \
	static const LV2_Descriptor descriptor ## ID = { \
		MFP_URI "#" URLSUFFIX, \
		instantiate, \
		connect_port, \
		NULL, \
		run, \
		NULL, \
		cleanup, \
		extension_data \
};

/** return a valid midi-value, limit to range 0..127 */
static int midi_limit_val(const int);

/** return a valid midi-channel, limit to range 0..15 */
static int midi_limit_chn(const int);

/** return non-zero (true) if value is in 0..127 */
static int midi_valid(const int);

/** return 14bit integer value of buffer[2], buffer[1]
 * e.g. midi pitchbend */
static int midi_14bit(const uint8_t * const);

static int midi_is_panic(const uint8_t * const b, const int size);

/** calculate a random number according to a modified Marsaglia polar method
 * approaching a normal distribution, should be more "human"
 *
 * @param dev standard deviation
 * @return unbound value with average 0.0 (most values (~67%) will be within -dev .. dev, but range is -inf .. inf)
 */
static float normrand(const float dev);

#define MIDI_NOTEOFF         0x80
#define MIDI_NOTEON          0x90
#define MIDI_POLYKEYPRESSURE 0xA0
#define MIDI_CONTROLCHANGE   0xB0
#define MIDI_PROGRAMCHANGE   0xC0
#define MIDI_CHANNELPRESSURE 0xD0
#define MIDI_PITCHBEND       0xE0

#define MIDI_SYSEX           0xF0
#define MIDI_QUARTERFRAME    0xF1
#define MIDI_SONGPOSITION    0xF2
#define MIDI_SONGSELECT      0xF3
#define MIDI_F4              0xF4
#define MIDI_F5              0xF5
#define MIDI_TUNEREQUEST     0xF6
#define MIDI_EOX             0xF7
#define MIDI_TIMINGCLOCK     0xF8
#define MIDI_F9              0xF9
#define MIDI_START           0xFA
#define MIDI_CONTINUE        0xFB
#define MIDI_STOP            0xFC
#define MIDI_FD              0xFD
#define MIDI_ACTIVESENSING   0xFE
#define MIDI_SYSTEMRESET     0xFF

#endif
