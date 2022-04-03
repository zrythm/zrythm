/*
  LV2 Fifths Example Plugin
  Copyright 2014-2016 David Robillard <d@drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef __cplusplus
#  include <stdbool.h>
#endif

#include "lv2/atom/atom.h"
#include "lv2/atom/forge.h"
#include "lv2/atom/util.h"
#include "lv2/core/lv2.h"
#include "lv2/core/lv2_util.h"
#include "lv2/log/log.h"
#include "lv2/log/logger.h"
#include "lv2/midi/midi.h"
#include "lv2/urid/urid.h"
#include "uris.h"

enum
{
  FIFTHS_IN = 0,
  FIFTHS_OUT = 1
};

typedef struct
{
  // Features
  LV2_URID_Map * map;
  LV2_Log_Logger logger;

  // Ports
  const LV2_Atom_Sequence * in_port;
  float *                   out_port;

  /** Atom forge. */
  LV2_Atom_Forge forge;

  /** Current velocity (0 to silence). */
  uint8_t velocities[128];

  // URIs
  FifthsURIs uris;
} Fifths;

static void
connect_port (
  LV2_Handle instance,
  uint32_t   port,
  void *     data)
{
  Fifths * self = (Fifths *) instance;
  switch (port)
    {
    case FIFTHS_IN:
      self->in_port =
        (const LV2_Atom_Sequence *) data;
      break;
    case FIFTHS_OUT:
      self->out_port = (float *) data;
      break;
    default:
      break;
    }
}

static LV2_Handle
instantiate (
  const LV2_Descriptor *      descriptor,
  double                      rate,
  const char *                path,
  const LV2_Feature * const * features)
{
  // Allocate and initialise instance structure.
  Fifths * self =
    (Fifths *) calloc (1, sizeof (Fifths));
  if (!self)
    {
      return NULL;
    }

  // Scan host features for URID map
  const char * missing = lv2_features_query (
    features, LV2_LOG__log, &self->logger.log,
    false, LV2_URID__map, &self->map, true, NULL);
  lv2_log_logger_set_map (&self->logger, self->map);
  if (missing)
    {
      lv2_log_error (
        &self->logger, "Missing feature <%s>\n",
        missing);
      free (self);
      return NULL;
    }

  map_fifths_uris (self->map, &self->uris);

  /* init atom forge */
  lv2_atom_forge_init (&self->forge, self->map);

  return (LV2_Handle) self;
}

static void
cleanup (LV2_Handle instance)
{
  free (instance);
}

/**
 * Processes 1 sample.
 */
static void
process (Fifths * self, uint32_t * offset)
{
  float * current_out = &self->out_port[*offset];
  *current_out = 0.f;

  for (int i = 0; i < 128; i++)
    {
      uint8_t vel = self->velocities[i];

      if (vel == 0)
        continue;

      *current_out = 1.f;
    }

  (*offset)++;
}

static void
run (LV2_Handle instance, uint32_t sample_count)
{
  Fifths * self = (Fifths *) instance;

  uint32_t processed = 0;

  /* read incoming events from host and UI */
  LV2_ATOM_SEQUENCE_FOREACH (self->in_port, ev)
  {
    if (ev->body.type == self->uris.midi_Event)
      {
        const uint8_t * const msg =
          (const uint8_t *) (ev + 1);
        switch (lv2_midi_message_type (msg))
          {
          case LV2_MIDI_MSG_NOTE_ON:
            /* note with velocity 0 can be note off */
            if (msg[2] == 0)
              {
                self->velocities[msg[1]] = 0;
              }
            else
              {
                self->velocities[msg[1]] = msg[2];
              }
            break;
          case LV2_MIDI_MSG_NOTE_OFF:
            self->velocities[msg[1]] = 0;
            break;
          case LV2_MIDI_MSG_CONTROLLER:
            if (
              msg[1] == 0x7b || // all notes off
              msg[1] == 0x78)   // all sound off
              {
                for (int i = 0; i < 128; i++)
                  {
                    self->velocities[i] = 0;
                  }
              }
            break;
          default:
            /*printf ("unknown MIDI message\n");*/
            break;
          }
        while (processed < ev->time.frames)
          {
            process (self, &processed);
          }
      }
    if (lv2_atom_forge_is_object_type (
          &self->forge, ev->body.type))
      {
        const LV2_Atom_Object * obj =
          (const LV2_Atom_Object *) &ev->body;

        /* TODO */
        (void) obj;
      }
  }

  for (uint32_t i = processed; i < sample_count; i++)
    {
      process (self, &processed);
    }

#if 0
  memset (
    self->out_port, 0, sample_count * sizeof (float));
#endif
}

static const void *
extension_data (const char * uri)
{
  return NULL;
}

static const LV2_Descriptor descriptor = {
  TEST_INSTRUMENT_URI,
  instantiate,
  connect_port,
  NULL, // activate,
  run,
  NULL, // deactivate,
  cleanup,
  extension_data
};

LV2_SYMBOL_EXPORT
const LV2_Descriptor *
lv2_descriptor (uint32_t index)
{
  switch (index)
    {
    case 0:
      return &descriptor;
    default:
      return NULL;
    }
}
