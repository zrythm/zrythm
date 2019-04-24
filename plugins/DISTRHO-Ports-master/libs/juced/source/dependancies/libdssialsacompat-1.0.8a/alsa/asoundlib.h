/* DSSI ALSA compatibility library
 *
 * This library provides for Mac OS X the ALSA snd_seq_event_t handling
 * necessary to compile and run DSSI.  It was extracted from alsa-lib 1.0.8.
 */

#include <stddef.h>

#include <alsa/asoundef.h>
#include <alsa/seq_event.h>
#include <alsa/seq_midi_event.h>

