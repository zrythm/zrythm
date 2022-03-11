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
#    include <stdbool.h>
#endif

#include "lv2/atom/atom.h"
#include "lv2/atom/util.h"
#include "lv2/core/lv2.h"
#include "lv2/core/lv2_util.h"
#include "lv2/log/log.h"
#include "lv2/log/logger.h"
#include "lv2/midi/midi.h"
#include "lv2/urid/urid.h"

#include "./uris.h"

enum {
  FIFTHS_IN  = 0,
  FIFTHS_OUT = 1
};

typedef struct {
  // Features
  LV2_URID_Map*  map;
  LV2_Log_Logger logger;

  // Ports
  const LV2_Atom_Sequence* in_port;
  float *       out_port;

  // URIs
  FifthsURIs uris;
} Fifths;

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
  Fifths* self = (Fifths*)instance;
  switch (port) {
  case FIFTHS_IN:
    self->in_port = (const LV2_Atom_Sequence*)data;
    break;
  case FIFTHS_OUT:
    self->out_port = (float *) data;
    break;
  default:
    break;
  }
}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               path,
            const LV2_Feature* const* features)
{
  // Allocate and initialise instance structure.
  Fifths* self = (Fifths*)calloc(1, sizeof(Fifths));
  if (!self) {
    return NULL;
  }

  // Scan host features for URID map
  const char*  missing = lv2_features_query(
    features,
    LV2_LOG__log,  &self->logger.log, false,
    LV2_URID__map, &self->map,        true,
    NULL);
  lv2_log_logger_set_map(&self->logger, self->map);
  if (missing) {
    lv2_log_error(&self->logger, "Missing feature <%s>\n", missing);
    free(self);
    return NULL;
  }

  map_fifths_uris(self->map, &self->uris);

  return (LV2_Handle)self;
}

static void
cleanup(LV2_Handle instance)
{
  free(instance);
}

static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
  Fifths*     self = (Fifths*)instance;

  memset (
    self->out_port, 0, sample_count * sizeof (float));
}

static const void*
extension_data(const char* uri)
{
  return NULL;
}

static const LV2_Descriptor descriptor = {
  EG_FIFTHS_URI,
  instantiate,
  connect_port,
  NULL,  // activate,
  run,
  NULL,  // deactivate,
  cleanup,
  extension_data
};

LV2_SYMBOL_EXPORT
const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
  switch (index) {
  case 0:
    return &descriptor;
  default:
    return NULL;
  }
}
