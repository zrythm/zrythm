// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/* Amplifier with valid DSP whose data file is malformed (see broken.ttl),
 * for testing that the host refuses to load it without leaving partially
 * initialized state behind. */

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "lv2/core/lv2.h"

#define BROKEN_URI "https://lv2.zrythm.org/test-plugins/broken"

typedef enum
{
  GAIN = 0,
  INPUT = 2,
  OUTPUT = 3
} PortIndex;

typedef struct
{
  const float * gain;
  const float * input;
  float *       output;
} Broken;

static LV2_Handle
instantiate (
  const LV2_Descriptor *      descriptor,
  double                      rate,
  const char *                bundle_path,
  const LV2_Feature * const * features)
{
  Broken * plugin = (Broken *) calloc (1, sizeof (Broken));

  return (LV2_Handle) plugin;
}

static void
connect_port (LV2_Handle instance, uint32_t port, void * data)
{
  Broken * plugin = (Broken *) instance;

  switch ((PortIndex) port)
    {
    case GAIN:
      plugin->gain = (const float *) data;
      break;
    case INPUT:
      plugin->input = (const float *) data;
      break;
    case OUTPUT:
      plugin->output = (float *) data;
      break;
    }
}

static void
activate (LV2_Handle instance)
{
}

/** Linear gain factor of a dB value (silence below -90 dB). */
static float
db_to_linear (float db)
{
  return db > -90.0f ? powf (10.0f, db / 20.0f) : 0.0f;
}

static void
run (LV2_Handle instance, uint32_t n_samples)
{
  const Broken * plugin = (const Broken *) instance;

  const float coef = db_to_linear (*(plugin->gain));

  for (uint32_t pos = 0; pos < n_samples; pos++)
    {
      plugin->output[pos] = plugin->input[pos] * coef;
    }
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

static const LV2_Descriptor descriptor = {
  BROKEN_URI, instantiate, connect_port, activate,
  run,        deactivate,  cleanup,      extension_data
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
