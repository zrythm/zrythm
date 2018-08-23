/*
 * audio/ports.h - ports to pass when processing the audio signal
 *
 * copyright (c) 2018 alexandros theodotou
 *
 * this file is part of zrythm
 *
 * zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the gnu general public license as published by
 * the free software foundation, either version 3 of the license, or
 * (at your option) any later version.
 *
 * zrythm is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * merchantability or fitness for a particular purpose.  see the
 * gnu general public license for more details.
 *
 * you should have received a copy of the gnu general public license
 * along with zrythm.  if not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __AUDIO_PORTS_H__
#define __AUDIO_PORTS_H__

/** \file
 * Port API. */

#include <jack/jack.h>

#define MAX_DESTINATIONS 23

typedef struct Plugin Plugin;

enum PortFlow {
	FLOW_UNKNOWN,
	FLOW_INPUT,
	FLOW_OUTPUT
};

enum PortType {
	TYPE_UNKNOWN,
	TYPE_CONTROL,
	TYPE_AUDIO,
	TYPE_EVENT,
	TYPE_CV
};

typedef jack_default_audio_sample_t   sample_t;
typedef jack_nframes_t                nframes_t;

struct LilvPort;
struct LV2_Evbuf;


typedef struct Port
{
  sample_t        * buf;      ///< buffer to be allocated with malloc
  nframes_t       nframes;        ///< number of frames (size of samples array)
  uint8_t         id;             ///< each port has an ID so that they can get connected
	enum PortType   type;       ///< Data type
	enum PortFlow   flow;       ///< Data flow direction
  struct Port *  srcs[MAX_DESTINATIONS];  ///< ports coming in
  struct Port *  dests[MAX_DESTINATIONS];  ///< destination ports
  int         num_srcs; ///< counter
  int         num_dests; ///< counter
  Plugin            * owner;           ///< owner plugin
} Port;

/**
 * Creates port.
 */
Port *
port_new ();

/**
 * Deletes port, doing required cleanup and updating counters.
 */
void
port_delete (Port * port);

/**
 * Connets src to dest.
 */
int
port_connect (Port * src, Port * dest);

/**
 * Disconnects src from dest.
 */
int
port_disconnect (Port * src, Port * dest);

#endif
