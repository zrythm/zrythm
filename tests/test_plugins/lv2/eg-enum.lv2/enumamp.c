// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/* Amplifier with an enumerated dB gain port, for testing that the host
 * feeds scale point values (not enumeration indices) to the port. */

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "lv2/core/lv2.h"

#define ENUMAMP_URI "http://lv2plug.in/plugins/eg-enum"

typedef enum
{
  GAIN = 0,
  INPUT = 1,
  OUTPUT = 2
} PortIndex;

typedef struct
{
  const float * gain;
  const float * input;
  float *       output;
} EnumAmp;

static LV2_Handle
instantiate (
  const LV2_Descriptor *      descriptor,
  double                      rate,
  const char *                bundle_path,
  const LV2_Feature * const * features)
{
  EnumAmp * amp = (EnumAmp *) calloc (1, sizeof (EnumAmp));

  return (LV2_Handle) amp;
}

static void
connect_port (LV2_Handle instance, uint32_t port, void * data)
{
  EnumAmp * amp = (EnumAmp *) instance;

  switch ((PortIndex) port)
    {
    case GAIN:
      amp->gain = (const float *) data;
      break;
    case INPUT:
      amp->input = (const float *) data;
      break;
    case OUTPUT:
      amp->output = (float *) data;
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
  const EnumAmp * amp = (const EnumAmp *) instance;

  const float coef = db_to_linear (*(amp->gain));

  for (uint32_t pos = 0; pos < n_samples; pos++)
    {
      amp->output[pos] = amp->input[pos] * coef;
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
  ENUMAMP_URI, instantiate, connect_port, activate,
  run,         deactivate,  cleanup,      extension_data
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
