// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * Copyright (C) 2015 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#define PLB_URI "http://gareus.org/oss/lv2/plumbing#"
#define MAX_CHANNELS 8

typedef struct
{
  /* control ports */
  const float * input[MAX_CHANNELS];
  float *       output[MAX_CHANNELS];

  LV2_Atom_Forge       forge;
  LV2_Atom_Forge_Frame frame;
  LV2_URID_Map *       map;

  const LV2_Atom_Sequence * midiin;
  LV2_Atom_Sequence *       midiout;

  uint32_t channels;
  bool     midieat;
} LVMidiPlumbing;

static LV2_Handle
m_instantiate (
  const LV2_Descriptor *      descriptor,
  double                      rate,
  const char *                bundle_path,
  const LV2_Feature * const * features)
{
  LVMidiPlumbing * self =
    (LVMidiPlumbing *) calloc (1, sizeof (LVMidiPlumbing));
  if (!self)
    return NULL;

  // TODO parse atoi(), check bounds
  if (!strcmp (descriptor->URI, PLB_URI "eat1"))
    {
      self->channels = 1;
      self->midieat = true;
    }
  else if (!strcmp (descriptor->URI, PLB_URI "eat2"))
    {
      self->channels = 2;
      self->midieat = true;
    }
  else if (!strcmp (descriptor->URI, PLB_URI "eat3"))
    {
      self->channels = 3;
      self->midieat = true;
    }
  else if (!strcmp (descriptor->URI, PLB_URI "eat4"))
    {
      self->channels = 4;
      self->midieat = true;
    }

  else if (!strcmp (descriptor->URI, PLB_URI "gen1"))
    {
      self->channels = 1;
      self->midieat = false;
    }
  else if (!strcmp (descriptor->URI, PLB_URI "gen2"))
    {
      self->channels = 2;
      self->midieat = false;
    }
  else if (!strcmp (descriptor->URI, PLB_URI "gen3"))
    {
      self->channels = 3;
      self->midieat = false;
    }
  else if (!strcmp (descriptor->URI, PLB_URI "gen4"))
    {
      self->channels = 4;
      self->midieat = false;
    }

  else
    {
      free (self);
      return NULL;
    }

  for (int i = 0; features[i]; ++i)
    {
      if (!strcmp (features[i]->URI, LV2_URID__map))
        {
          self->map = (LV2_URID_Map *) features[i]->data;
        }
    }

  if (!self->map)
    {
      fprintf (stderr, "Host does not support urid:map\n");
      free (self);
      return NULL;
    }

  lv2_atom_forge_init (&self->forge, self->map);

  return (LV2_Handle) self;
}

static void
m_connect_port (LV2_Handle instance, uint32_t port, void * data)
{
  LVMidiPlumbing * self = (LVMidiPlumbing *) instance;

  if (self->midieat)
    {
      if (port == 0)
        {
          self->midiin = (const LV2_Atom_Sequence *) data;
        }
      else if (port == 1)
        {
          self->midiout = (LV2_Atom_Sequence *) data;
        }
      else
        {
          const uint32_t p = (port - 2) >> 1;
          if (port % 2 == 0)
            {
              self->output[p] = data;
            }
          else
            {
              self->input[p] = data;
            }
        }
    }
  else
    {
      if (port == 0)
        {
          if (self->midieat)
            {
              self->midiin = (const LV2_Atom_Sequence *) data;
            }
          else
            {
              self->midiout = (LV2_Atom_Sequence *) data;
            }
        }
      else
        {
          const uint32_t p = (port - 1) >> 1;
          if (port % 2 == 0)
            {
              self->output[p] = data;
            }
          else
            {
              self->input[p] = data;
            }
        }
    }
}

static void
m_run (LV2_Handle instance, uint32_t n_samples)
{
  LVMidiPlumbing * self = (LVMidiPlumbing *) instance;
  if (self->midiout)
    {
      const uint32_t capacity = self->midiout->atom.size;
      lv2_atom_forge_set_buffer (
        &self->forge, (uint8_t *) self->midiout, capacity);
      lv2_atom_forge_sequence_head (
        &self->forge, &self->frame, 0);
    }

  for (uint32_t c = 0; c < self->channels; ++c)
    {
      if (self->input[c] != self->output[c])
        {
          memcpy (
            self->output[c], self->input[c],
            sizeof (float) * n_samples);
        }
    }
}

static void
m_cleanup (LV2_Handle instance)
{
  free (instance);
}
