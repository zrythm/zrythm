// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/* Two plugins for testing time:Position delivery: pos-probe declares a
 * single atom input supporting both MIDI and time:Position,
 * pos-probe-separate declares separate MIDI and Position atom inputs,
 * a latency control output that reports NaN, and a MIDI output that
 * echoes every received MIDI event into a buffer sized by the
 * rsz:minimumSize the plugin declares for it. Both emit the transport's
 * frames per second as a DC offset, plus the number of MIDI events
 * received. */

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "lv2/atom/atom.h"
#include "lv2/atom/forge.h"
#include "lv2/atom/util.h"
#include "lv2/core/lv2.h"
#include "lv2/midi/midi.h"
#include "lv2/time/time.h"
#include "lv2/urid/urid.h"

#define POS_PROBE_URI "http://lv2plug.in/plugins/pos-probe"
#define POS_PROBE_SEPARATE_URI "http://lv2plug.in/plugins/pos-probe-separate"

typedef enum
{
  IN = 0,
  OUT = 1,
  DEGREE = 2
} PortIndex;

typedef enum
{
  SEPARATE_MIDI = 0,
  SEPARATE_POSITION = 1,
  SEPARATE_OUTPUT = 2,
  SEPARATE_LATENCY = 3,
  SEPARATE_MIDI_OUT = 4
} SeparatePortIndex;

typedef struct
{
  /* Unconnected sequence pointers read as NULL: the probes report
   * silence (a DC of 0) instead of dereferencing them */
  const LV2_Atom_Sequence * in;
  const LV2_Atom_Sequence * in_position;
  float *                   out;
  float *                   latency;
  const float *             degree;
  LV2_Atom_Sequence *       midi_out;

  LV2_URID_Map * map;
  LV2_URID       time_Position;
  LV2_URID       time_framesPerSecond;
  LV2_URID       atom_Object;
  LV2_URID       atom_Float;
  LV2_URID       midi_MidiEvent;
  LV2_Atom_Forge forge;
  double         rate;
} PosProbe;

static LV2_Handle
instantiate (
  const LV2_Descriptor *      descriptor,
  double                      rate,
  const char *                bundle_path,
  const LV2_Feature * const * features)
{
  PosProbe * self = (PosProbe *) calloc (1, sizeof (PosProbe));
  if (self == NULL)
    return NULL;

  self->rate = rate;
  for (const LV2_Feature * const * f = features; *f != NULL; ++f)
    {
      if (strcmp ((*f)->URI, LV2_URID__map) == 0)
        {
          self->map = (LV2_URID_Map *) (*f)->data;
        }
    }
  if (self->map == NULL)
    {
      free (self);
      return NULL;
    }

  self->time_Position = self->map->map (self->map->handle, LV2_TIME__Position);
  self->time_framesPerSecond =
    self->map->map (self->map->handle, LV2_TIME__framesPerSecond);
  self->atom_Object = self->map->map (self->map->handle, LV2_ATOM__Object);
  self->atom_Float = self->map->map (self->map->handle, LV2_ATOM__Float);
  self->midi_MidiEvent = self->map->map (self->map->handle, LV2_MIDI__MidiEvent);
  lv2_atom_forge_init (&self->forge, self->map);

  return (LV2_Handle) self;
}

static void
connect_port (LV2_Handle instance, uint32_t port, void * data)
{
  PosProbe * self = (PosProbe *) instance;

  switch ((PortIndex) port)
    {
    case IN:
      self->in = (const LV2_Atom_Sequence *) data;
      break;
    case OUT:
      self->out = (float *) data;
      break;
    case DEGREE:
      self->degree = (const float *) data;
      break;
    }
}

static void
connect_port_separate (LV2_Handle instance, uint32_t port, void * data)
{
  PosProbe * self = (PosProbe *) instance;

  switch ((SeparatePortIndex) port)
    {
    case SEPARATE_MIDI:
      self->in = (const LV2_Atom_Sequence *) data;
      break;
    case SEPARATE_POSITION:
      self->in_position = (const LV2_Atom_Sequence *) data;
      break;
    case SEPARATE_OUTPUT:
      self->out = (float *) data;
      break;
    case SEPARATE_LATENCY:
      self->latency = (float *) data;
      break;
    case SEPARATE_MIDI_OUT:
      self->midi_out = (LV2_Atom_Sequence *) data;
      break;
    }
}

static void
activate (LV2_Handle instance)
{
}

/* DC offset carried by a position event, or 0 when the sequence holds
 * no Float-typed framesPerSecond */
static float
sequence_position_dc (const PosProbe * self, const LV2_Atom_Sequence * seq)
{
  LV2_ATOM_SEQUENCE_FOREACH (seq, ev)
  {
    if (ev->body.type != self->atom_Object)
      continue;
    const LV2_Atom_Object * obj = (const LV2_Atom_Object *) &ev->body;
    if (obj->body.otype != self->time_Position)
      continue;
    LV2_ATOM_OBJECT_FOREACH ((LV2_Atom_Object *) obj, prop)
    {
      if (
        prop->key == self->time_framesPerSecond
        && prop->value.type == self->atom_Float)
        {
          return *(const float *) LV2_ATOM_BODY_CONST (&prop->value)
                 / (float) self->rate;
        }
    }
  }
  return 0.0f;
}

static void
run (LV2_Handle instance, uint32_t n_samples)
{
  PosProbe * self = (PosProbe *) instance;

  float dc = 0.0f;
  if (self->in != NULL)
    dc = sequence_position_dc (self, self->in);
  if (self->in_position != NULL)
    dc = sequence_position_dc (self, self->in_position);

  /* The number of MIDI events is added to the DC offset so event
   * delivery is observable */
  unsigned midi_events = 0;
  if (self->in != NULL)
    {
      LV2_ATOM_SEQUENCE_FOREACH (self->in, ev)
      {
        if (ev->body.type == self->midi_MidiEvent)
          ++midi_events;
      }
    }
  dc += (float) midi_events;

  if (self->latency != NULL)
    self->latency[0] = NAN;

  /* The echo output is written trusting the rsz:minimumSize this
   * plugin declares for the port (65536 bytes) */
  if (self->midi_out != NULL)
    {
      lv2_atom_forge_set_buffer (
        &self->forge, (uint8_t *) self->midi_out, 65536);
      LV2_Atom_Forge_Frame seq_frame;
      lv2_atom_forge_sequence_head (&self->forge, &seq_frame, 0);
      if (self->in != NULL)
        {
          LV2_ATOM_SEQUENCE_FOREACH (self->in, ev)
          {
            if (ev->body.type != self->midi_MidiEvent)
              continue;
            lv2_atom_forge_frame_time (&self->forge, 0);
            lv2_atom_forge_atom (
              &self->forge, ev->body.size, self->midi_MidiEvent);
            lv2_atom_forge_write (
              &self->forge, LV2_ATOM_BODY_CONST (&ev->body), ev->body.size);
          }
        }
      lv2_atom_forge_pop (&self->forge, &seq_frame);
    }

  for (uint32_t i = 0; i < n_samples; ++i)
    self->out[i] = dc;
}

static void
deactivate (LV2_Handle instance)
{
}

static void
cleanup (LV2_Handle instance)
{
  free (instance);
}

static const void *
extension_data (const char * uri)
{
  return NULL;
}

static const LV2_Descriptor pos_probe_descriptor = {
  POS_PROBE_URI,    instantiate, connect_port,     activate,
  run,              deactivate,  cleanup,          extension_data
};

static const LV2_Descriptor pos_probe_separate_descriptor = {
  POS_PROBE_SEPARATE_URI, instantiate, connect_port_separate, activate,
  run,                    deactivate,  cleanup,               extension_data
};

LV2_SYMBOL_EXPORT
const LV2_Descriptor *
lv2_descriptor (uint32_t index)
{
  switch (index)
    {
    case 0:
      return &pos_probe_descriptor;
    case 1:
      return &pos_probe_separate_descriptor;
    default:
      return NULL;
    }
}
