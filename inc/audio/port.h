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

#include "audio/midi.h"

#include <jack/jack.h>

#define MAX_DESTINATIONS 23

typedef struct Plugin Plugin;

typedef enum PortFlow {
	FLOW_UNKNOWN,
	FLOW_INPUT,
	FLOW_OUTPUT
} PortFlow;

typedef enum PortType {
	TYPE_UNKNOWN,
	TYPE_CONTROL,
	TYPE_AUDIO,
	TYPE_EVENT,
	TYPE_CV
} PortType;

/**
 * What the internal data is
 */
typedef enum PortInternalType
{
  INTERNAL_LV2_PORT,                ///< LV2_Port (see lv2_plugin.c)
  INTERNAL_JACK_PORT                ///< jack_port_t
} PortInternalType;

typedef jack_default_audio_sample_t   sample_t;
typedef jack_nframes_t                nframes_t;

typedef struct LV2_Port LV2_Port;
typedef struct Channel Channel;

/**
 * Must ONLY be created via port_new()
 */
typedef struct Port
{
  int                 id;             ///< each port has an ID so that they can get connected
  char                * label;     ///< human readable label
  sample_t            * buf;      ///< buffer to be allocated with malloc, for AUDIO
  Midi_Events         midi_events;   ///< contains the raw MIDI packets, for MIDI ports
  /* FIXME necessary? */
  nframes_t           nframes;        ///< number of frames (size of samples array)
	PortType            type;       ///< Data type
	PortFlow            flow;       ///< Data flow direction
  struct Port         * srcs[MAX_DESTINATIONS];  ///< ports coming in
  struct Port         * dests[MAX_DESTINATIONS];  ///< destination ports
  int                 num_srcs; ///< counter
  int                 num_dests; ///< counter
  LV2_Port            * lv2_port;    ///< used for LV2
  PortInternalType   internal_type;
  void                * data;    ///< pointer to arbitrary data. use internal to check what data it is
  int                 owner_jack;        ///< 1 if owner is JACK
  Plugin              * owner_pl;           ///< owner plugin, for plugins
  Channel             * owner_ch;           ///< owner channel, for channels
  int                 exported;   ///< used in xml project file export
} Port;

/**
 * L & R port, for convenience.
 *
 * Must ONLY be craeted via stereo_ports_new()
 */
typedef struct StereoPorts
{
  Port       * l;
  Port       * r;
} StereoPorts;

/**
 * Creates port.
 */
Port *
port_new (nframes_t nframes, char * label);

/**
 * Creates port.
 */
Port *
port_new_with_type (nframes_t    nframes,
                    PortType     type,
                    PortFlow     flow,
                    char         * label);

/**
 * Creates port and adds given data to it
 */
Port *
port_new_with_data (nframes_t    nframes,
                    PortInternalType internal_type, ///< the internal data format
                    PortType     type,
                    PortFlow     flow,
                    char         * label,
                    void         * data);   ///< the data

/**
 * Creates stereo ports.
 */
StereoPorts *
stereo_ports_new (Port * l, Port * r);

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

/**
 * Apply given fader value to port.
 */
void
port_apply_fader (Port * port, float dbfs);

/**
 * First sets port buf to 0, then sums the given port signal from its inputs.
 */
void
port_sum_signal_from_inputs (Port * port, nframes_t nframes);

/**
 * if port buffer size changed, reallocate port buffer, otherwise memset to 0.
 */
//void
//port_init_buf (Port *port, nframes_t nframes);

/**
 * Prints all connections.
 */
void
port_print_connections_all ();

/**
 * Clears the port buffer.
 */
void
port_clear_buffer (Port * port);

#endif
