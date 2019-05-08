/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __AUDIO_PORTS_H__
#define __AUDIO_PORTS_H__

/** \file
 * Ports that transfer audio/midi/other signals to
 * one another.
 */

#include "utils/yaml.h"

#define MAX_DESTINATIONS 600
#define FOREACH_SRCS(port) \
  for (int i = 0; i < port->num_srcs; i++)
#define FOREACH_DESTS(port) \
  for (int i = 0; i < port->num_dests; i++)

/**
 * Special ID for owner_pl, owner_ch, etc. to indicate that
 * the port is not owned.
 */
#define PORT_NOT_OWNED -1

typedef struct Plugin Plugin;
typedef struct MidiEvents MidiEvents;

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
 * Type of owner.
 */
typedef enum PortOwnerType
{
  PORT_OWNER_TYPE_BACKEND,
  PORT_OWNER_TYPE_PLUGIN,
  PORT_OWNER_TYPE_TRACK,
} PortOwnerType;

typedef enum PortFlags
{
  PORT_FLAG_STEREO_L = 0x01,
  PORT_FLAG_STEREO_R = 0x02,
  PORT_FLAG_PIANO_ROLL = 0x04,
  /** See http://lv2plug.in/ns/ext/port-groups/port-groups.html#sideChainOf. */
  PORT_FLAG_SIDECHAIN = 0x08,
  /** See http://lv2plug.in/ns/ext/port-groups/port-groups.html#mainInput
   * and http://lv2plug.in/ns/ext/port-groups/port-groups.html#mainOutput. */
  PORT_FLAG_MAIN_PORT = 0x10,
  OPT_F = 0x20,
} PortFlags;

/**
 * What the internal data is.
 */
typedef enum PortInternalType
{
  INTERNAL_NONE,
  INTERNAL_LV2_PORT, ///< Lv2Port
  INTERNAL_JACK_PORT, ///< jack_port_t
  INTERNAL_PA_PORT, ///< port audio
} PortInternalType;

typedef struct Lv2Port Lv2Port;
typedef struct Channel Channel;
typedef struct Track Track;
typedef enum PanAlgorithm PanAlgorithm;
typedef enum PanLaw PanLaw;

/**
 * Struct used to identify Ports in the project.
 *
 * This should include some members of the original
 * struct enough to identify the port. To be used
 * for sources and dests.
 *
 * This must be filled in before saving and read from
 * while loading to fill in the srcs/dests.
 */
typedef struct PortIdentifier
{
  /**
   * Human readable label.
   */
  char *              label;
  /** Owner type. */
  PortOwnerType       owner_type;
  /** Data type (e.g. AUDIO). */
	PortType            type;
  /** Flow (IN/OUT). */
	PortFlow            flow;
  /** Flags (e.g. is side chain). */
  PortFlags           flags;
  int                 plugin_slot;
  int                 track_pos;
  /** Index (e.g. in plugin's output ports). */
  int                 port_index;
} PortIdentifier;

/**
 * Must ONLY be created via port_new()
 */
typedef struct Port
{
  PortIdentifier      identifier;

  /**
   * Buffer to be reallocated every time the buffer
   * size changes.
   *
   * The buffer size is AUDIO_ENGINE->block_length.
   */
  float *             buf;

  /**
   * Contains raw MIDI data (MIDI ports only)
   */
  MidiEvents *        midi_events;

  /**
   * Inputs and Outputs.
   *
   * These should be serialized, and when loading
   * they shall be used to find the original ports
   * and replace the pointer (also freeing the
   * current one).
   */
  struct Port *       srcs[MAX_DESTINATIONS];
  struct Port *       dests[MAX_DESTINATIONS];
  PortIdentifier      src_ids[MAX_DESTINATIONS];
  PortIdentifier      dest_ids[MAX_DESTINATIONS];
  int                 num_srcs;
  int                 num_dests;

  /**
   * Indicates whether data or lv2_port should be
   * used.
   */
  PortInternalType    internal_type;
  Lv2Port *          lv2_port; ///< used for LV2

  /**
   * Pointer to arbitrary data.
   *
   * Use internal_type to check what data it is.
   */
  void *              data;

  /* ====== flags to indicate port owner ====== */

  Plugin              * plugin;
  Track             * track;

  /** used when loading projects FIXME needed? */
  int                 initialized;

  /** Port undergoing deletion. */
  int                 deleting;
} Port;

static const cyaml_strval_t
port_flow_strings[] =
{
	{ "Unknown",       FLOW_UNKNOWN    },
	{ "Input",         FLOW_INPUT   },
	{ "Output",        FLOW_OUTPUT   },
};

static const cyaml_strval_t
port_owner_type_strings[] =
{
	{ "Backend",   PORT_OWNER_TYPE_BACKEND    },
	{ "Plugin",    PORT_OWNER_TYPE_PLUGIN   },
	{ "Track",     PORT_OWNER_TYPE_TRACK   },
};

static const cyaml_strval_t
port_type_strings[] =
{
	{ "Unknown",       TYPE_UNKNOWN    },
	{ "Control",       TYPE_CONTROL   },
	{ "Audio",         TYPE_AUDIO   },
	{ "Event",         TYPE_EVENT   },
	{ "CV",            TYPE_CV   },
};

static const cyaml_strval_t
port_internal_type_strings[] =
{
	{ "LV2 Port",       INTERNAL_LV2_PORT    },
	{ "JACK Port",      INTERNAL_JACK_PORT   },
};

static const cyaml_schema_field_t
port_identifier_fields_schema[] =
{
  CYAML_FIELD_STRING_PTR (
    "label", CYAML_FLAG_POINTER,
    PortIdentifier, label,
   	0, CYAML_UNLIMITED),
  CYAML_FIELD_ENUM (
    "owner_type", CYAML_FLAG_DEFAULT,
    PortIdentifier, owner_type, port_owner_type_strings,
    CYAML_ARRAY_LEN (port_type_strings)),
  CYAML_FIELD_ENUM (
    "type", CYAML_FLAG_DEFAULT,
    PortIdentifier, type, port_type_strings,
    CYAML_ARRAY_LEN (port_type_strings)),
  CYAML_FIELD_ENUM (
    "flow", CYAML_FLAG_DEFAULT,
    PortIdentifier, flow, port_flow_strings,
    CYAML_ARRAY_LEN (port_flow_strings)),

	CYAML_FIELD_END,
};

static const cyaml_schema_value_t
port_identifier_schema = {
	CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    PortIdentifier, port_identifier_fields_schema),
};

static const cyaml_schema_field_t
port_fields_schema[] =
{
  CYAML_FIELD_MAPPING (
    "identifier",
    /* direct struct inside struct -> default */
    CYAML_FLAG_DEFAULT,
    Port, identifier, port_identifier_fields_schema),
  CYAML_FIELD_SEQUENCE_COUNT (
    "src_ids", CYAML_FLAG_DEFAULT,
    Port, src_ids, num_srcs,
    &port_identifier_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "dest_ids", CYAML_FLAG_DEFAULT,
    Port, dest_ids, num_dests,
    &port_identifier_schema, 0, CYAML_UNLIMITED),
  CYAML_FIELD_ENUM (
    "internal_type", CYAML_FLAG_DEFAULT,
    Port, internal_type, port_internal_type_strings,
    CYAML_ARRAY_LEN (port_internal_type_strings)),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
port_schema = {
	CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    Port, port_fields_schema),
};

/**
 * L & R port, for convenience.
 *
 * Must ONLY be created via stereo_ports_new()
 */
typedef struct StereoPorts
{
  Port       * l;
  Port       * r;
} StereoPorts;

static const cyaml_schema_field_t
  stereo_ports_fields_schema[] =
{
  CYAML_FIELD_MAPPING_PTR (
    "l", CYAML_FLAG_POINTER,
    StereoPorts, l,
    port_fields_schema),
  CYAML_FIELD_MAPPING_PTR (
    "r", CYAML_FLAG_POINTER,
    StereoPorts, r,
    port_fields_schema),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
  stereo_ports_schema = {
	CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    StereoPorts, stereo_ports_fields_schema),
};

/**
 * This function finds the Ports corresponding to
 * the PortIdentifiers for srcs and dests.
 *
 * Should be called after the ports are loaded from
 * yml.
 */
void
port_init_loaded (Port * port);

Port *
port_find_from_identifier (
  PortIdentifier * id);

static inline void
port_identifier_copy (
  PortIdentifier * src,
  PortIdentifier * dest)
{
  dest->label = g_strdup (src->label);
  dest->owner_type = src->owner_type;
  dest->type = src->type;
  dest->flow = src->flow;
  dest->flags = src->flags;
  dest->plugin_slot = src->plugin_slot;
  dest->track_pos = src->track_pos;
  dest->port_index = src->port_index;
}

void
stereo_ports_init_loaded (StereoPorts * sp);

/**
 * Creates port.
 */
Port *
port_new (char * label);

/**
 * Creates port.
 */
Port *
port_new_with_type (
  PortType     type,
  PortFlow     flow,
  char         * label);

/**
 * Creates port and adds given data to it.
 *
 * @param internal_type The internal data format.
 * @param data The data.
 */
Port *
port_new_with_data (
  PortInternalType internal_type,
  PortType     type,
  PortFlow     flow,
  char         * label,
  void         * data);

/**
 * Creates stereo ports.
 */
StereoPorts *
stereo_ports_new (Port * l, Port * r);

/**
 * Gathers all ports in the project and puts them
 * in the given array and size.
 */
void
port_get_all (
  Port ** ports,
  int *   size);

/**
 * Deletes port, doing required cleanup and updating counters.
 */
void
port_free (Port * port);

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
 * Disconnects dests of port.
 */
static inline int
port_disconnect_dests (Port * src)
{
  g_warn_if_fail (src);

  for (int i = 0; i < src->num_dests; i++)
    {
      port_disconnect (src, src->dests[i]);
    }
  return 0;
}

/**
 * Apply given fader value to port.
 */
void
port_apply_fader (Port * port, float amp);

/**
 * First sets port buf to 0, then sums the given port signal from its inputs.
 */
void
port_sum_signal_from_inputs (Port * port);

/**
 * Sets the owner channel & its ID.
 */
static inline void
port_set_owner_track (
  Port *    port,
  Track *   track)
{
  g_warn_if_fail (port && track);

  port->track = track;
  port->identifier.owner_type =
    PORT_OWNER_TYPE_TRACK;
}

/**
 * Sets the owner plugin & its ID.
 */
void
port_set_owner_plugin (
  Port *   port,
  Plugin * pl);

int
ports_connected (Port * src, Port * dest);

/**
 * Disconnects all the given ports.
 */
static inline void
ports_disconnect (
  Port ** ports,
  int     num_ports,
  int     deleting)
{
  int i, j;
  Port * port;

  /* go through each port */
  for (i = 0; i < num_ports; i++)
    {
      port = ports[i];
      port->deleting = deleting;

      /* disconnect incoming ports */
      if (port->identifier.flow == FLOW_INPUT)
        {
          for (j = 0; j < port->num_srcs; j++)
            {
              port_disconnect (port->srcs[j], port);
            }
        }
        /* disconnect outgoing ports */
        else if (port->identifier.flow == FLOW_OUTPUT)
          {
            for (j = 0; j < port->num_dests; j++)
              {
                port_disconnect (
                  port, port->dests[j]);
              }
          }
    }
}

/**
 * Removes all the given ports from the project,
 * optionally freeing them.
 */
int
ports_remove (
  Port ** ports,
  int *   num_ports);

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

/**
 * Disconnects all srcs and dests from port.
 */
int
port_disconnect_all (Port * port);

/**
 * Applies the pan to the given L/R ports.
 */
void
port_apply_pan_stereo (Port *       l,
                       Port *       r,
                       float        pan,
                       PanLaw       pan_law,
                       PanAlgorithm pan_algo);

/**
 * Applies the pan to the given L/R ports.
 */
void
port_apply_pan (
  Port *       self,
  float        pan,
  PanLaw       pan_law,
  PanAlgorithm pan_algo);

#endif
