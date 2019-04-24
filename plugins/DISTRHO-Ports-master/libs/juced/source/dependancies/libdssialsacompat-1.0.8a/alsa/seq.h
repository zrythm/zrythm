/* DSSI ALSA compatibility library
 *
 * This library provides for Mac OS X the ALSA snd_seq_event_t handling
 * necessary to compile and run DSSI.  It was extracted from alsa-lib 1.0.8.
 */

/**
 * \file <alsa/seq.h>
 * \brief Application interface library for the ALSA driver
 * \author Jaroslav Kysela <perex@suse.cz>
 * \author Abramo Bagnara <abramo@alsa-project.org>
 * \author Takashi Iwai <tiwai@suse.de>
 * \date 1998-2001
 */
/*
 * Application interface library for the ALSA driver
 *
 *
 *   This library is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as
 *   published by the Free Software Foundation; either version 2.1 of
 *   the License, or (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#ifndef __ALSA_SEQ_H
#define __ALSA_SEQ_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 *  \defgroup SeqEvType Sequencer Event Type Checks
 *  Sequencer Event Type Checks
 *  \ingroup Sequencer
 *  \{
 */

/* event type macros */
enum {
	SND_SEQ_EVFLG_RESULT,
	SND_SEQ_EVFLG_NOTE,
	SND_SEQ_EVFLG_CONTROL,
	SND_SEQ_EVFLG_QUEUE,
	SND_SEQ_EVFLG_SYSTEM,
	SND_SEQ_EVFLG_MESSAGE,
	SND_SEQ_EVFLG_CONNECTION,
	SND_SEQ_EVFLG_SAMPLE,
	SND_SEQ_EVFLG_USERS,
	SND_SEQ_EVFLG_INSTR,
	SND_SEQ_EVFLG_QUOTE,
	SND_SEQ_EVFLG_NONE,
	SND_SEQ_EVFLG_RAW,
	SND_SEQ_EVFLG_FIXED,
	SND_SEQ_EVFLG_VARIABLE,
	SND_SEQ_EVFLG_VARUSR
};

enum {
	SND_SEQ_EVFLG_NOTE_ONEARG,
	SND_SEQ_EVFLG_NOTE_TWOARG
};

enum {
	SND_SEQ_EVFLG_QUEUE_NOARG,
	SND_SEQ_EVFLG_QUEUE_TICK,
	SND_SEQ_EVFLG_QUEUE_TIME,
	SND_SEQ_EVFLG_QUEUE_VALUE
};

/**
 * Exported event type table
 *
 * This table is referred by snd_seq_ev_is_xxx.
 */
extern const unsigned int snd_seq_event_types[];

#define _SND_SEQ_TYPE(x)	(1<<(x))	/**< master type - 24bit */
#define _SND_SEQ_TYPE_OPT(x)	((x)<<24)	/**< optional type - 8bit */

/** check the event type */
#define snd_seq_type_check(ev,x) (snd_seq_event_types[(ev)->type] & _SND_SEQ_TYPE(x))

/** event type check: result events */
#define snd_seq_ev_is_result_type(ev) \
	snd_seq_type_check(ev, SND_SEQ_EVFLG_RESULT)
/** event type check: note events */
#define snd_seq_ev_is_note_type(ev) \
	snd_seq_type_check(ev, SND_SEQ_EVFLG_NOTE)
/** event type check: control events */
#define snd_seq_ev_is_control_type(ev) \
	snd_seq_type_check(ev, SND_SEQ_EVFLG_CONTROL)
/** event type check: channel specific events */
#define snd_seq_ev_is_channel_type(ev) \
	(snd_seq_event_types[(ev)->type] & (_SND_SEQ_TYPE(SND_SEQ_EVFLG_NOTE) | _SND_SEQ_TYPE(SND_SEQ_EVFLG_CONTROL)))

/** event type check: queue control events */
#define snd_seq_ev_is_queue_type(ev) \
	snd_seq_type_check(ev, SND_SEQ_EVFLG_QUEUE)
/** event type check: system status messages */
#define snd_seq_ev_is_message_type(ev) \
	snd_seq_type_check(ev, SND_SEQ_EVFLG_MESSAGE)
/** event type check: system status messages */
#define snd_seq_ev_is_subscribe_type(ev) \
	snd_seq_type_check(ev, SND_SEQ_EVFLG_CONNECTION)
/** event type check: sample messages */
#define snd_seq_ev_is_sample_type(ev) \
	snd_seq_type_check(ev, SND_SEQ_EVFLG_SAMPLE)
/** event type check: user-defined messages */
#define snd_seq_ev_is_user_type(ev) \
	snd_seq_type_check(ev, SND_SEQ_EVFLG_USERS)
/** event type check: instrument layer events */
#define snd_seq_ev_is_instr_type(ev) \
	snd_seq_type_check(ev, SND_SEQ_EVFLG_INSTR)
/** event type check: fixed length events */
#define snd_seq_ev_is_fixed_type(ev) \
	snd_seq_type_check(ev, SND_SEQ_EVFLG_FIXED)
/** event type check: variable length events */
#define snd_seq_ev_is_variable_type(ev)	\
	snd_seq_type_check(ev, SND_SEQ_EVFLG_VARIABLE)
/** event type check: user pointer events */
#define snd_seq_ev_is_varusr_type(ev) \
	snd_seq_type_check(ev, SND_SEQ_EVFLG_VARUSR)
/** event type check: reserved for kernel */
#define snd_seq_ev_is_reserved(ev) \
	(! snd_seq_event_types[(ev)->type])

/**
 * macros to check event flags
 */
/** prior events */
#define snd_seq_ev_is_prior(ev)	\
	(((ev)->flags & SND_SEQ_PRIORITY_MASK) == SND_SEQ_PRIORITY_HIGH)

/** get the data length type */
#define snd_seq_ev_length_type(ev) \
	((ev)->flags & SND_SEQ_EVENT_LENGTH_MASK)
/** fixed length events */
#define snd_seq_ev_is_fixed(ev)	\
	(snd_seq_ev_length_type(ev) == SND_SEQ_EVENT_LENGTH_FIXED)
/** variable length events */
#define snd_seq_ev_is_variable(ev) \
	(snd_seq_ev_length_type(ev) == SND_SEQ_EVENT_LENGTH_VARIABLE)
/** variable length on user-space */
#define snd_seq_ev_is_varusr(ev) \
	(snd_seq_ev_length_type(ev) == SND_SEQ_EVENT_LENGTH_VARUSR)

/** time-stamp type */
#define snd_seq_ev_timestamp_type(ev) \
	((ev)->flags & SND_SEQ_TIME_STAMP_MASK)
/** event is in tick time */
#define snd_seq_ev_is_tick(ev) \
	(snd_seq_ev_timestamp_type(ev) == SND_SEQ_TIME_STAMP_TICK)
/** event is in real-time */
#define snd_seq_ev_is_real(ev) \
	(snd_seq_ev_timestamp_type(ev) == SND_SEQ_TIME_STAMP_REAL)

/** time-mode type */
#define snd_seq_ev_timemode_type(ev) \
	((ev)->flags & SND_SEQ_TIME_MODE_MASK)
/** scheduled in absolute time */
#define snd_seq_ev_is_abstime(ev) \
	(snd_seq_ev_timemode_type(ev) == SND_SEQ_TIME_MODE_ABS)
/** scheduled in relative time */
#define snd_seq_ev_is_reltime(ev) \
	(snd_seq_ev_timemode_type(ev) == SND_SEQ_TIME_MODE_REL)

/** direct dispatched events */
#define snd_seq_ev_is_direct(ev) \
	((ev)->queue == SND_SEQ_QUEUE_DIRECT)

/** \} */

#ifdef __cplusplus
}
#endif

#endif /* __ALSA_SEQ_H */

