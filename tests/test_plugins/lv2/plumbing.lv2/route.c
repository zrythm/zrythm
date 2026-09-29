// SPDX-FileCopyrightText: 2015 Robin Gareus <robin@gareus.org>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#define PLB_URI "http://gareus.org/oss/lv2/plumbing#"
#define MAX_CHANNELS 8

typedef struct
{
  /* control ports */
  const float * input[MAX_CHANNELS];
  float *       output[MAX_CHANNELS];

  uint32_t n_in;
  uint32_t n_out;
  float *  chnmap[MAX_CHANNELS];
} LVAudioPlumbing;

static LV2_Handle
a_instantiate (
  const LV2_Descriptor *      descriptor,
  double                      rate,
  const char *                bundle_path,
  const LV2_Feature * const * features)
{
  LVAudioPlumbing * self =
    (LVAudioPlumbing *) calloc (1, sizeof (LVAudioPlumbing));
  if (!self)
    return NULL;

  // TODO parse atoi(), check bounds
  if (!strcmp (descriptor->URI, PLB_URI "route_1_2"))
    {
      self->n_in = 1;
      self->n_out = 2;
    }
  else if (!strcmp (descriptor->URI, PLB_URI "route_1_3"))
    {
      self->n_in = 1;
      self->n_out = 3;
    }
  else if (!strcmp (descriptor->URI, PLB_URI "route_1_4"))
    {
      self->n_in = 1;
      self->n_out = 4;
    }

  else if (!strcmp (descriptor->URI, PLB_URI "route_2_1"))
    {
      self->n_in = 2;
      self->n_out = 1;
    }
  else if (!strcmp (descriptor->URI, PLB_URI "route_2_2"))
    {
      self->n_in = 2;
      self->n_out = 2;
    }
  else if (!strcmp (descriptor->URI, PLB_URI "route_2_3"))
    {
      self->n_in = 2;
      self->n_out = 3;
    }
  else if (!strcmp (descriptor->URI, PLB_URI "route_2_4"))
    {
      self->n_in = 2;
      self->n_out = 4;
    }

  else if (!strcmp (descriptor->URI, PLB_URI "route_3_1"))
    {
      self->n_in = 3;
      self->n_out = 1;
    }
  else if (!strcmp (descriptor->URI, PLB_URI "route_3_2"))
    {
      self->n_in = 3;
      self->n_out = 2;
    }
  else if (!strcmp (descriptor->URI, PLB_URI "route_3_3"))
    {
      self->n_in = 3;
      self->n_out = 3;
    }
  else if (!strcmp (descriptor->URI, PLB_URI "route_3_4"))
    {
      self->n_in = 3;
      self->n_out = 4;
    }

  else if (!strcmp (descriptor->URI, PLB_URI "route_4_1"))
    {
      self->n_in = 4;
      self->n_out = 1;
    }
  else if (!strcmp (descriptor->URI, PLB_URI "route_4_2"))
    {
      self->n_in = 4;
      self->n_out = 2;
    }
  else if (!strcmp (descriptor->URI, PLB_URI "route_4_3"))
    {
      self->n_in = 4;
      self->n_out = 3;
    }
  else if (!strcmp (descriptor->URI, PLB_URI "route_4_4"))
    {
      self->n_in = 4;
      self->n_out = 4;
    }
  else
    {
      free (self);
      return NULL;
    }

  return (LV2_Handle) self;
}

static void
a_connect_port (LV2_Handle instance, uint32_t port, void * data)
{
  LVAudioPlumbing * self = (LVAudioPlumbing *) instance;

  if (port < self->n_out)
    {
      self->chnmap[port] = data;
    }
  else if (port < self->n_out + self->n_in)
    {
      self->input[port - self->n_out] = data;
    }
  else if (port < self->n_out + self->n_in + self->n_out)
    {
      self->output[port - self->n_in - self->n_out] = data;
    }
  else
    {
      assert (0);
    }
}

static void
a_run (LV2_Handle instance, uint32_t n_samples)
{
  LVAudioPlumbing * self = (LVAudioPlumbing *) instance;
  uint32_t          map[MAX_CHANNELS];
  for (uint32_t c = 0; c < self->n_out; ++c)
    {
      map[c] = *(self->chnmap[c]);
      if (map[c] > self->n_in)
        map[c] = 0;
    }

  // copy data if required
  // TODO reverse sort by map and use temp buffers for x-over inline.
  for (uint32_t c = 0; c < self->n_out; ++c)
    {
      if (map[c] > 0 && self->input[map[c] - 1] != self->output[c])
        {
          memcpy (
            self->output[c], self->input[map[c] - 1],
            sizeof (float) * n_samples);
        }
    }
  // clear after to allow in-place processing
  for (uint32_t c = 0; c < self->n_out; ++c)
    {
      if (map[c] == 0)
        {
          memset (self->output[c], 0, sizeof (float) * n_samples);
        }
    }
}

static void
a_cleanup (LV2_Handle instance)
{
  free (instance);
}
