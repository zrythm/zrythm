// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/* A CV-only plugin whose default state carries the CV input's default
 * value, like VM CV does. */

#include <stdint.h>
#include <stdlib.h>

#include "lv2/core/lv2.h"

#define CV_STATE_URI "https://lv2.zrythm.org/cv-state"

typedef struct
{
  const float * in;
  float *       out;
} CvState;

static LV2_Handle
instantiate (
  const LV2_Descriptor *      descriptor,
  double                      rate,
  const char *                bundle_path,
  const LV2_Feature * const * features)
{
  (void) descriptor;
  (void) rate;
  (void) bundle_path;
  (void) features;
  return (LV2_Handle) calloc (1, sizeof (CvState));
}

static void
connect_port (LV2_Handle instance, uint32_t port, void * data)
{
  CvState * self = (CvState *) instance;
  if (port == 0)
    self->in = (const float *) data;
  else if (port == 1)
    self->out = (float *) data;
}

static void
activate (LV2_Handle instance)
{
  (void) instance;
}

static void
run (LV2_Handle instance, uint32_t n_samples)
{
  CvState * self = (CvState *) instance;
  if (self->out == NULL)
    return;

  for (uint32_t i = 0; i < n_samples; ++i)
    self->out[i] = self->in != NULL ? self->in[i] : 0.0f;
}

static void
deactivate (LV2_Handle instance)
{
  (void) instance;
}

static void
cleanup (LV2_Handle instance)
{
  free (instance);
}

static const LV2_Descriptor cv_state_descriptor = {
  CV_STATE_URI, instantiate, connect_port, activate,
  run,          deactivate,  cleanup,       NULL
};

LV2_SYMBOL_EXPORT
const LV2_Descriptor *
lv2_descriptor (uint32_t index)
{
  return index == 0 ? &cv_state_descriptor : NULL;
}
