// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/transport.h"
#include "utils/objects.h"

#include "schemas/audio/transport.h"

Transport *
transport_upgrade_from_v1 (Transport_v1 * old)
{
  if (!old)
    return NULL;

  Transport * self = object_new (Transport);

  self->schema_version = TRANSPORT_SCHEMA_VERSION;

#define UPDATE_POS(name) \
  Position * name = position_upgrade_from_v1 (&old->name); \
  self->name = *name

#define UPDATE_PORT(name) \
  self->name = port_upgrade_from_v1 (old->name)

#define UPDATE(name) self->name = old->name

  UPDATE (total_bars);
  UPDATE_POS (playhead_pos);
  UPDATE_POS (cue_pos);
  UPDATE_POS (loop_start_pos);
  UPDATE_POS (loop_end_pos);
  UPDATE_POS (punch_in_pos);
  UPDATE_POS (punch_out_pos);
  UPDATE_POS (range_1);
  UPDATE_POS (range_2);
  UPDATE (has_range);
  UPDATE (position);
  UPDATE_PORT (roll);
  UPDATE_PORT (stop);
  UPDATE_PORT (backward);
  UPDATE_PORT (forward);
  UPDATE_PORT (loop_toggle);
  UPDATE_PORT (rec_toggle);

  return self;
}
