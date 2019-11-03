/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of ZPlugins
 *
 * ZPlugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * ZPlugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU General Affero Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "zlfo_common.h"

#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/log/log.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

const float PI = (float) M_PI;

typedef struct ZLFO
{
  /** Plugin ports. */
  const LV2_Atom_Sequence* control;
  LV2_Atom_Sequence* notify;
  const float * freq;
  const float * phase;
  const int *   follow_time;
  float *       sine;
  float *       saw_up;
  float *       saw_down;
  float *       triangle;
  float *       square;
  float *       random;

  /** Frequency during the last run. */
  float         last_freq;

  /** Plugin samplerate. */
  double        samplerate;

  /** Total samples processed. */
  uint64_t      samples_processed;

  /** Atom forge. */
  LV2_Atom_Forge forge;

  /** Log feature. */
  LV2_Log_Log *        log;

  /** Map feature. */
  LV2_URID_Map *       map;

  ZLfoUris      uris;

  /**
   * Sine multiplier.
   *
   * This is a pre-calculated variable that is used
   * when calculating the sine value.
   */
  float         sine_multiplier;

  float         saw_up_multiplier;

} ZLFO;

static LV2_Handle
instantiate (
  const LV2_Descriptor*     descriptor,
  double                    rate,
  const char*               bundle_path,
  const LV2_Feature* const* features)
{
  ZLFO * self = calloc (1, sizeof (ZLFO));

  self->samplerate = rate;

#define HAVE_FEATURE(x) \
  (!strcmp(features[i]->URI, x))

  for (int i = 0; features[i]; ++i)
    {
      if (HAVE_FEATURE (LV2_URID__map))
        {
          self->map =
            (LV2_URID_Map*) features[i]->data;
        }
      else if (HAVE_FEATURE (LV2_LOG__log))
        {
          self->log =
            (LV2_Log_Log *) features[i]->data;
        }
    }
#undef HAVE_FEATURE

  if (!self->map)
    {
      fprintf (stderr, "Missing feature urid:map\n");
      return NULL;
    }

  /* map uris */
  map_uris (self->map, &self->uris);

  lv2_atom_forge_init (
    &self->forge, self->map);

  return (LV2_Handle) self;
}

static void
connect_port (
  LV2_Handle instance,
  uint32_t   port,
  void *     data)
{
  ZLFO * self = (ZLFO*) instance;

  switch ((PortIndex) port)
    {
    case LFO_CONTROL:
      self->control =
        (const LV2_Atom_Sequence *) data;
      break;
    case LFO_NOTIFY:
      self->notify =
        (LV2_Atom_Sequence *) data;
      break;
    case LFO_FREQ:
      self->freq = (const float *) data;
      break;
    case LFO_PHASE:
      self->phase = (const float *) data;
      break;
    case LFO_SINE:
      self->sine = (float *) data;
      break;
    case LFO_SAW_UP:
      self->saw_up = (float *) data;
      break;
    case LFO_SAW_DOWN:
      self->saw_down = (float *) data;
      break;
    case LFO_TRIANGLE:
      self->triangle = (float *) data;
      break;
    case LFO_SQUARE:
      self->square = (float *) data;
      break;
    case LFO_RANDOM:
      self->random = (float *) data;
      break;
    default:
      break;
    }
}

static void
activate (
  LV2_Handle instance)
{
  /*ZLFO * self = (ZLFO*) instance;*/
}

static void
recalc_multipliers (
  ZLFO * self)
{
  /*
   * F = frequency
   * X = samples processed
   * SR = sample rate
   *
   * First, get the radians.
   * ? radians =
   *   (2 * PI) radians per LFO cycle *
   *   F cycles per second *
   *   1 / SR samples per second *
   *   X samples
   *
   * Then the LFO value is the sine of (radians % (2 * PI)).
   *
   * This multiplier handles the part known by now and the
   * first part of the calculation should become:
   * ? radians = X samples * sine_multiplier
   */
  self->sine_multiplier =
    (*self->freq / (float) self->samplerate) * 2.f * PI;

  /*
   * F = frequency
   * X = samples processed
   * SR = sample rate
   *
   * First, get the value.
   * ? value =
   *   (1 value per LFO cycle *
   *    F cycles per second *
   *    1 / SR samples per second *
   *    X samples) % 1
   *
   * Then the LFO value is value * 2 - 1 (to make
   * it start from -1 and end at 1).
   *
   * This multiplier handles the part known by now and the
   * first part of the calculation should become:
   * ? value = ((X samples * saw_multiplier) % 1) * 2 - 1
   */
  self->saw_up_multiplier =
    (*self->freq / (float) self->samplerate);
}

static void
run (
  LV2_Handle instance,
  uint32_t n_samples)
{
  ZLFO * self = (ZLFO *) instance;

  /* if first run or freq changed, set the multipliers */
  if (self->samples_processed == 0 ||
      fabsf (self->last_freq - *self->freq) > 0.0001f)
    {
      recalc_multipliers (self);
    }

  for (uint32_t i = 0; i < n_samples; i++)
    {
      /* handle sine */
      self->sine[i] =
        sinf (
          ((float) self->samples_processed *
              self->sine_multiplier));

      /* handle saw up */
      self->saw_up[i] =
        fmodf (
          (float) self->samples_processed *
            self->saw_up_multiplier, 1.f) * 2.f - 1.f;

      /* saw down is the opposite of saw up */
      self->saw_down[i] = - self->saw_up[i];

      /* triangle can be calculated based on the saw */
      if (self->saw_up[i] < 0.f)
        self->triangle[i] =
          (self->saw_up[i] + 1.f) * 2.f - 1.f;
      else
        self->triangle[i] =
          (self->saw_down[i] + 1.f) * 2.f - 1.f;

      /* square too */
      self->square[i] =
        self->saw_up[i] < 0.f ? -1.f : 1.f;

      /* TODO */
      self->random[i] = 0.f;
      /*self->random[i] =*/
        /*((float) rand () / (float) ((float) RAND_MAX / 2.f)) -*/
          /*1.f;*/

      self->samples_processed++;
    }

  /* remember frequency */
  self->last_freq = *self->freq;
}

static void
deactivate (
  LV2_Handle instance)
{
}

static void
cleanup (
  LV2_Handle instance)
{
  free (instance);
}

static const void*
extension_data (
  const char* uri)
{
  return NULL;
}

static const LV2_Descriptor descriptor = {
  LFO_URI,
  instantiate,
  connect_port,
  activate,
  run,
  deactivate,
  cleanup,
  extension_data
};

LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor (
  uint32_t index)
{
  switch (index)
    {
    case 0:
      return &descriptor;
    default:
      return NULL;
    }
}
