/* DSSI ALSA compatibility library
 *
 * This library provides for non-ALSA systems the ALSA snd_seq_event_t handling
 * necessary to compile and run DSSI.  It was extracted from alsa-lib 1.0.8.
 */

#include <stdlib.h>
#include <errno.h>

#include <alsa/asoundef.h>
#include <alsa/sound/asequencer.h>
#include <alsa/seq.h>
#include <alsa/seq_event.h>
#include <alsa/seq_midi_event.h>

/**
 * \file seq/seq_event.c
 * \brief Sequencer Event Types
 * \author Takashi Iwai <tiwai@suse.de>
 * \date 2001
 */

#define FIXED_EV(x)	(_SND_SEQ_TYPE(SND_SEQ_EVFLG_FIXED) | _SND_SEQ_TYPE(x))

/** Event types conversion array */
const unsigned int snd_seq_event_types[256] = {
	[SND_SEQ_EVENT_SYSTEM ... SND_SEQ_EVENT_RESULT]
	= FIXED_EV(SND_SEQ_EVFLG_RESULT),
	[SND_SEQ_EVENT_NOTE]
	= FIXED_EV(SND_SEQ_EVFLG_NOTE) | _SND_SEQ_TYPE_OPT(SND_SEQ_EVFLG_NOTE_TWOARG),
	[SND_SEQ_EVENT_NOTEON ... SND_SEQ_EVENT_KEYPRESS]
	= FIXED_EV(SND_SEQ_EVFLG_NOTE),
	[SND_SEQ_EVENT_CONTROLLER ... SND_SEQ_EVENT_REGPARAM]
	= FIXED_EV(SND_SEQ_EVFLG_CONTROL),
	[SND_SEQ_EVENT_START ... SND_SEQ_EVENT_STOP]
	= FIXED_EV(SND_SEQ_EVFLG_QUEUE),
	[SND_SEQ_EVENT_SETPOS_TICK]
	= FIXED_EV(SND_SEQ_EVFLG_QUEUE) | _SND_SEQ_TYPE_OPT(SND_SEQ_EVFLG_QUEUE_TICK),
	[SND_SEQ_EVENT_SETPOS_TIME]
	= FIXED_EV(SND_SEQ_EVFLG_QUEUE) | _SND_SEQ_TYPE_OPT(SND_SEQ_EVFLG_QUEUE_TIME),
	[SND_SEQ_EVENT_TEMPO ... SND_SEQ_EVENT_SYNC_POS]
	= FIXED_EV(SND_SEQ_EVFLG_QUEUE) | _SND_SEQ_TYPE_OPT(SND_SEQ_EVFLG_QUEUE_VALUE),
	[SND_SEQ_EVENT_TUNE_REQUEST ... SND_SEQ_EVENT_SENSING]
	= FIXED_EV(SND_SEQ_EVFLG_NONE),
	[SND_SEQ_EVENT_ECHO ... SND_SEQ_EVENT_OSS]
	= FIXED_EV(SND_SEQ_EVFLG_RAW) | FIXED_EV(SND_SEQ_EVFLG_SYSTEM),
	[SND_SEQ_EVENT_CLIENT_START ... SND_SEQ_EVENT_PORT_CHANGE]
	= FIXED_EV(SND_SEQ_EVFLG_MESSAGE),
	[SND_SEQ_EVENT_PORT_SUBSCRIBED ... SND_SEQ_EVENT_PORT_UNSUBSCRIBED]
	= FIXED_EV(SND_SEQ_EVFLG_CONNECTION),
	[SND_SEQ_EVENT_SAMPLE ... SND_SEQ_EVENT_SAMPLE_PRIVATE1]
	= FIXED_EV(SND_SEQ_EVFLG_SAMPLE),
	[SND_SEQ_EVENT_USR0 ... SND_SEQ_EVENT_USR9]
	= FIXED_EV(SND_SEQ_EVFLG_RAW) | FIXED_EV(SND_SEQ_EVFLG_USERS),
	[SND_SEQ_EVENT_INSTR_BEGIN ... SND_SEQ_EVENT_INSTR_CHANGE]
	= _SND_SEQ_TYPE(SND_SEQ_EVFLG_INSTR) | _SND_SEQ_TYPE(SND_SEQ_EVFLG_VARUSR),
	[SND_SEQ_EVENT_SYSEX ... SND_SEQ_EVENT_BOUNCE]
	= _SND_SEQ_TYPE(SND_SEQ_EVFLG_VARIABLE),
	[SND_SEQ_EVENT_USR_VAR0 ... SND_SEQ_EVENT_USR_VAR4]
	= _SND_SEQ_TYPE(SND_SEQ_EVFLG_VARIABLE) | _SND_SEQ_TYPE(SND_SEQ_EVFLG_USERS),
	[SND_SEQ_EVENT_NONE]
	= FIXED_EV(SND_SEQ_EVFLG_NONE),
};

/**
 * \file seq/seq_midi_event.c
 * \brief MIDI byte <-> sequencer event coder
 * \author Takashi Iwai <tiwai@suse.de>
 * \author Jaroslav Kysela <perex@suse.cz>
 * \date 2000-2001
 */

/*
 *  MIDI byte <-> sequencer event coder
 *
 *  Copyright (C) 1998,99,2000 Takashi Iwai <tiwai@suse.de>,
 *			       Jaroslav Kysela <perex@suse.cz>
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
 */

/* midi status */
struct snd_midi_event {
	size_t qlen;	/* queue length */
	size_t read;	/* chars read */
	int type;	/* current event type */
	unsigned char lastcmd;
	unsigned char nostat;
	size_t bufsize;
	unsigned char *buf;	/* input buffer */
};


/* queue type */
/* from 0 to 7 are normal commands (note off, on, etc.) */
#define ST_NOTEOFF	0
#define ST_NOTEON	1
#define ST_SPECIAL	8
#define ST_SYSEX	ST_SPECIAL
/* from 8 to 15 are events for 0xf0-0xf7 */


/* status event types */
typedef void (*event_encode_t)(snd_midi_event_t *dev, snd_seq_event_t *ev);
typedef void (*event_decode_t)(const snd_seq_event_t *ev, unsigned char *buf);

/*
 * prototypes
 */
static void note_event(snd_midi_event_t *dev, snd_seq_event_t *ev);
static void one_param_ctrl_event(snd_midi_event_t *dev, snd_seq_event_t *ev);
static void pitchbend_ctrl_event(snd_midi_event_t *dev, snd_seq_event_t *ev);
static void two_param_ctrl_event(snd_midi_event_t *dev, snd_seq_event_t *ev);
static void one_param_event(snd_midi_event_t *dev, snd_seq_event_t *ev);
static void songpos_event(snd_midi_event_t *dev, snd_seq_event_t *ev);
static void note_decode(const snd_seq_event_t *ev, unsigned char *buf);
static void one_param_decode(const snd_seq_event_t *ev, unsigned char *buf);
static void pitchbend_decode(const snd_seq_event_t *ev, unsigned char *buf);
static void two_param_decode(const snd_seq_event_t *ev, unsigned char *buf);
static void songpos_decode(const snd_seq_event_t *ev, unsigned char *buf);

/*
 * event list
 */
static struct status_event_list_t {
	int event;
	int qlen;
	event_encode_t encode;
	event_decode_t decode;
} status_event[] = {
	/* 0x80 - 0xf0 */
	{SND_SEQ_EVENT_NOTEOFF,		2, note_event, note_decode},
	{SND_SEQ_EVENT_NOTEON,		2, note_event, note_decode},
	{SND_SEQ_EVENT_KEYPRESS,	2, note_event, note_decode},
	{SND_SEQ_EVENT_CONTROLLER,	2, two_param_ctrl_event, two_param_decode},
	{SND_SEQ_EVENT_PGMCHANGE,	1, one_param_ctrl_event, one_param_decode},
	{SND_SEQ_EVENT_CHANPRESS,	1, one_param_ctrl_event, one_param_decode},
	{SND_SEQ_EVENT_PITCHBEND,	2, pitchbend_ctrl_event, pitchbend_decode},
	{SND_SEQ_EVENT_NONE,		0, NULL, NULL}, /* 0xf0 */
	/* 0xf0 - 0xff */
	{SND_SEQ_EVENT_SYSEX,		1, NULL, NULL}, /* sysex: 0xf0 */
	{SND_SEQ_EVENT_QFRAME,		1, one_param_event, one_param_decode}, /* 0xf1 */
	{SND_SEQ_EVENT_SONGPOS,		2, songpos_event, songpos_decode}, /* 0xf2 */
	{SND_SEQ_EVENT_SONGSEL,		1, one_param_event, one_param_decode}, /* 0xf3 */
	{SND_SEQ_EVENT_NONE,		0, NULL, NULL}, /* 0xf4 */
	{SND_SEQ_EVENT_NONE,		0, NULL, NULL}, /* 0xf5 */
	{SND_SEQ_EVENT_TUNE_REQUEST,	0, NULL, NULL},	/* 0xf6 */
	{SND_SEQ_EVENT_NONE,		0, NULL, NULL}, /* 0xf7 */
	{SND_SEQ_EVENT_CLOCK,		0, NULL, NULL}, /* 0xf8 */
	{SND_SEQ_EVENT_NONE,		0, NULL, NULL}, /* 0xf9 */
	{SND_SEQ_EVENT_START,		0, NULL, NULL}, /* 0xfa */
	{SND_SEQ_EVENT_CONTINUE,	0, NULL, NULL}, /* 0xfb */
	{SND_SEQ_EVENT_STOP, 		0, NULL, NULL}, /* 0xfc */
	{SND_SEQ_EVENT_NONE, 		0, NULL, NULL}, /* 0xfd */
	{SND_SEQ_EVENT_SENSING, 	0, NULL, NULL}, /* 0xfe */
	{SND_SEQ_EVENT_RESET, 		0, NULL, NULL}, /* 0xff */
};

#ifdef UNNEEDED_BY_DSSI
static int extra_decode_ctrl14(snd_midi_event_t *dev, unsigned char *buf, int len, const snd_seq_event_t *ev);
static int extra_decode_xrpn(snd_midi_event_t *dev, unsigned char *buf, int count, const snd_seq_event_t *ev);

static struct extra_event_list_t {
	int event;
	int (*decode)(snd_midi_event_t *dev, unsigned char *buf, int len, const snd_seq_event_t *ev);
} extra_event[] = {
	{SND_SEQ_EVENT_CONTROL14, extra_decode_ctrl14},
	{SND_SEQ_EVENT_NONREGPARAM, extra_decode_xrpn},
	{SND_SEQ_EVENT_REGPARAM, extra_decode_xrpn},
};

#define numberof(ary)	(sizeof(ary)/sizeof(ary[0]))
#endif /* UNNEEDED_BY_DSSI */

/**
 * \brief Initialize MIDI event parser
 * \param bufsize buffer size for MIDI message
 * \param rdev allocated MIDI event parser
 * \return 0 on success otherwise a negative error code
 *
 * Allocates and initializes MIDI event parser.
 */
int snd_midi_event_new(size_t bufsize, snd_midi_event_t **rdev)
{
	snd_midi_event_t *dev;

	*rdev = NULL;
	dev = (snd_midi_event_t *)calloc(1, sizeof(snd_midi_event_t));
	if (dev == NULL)
		return -ENOMEM;
	if (bufsize > 0) {
		dev->buf = malloc(bufsize);
		if (dev->buf == NULL) {
			free(dev);
			return -ENOMEM;
		}
	}
	dev->bufsize = bufsize;
	dev->lastcmd = 0xff;
	*rdev = dev;
	return 0;
}

/**
 * \brief Free MIDI event parser
 * \param rdev MIDI event parser
 * \return 0 on success otherwise a negative error code
 *
 * Frees MIDI event parser.
 */
void snd_midi_event_free(snd_midi_event_t *dev)
{
	if (dev != NULL) {
		if (dev->buf)
			free(dev->buf);
		free(dev);
	}
}

#ifdef UNNEEDED_BY_DSSI
/**
 * \brief Enable/disable MIDI command merging
 * \param dev MIDI event parser
 * \param on 0 - enable MIDI command merging, 1 - always pass the command
 *
 * Enable/disable MIDI command merging
 */
void snd_midi_event_no_status(snd_midi_event_t *dev, int on)
{
	dev->nostat = on ? 1 : 0;
}
#endif /* UNNEEDED_BY_DSSI */

/*
 * initialize record
 */
inline static void reset_encode(snd_midi_event_t *dev)
{
	dev->read = 0;
	dev->qlen = 0;
	dev->type = 0;
}

/**
 * \brief Reset MIDI encode parser
 * \param dev MIDI event parser
 * \return 0 on success otherwise a negative error code
 *
 * Resets MIDI encode parser
 */
void snd_midi_event_reset_encode(snd_midi_event_t *dev)
{
	reset_encode(dev);
}

#ifdef UNNEEDED_BY_DSSI
/**
 * \brief Reset MIDI decode parser
 * \param dev MIDI event parser
 * \return 0 on success otherwise a negative error code
 *
 * Resets MIDI decode parser
 */
void snd_midi_event_reset_decode(snd_midi_event_t *dev)
{
	dev->lastcmd = 0xff;
}

/**
 * \brief Initializes MIDI parsers
 * \param dev MIDI event parser
 * \return 0 on success otherwise a negative error code
 *
 * Initializes MIDI parsers (both encode and decode)
 */
void snd_midi_event_init(snd_midi_event_t *dev)
{
	snd_midi_event_reset_encode(dev);
	snd_midi_event_reset_decode(dev);
}

/**
 * \brief Resize MIDI message (event) buffer
 * \param dev MIDI event parser
 * \param bufsize new requested buffer size
 * \return 0 on success otherwise a negative error code
 *
 * Resizes MIDI message (event) buffer.
 */
int snd_midi_event_resize_buffer(snd_midi_event_t *dev, size_t bufsize)
{
	unsigned char *new_buf, *old_buf;

	if (bufsize == dev->bufsize)
		return 0;
	new_buf = malloc(bufsize);
	if (new_buf == NULL)
		return -ENOMEM;
	old_buf = dev->buf;
	dev->buf = new_buf;
	dev->bufsize = bufsize;
	reset_encode(dev);
	if (old_buf)
		free(old_buf);
	return 0;
}
#endif /* UNNEEDED_BY_DSSI */

/**
 * \brief Read bytes and encode to sequencer event if finished
 * \param dev MIDI event parser
 * \param buf MIDI byte stream
 * \param count count of bytes of MIDI byte stream to encode
 * \param ev Result - sequencer event
 * \return count of encoded bytes otherwise a negative error code
 *
 * Read bytes and encode to sequencer event if finished.
 * If complete sequencer event is available, ev->type is not
 * equal to #SND_SEQ_EVENT_NONE.
 */
long snd_midi_event_encode(snd_midi_event_t *dev, const unsigned char *buf, long count, snd_seq_event_t *ev)
{
	long result = 0;
	int rc;

	ev->type = SND_SEQ_EVENT_NONE;

	while (count-- > 0) {
		rc = snd_midi_event_encode_byte(dev, *buf++, ev);
		result++;
		if (rc < 0)
			return rc;
		else if (rc > 0)
			return result;
	}

	return result;
}

/**
 * \brief Read one byte and encode to sequencer event if finished
 * \param dev MIDI event parser
 * \param c a byte of MIDI stream
 * \param ev Result - sequencer event
 * \return 1 - sequencer event is completed, 0 - next byte is required for completion, otherwise a negative error code
 *
 * Read byte and encode to sequencer event if finished.
 */
int snd_midi_event_encode_byte(snd_midi_event_t *dev, int c, snd_seq_event_t *ev)
{
	int rc = 0;

	c &= 0xff;

	if (c >= MIDI_CMD_COMMON_CLOCK) {
		/* real-time event */
		ev->type = status_event[ST_SPECIAL + c - 0xf0].event;
		ev->flags &= ~SNDRV_SEQ_EVENT_LENGTH_MASK;
		ev->flags |= SNDRV_SEQ_EVENT_LENGTH_FIXED;
		return 1;
	}

	if (dev->qlen > 0) {
		/* rest of command */
		dev->buf[dev->read++] = c;
		if (dev->type != ST_SYSEX)
			dev->qlen--;
	} else {
		/* new command */
		dev->read = 1;
		if (c & 0x80) {
			dev->buf[0] = c;
			if ((c & 0xf0) == 0xf0) /* special events */
				dev->type = (c & 0x0f) + ST_SPECIAL;
			else
				dev->type = (c >> 4) & 0x07;
			dev->qlen = status_event[dev->type].qlen;
		} else {
			/* process this byte as argument */
			dev->buf[dev->read++] = c;
			dev->qlen = status_event[dev->type].qlen - 1;
		}
	}
	if (dev->qlen == 0) {
		ev->type = status_event[dev->type].event;
		ev->flags &= ~SNDRV_SEQ_EVENT_LENGTH_MASK;
		ev->flags |= SNDRV_SEQ_EVENT_LENGTH_FIXED;
		if (status_event[dev->type].encode) /* set data values */
			status_event[dev->type].encode(dev, ev);
		rc = 1;
	} else 	if (dev->type == ST_SYSEX) {
		if (c == MIDI_CMD_COMMON_SYSEX_END ||
		    dev->read >= dev->bufsize) {
			ev->flags &= ~SNDRV_SEQ_EVENT_LENGTH_MASK;
			ev->flags |= SNDRV_SEQ_EVENT_LENGTH_VARIABLE;
			ev->type = SNDRV_SEQ_EVENT_SYSEX;
			ev->data.ext.len = dev->read;
			ev->data.ext.ptr = dev->buf;
			if (c != MIDI_CMD_COMMON_SYSEX_END)
				dev->read = 0; /* continue to parse */
			else
				reset_encode(dev); /* all parsed */
			rc = 1;
		}
	}

	return rc;
}

/* encode note event */
static void note_event(snd_midi_event_t *dev, snd_seq_event_t *ev)
{
	ev->data.note.channel = dev->buf[0] & 0x0f;
	ev->data.note.note = dev->buf[1];
	ev->data.note.velocity = dev->buf[2];
}

/* encode one parameter controls */
static void one_param_ctrl_event(snd_midi_event_t *dev, snd_seq_event_t *ev)
{
	ev->data.control.channel = dev->buf[0] & 0x0f;
	ev->data.control.value = dev->buf[1];
}

/* encode pitch wheel change */
static void pitchbend_ctrl_event(snd_midi_event_t *dev, snd_seq_event_t *ev)
{
	ev->data.control.channel = dev->buf[0] & 0x0f;
	ev->data.control.value = (int)dev->buf[2] * 128 + (int)dev->buf[1] - 8192;
}

/* encode midi control change */
static void two_param_ctrl_event(snd_midi_event_t *dev, snd_seq_event_t *ev)
{
	ev->data.control.channel = dev->buf[0] & 0x0f;
	ev->data.control.param = dev->buf[1];
	ev->data.control.value = dev->buf[2];
}

/* encode one parameter value*/
static void one_param_event(snd_midi_event_t *dev, snd_seq_event_t *ev)
{
	ev->data.control.value = dev->buf[1];
}

/* encode song position */
static void songpos_event(snd_midi_event_t *dev, snd_seq_event_t *ev)
{
	ev->data.control.value = (int)dev->buf[2] * 128 + (int)dev->buf[1];
}

#ifdef UNNEEDED_BY_DSSI
/**
 * \brief Decode sequencer event to MIDI byte stream
 * \param dev MIDI event parser
 * \param buf Result - MIDI byte stream
 * \param count Available bytes in MIDI byte stream
 * \param ev Event to decode
 * \return count of decoded bytes otherwise a negative error code
 *
 * Decode sequencer event to MIDI byte stream.
 */
long snd_midi_event_decode(snd_midi_event_t *dev, unsigned char *buf, long count, const snd_seq_event_t *ev)
{
	int cmd;
	long qlen;
	unsigned int type;

	if (ev->type == SNDRV_SEQ_EVENT_NONE)
		return -ENOENT;

	for (type = 0; type < numberof(status_event); type++) {
		if (ev->type == status_event[type].event)
			goto __found;
	}
	for (type = 0; type < numberof(extra_event); type++) {
		if (ev->type == extra_event[type].event)
			return extra_event[type].decode(dev, buf, count, ev);
	}
	return -ENOENT;

      __found:
	if (type >= ST_SPECIAL)
		cmd = 0xf0 + (type - ST_SPECIAL);
	else
		/* data.note.channel and data.control.channel is identical */
		cmd = 0x80 | (type << 4) | (ev->data.note.channel & 0x0f);


	if (cmd == MIDI_CMD_COMMON_SYSEX) {
		qlen = ev->data.ext.len;
		if (count < qlen)
			return -ENOMEM;
		switch (ev->flags & SNDRV_SEQ_EVENT_LENGTH_MASK) {
		case SNDRV_SEQ_EVENT_LENGTH_FIXED:
			return -EINVAL;	/* invalid event */
		}
		memcpy(buf, ev->data.ext.ptr, qlen);
		return qlen;
	} else {
		unsigned char xbuf[4];

		if ((cmd & 0xf0) == 0xf0 || dev->lastcmd != cmd || dev->nostat) {
			dev->lastcmd = cmd;
			xbuf[0] = cmd;
			if (status_event[type].decode)
				status_event[type].decode(ev, xbuf + 1);
			qlen = status_event[type].qlen + 1;
		} else {
			if (status_event[type].decode)
				status_event[type].decode(ev, xbuf + 0);
			qlen = status_event[type].qlen;
		}
		if (count < qlen)
			return -ENOMEM;
		memcpy(buf, xbuf, qlen);
		return qlen;
	}
}
#endif /* UNNEEDED_BY_DSSI */

/* decode note event */
static void note_decode(const snd_seq_event_t *ev, unsigned char *buf)
{
	buf[0] = ev->data.note.note & 0x7f;
	buf[1] = ev->data.note.velocity & 0x7f;
}

/* decode one parameter controls */
static void one_param_decode(const snd_seq_event_t *ev, unsigned char *buf)
{
	buf[0] = ev->data.control.value & 0x7f;
}

/* decode pitch wheel change */
static void pitchbend_decode(const snd_seq_event_t *ev, unsigned char *buf)
{
	int value = ev->data.control.value + 8192;
	buf[0] = value & 0x7f;
	buf[1] = (value >> 7) & 0x7f;
}

/* decode midi control change */
static void two_param_decode(const snd_seq_event_t *ev, unsigned char *buf)
{
	buf[0] = ev->data.control.param & 0x7f;
	buf[1] = ev->data.control.value & 0x7f;
}

/* decode song position */
static void songpos_decode(const snd_seq_event_t *ev, unsigned char *buf)
{
	buf[0] = ev->data.control.value & 0x7f;
	buf[1] = (ev->data.control.value >> 7) & 0x7f;
}

#ifdef UNNEEDED_BY_DSSI
/* decode 14bit control */
static int extra_decode_ctrl14(snd_midi_event_t *dev, unsigned char *buf, int count, const snd_seq_event_t *ev)
{
	unsigned char cmd;
	int idx = 0;

	cmd = MIDI_CMD_CONTROL|(ev->data.control.channel & 0x0f);
	if (ev->data.control.param < 32) {
		if (count < 4)
			return -ENOMEM;
		if (dev->nostat && count < 6)
			return -ENOMEM;
		if (cmd != dev->lastcmd || dev->nostat) {
			if (count < 5)
				return -ENOMEM;
			buf[idx++] = dev->lastcmd = cmd;
		}
		buf[idx++] = ev->data.control.param;
		buf[idx++] = (ev->data.control.value >> 7) & 0x7f;
		if (dev->nostat)
			buf[idx++] = cmd;
		buf[idx++] = ev->data.control.param + 32;
		buf[idx++] = ev->data.control.value & 0x7f;
	} else {
		if (count < 2)
			return -ENOMEM;
		if (cmd != dev->lastcmd || dev->nostat) {
			if (count < 3)
				return -ENOMEM;
			buf[idx++] = dev->lastcmd = cmd;
		}
		buf[idx++] = ev->data.control.param & 0x7f;
		buf[idx++] = ev->data.control.value & 0x7f;
	}
	return idx;
}

/* decode reg/nonreg param */
static int extra_decode_xrpn(snd_midi_event_t *dev, unsigned char *buf, int count, const snd_seq_event_t *ev)
{
	unsigned char cmd;
	char *cbytes;
	static char cbytes_nrpn[4] = { MIDI_CTL_NONREG_PARM_NUM_MSB,
				       MIDI_CTL_NONREG_PARM_NUM_LSB,
				       MIDI_CTL_MSB_DATA_ENTRY,
				       MIDI_CTL_LSB_DATA_ENTRY };
	static char cbytes_rpn[4] =  { MIDI_CTL_REGIST_PARM_NUM_MSB,
				       MIDI_CTL_REGIST_PARM_NUM_LSB,
				       MIDI_CTL_MSB_DATA_ENTRY,
				       MIDI_CTL_LSB_DATA_ENTRY };
	unsigned char bytes[4];
	int idx = 0, i;

	if (count < 8)
		return -ENOMEM;
	if (dev->nostat && count < 12)
		return -ENOMEM;
	cmd = MIDI_CMD_CONTROL|(ev->data.control.channel & 0x0f);
	bytes[0] = ev->data.control.param & 0x007f;
	bytes[1] = (ev->data.control.param & 0x3f80) >> 7;
	bytes[2] = ev->data.control.value & 0x007f;
	bytes[3] = (ev->data.control.value & 0x3f80) >> 7;
	if (cmd != dev->lastcmd && !dev->nostat) {
		if (count < 9)
			return -ENOMEM;
		buf[idx++] = dev->lastcmd = cmd;
	}
	cbytes = ev->type == SND_SEQ_EVENT_NONREGPARAM ? cbytes_nrpn : cbytes_rpn;
	for (i = 0; i < 4; i++) {
		if (dev->nostat)
			buf[idx++] = dev->lastcmd = cmd;
		buf[idx++] = cbytes[i];
		buf[idx++] = bytes[i];
	}
	return idx;
}
#endif /* UNNEEDED_BY_DSSI */
