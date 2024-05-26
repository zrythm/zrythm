// clang-format off
// SPDX-FileCopyrightText: © 2020-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

/**
 * \file
 *
 * Plugin identifier.
 */

#ifndef __PLUGINS_PLUGIN_IDENTIFIER_H__
#define __PLUGINS_PLUGIN_IDENTIFIER_H__

#include "zrythm-config.h"

#include <cstddef>
#include <cstdint>

/**
 * @addtogroup plugins
 *
 * @{
 */

enum class ZPluginSlotType
{
  Z_PLUGIN_SLOT_INVALID,
  Z_PLUGIN_SLOT_INSERT,
  Z_PLUGIN_SLOT_MIDI_FX,
  Z_PLUGIN_SLOT_INSTRUMENT,

  /** Plugin is part of a modulator. */
  Z_PLUGIN_SLOT_MODULATOR,
};

/**
 * Plugin identifier.
 */
typedef struct PluginIdentifier
{
  ZPluginSlotType slot_type;

  /** Track name hash. */
  unsigned int track_name_hash;

  /**
   * The slot this plugin is in the channel, or
   * the index if this is part of a modulator.
   *
   * If PluginIdentifier.slot_type is an instrument,
   * this must be set to -1.
   */
  int slot;
} PluginIdentifier;

void
plugin_identifier_init (PluginIdentifier * self);

static inline int
plugin_identifier_is_equal (
  const PluginIdentifier * a,
  const PluginIdentifier * b)
{
  return a->slot_type == b->slot_type
         && a->track_name_hash == b->track_name_hash && a->slot == b->slot;
}

void
plugin_identifier_copy (PluginIdentifier * dest, const PluginIdentifier * src);

NONNULL bool
plugin_identifier_validate (const PluginIdentifier * self);

/**
 * Verifies that @ref slot_type and @ref slot is
 * a valid combination.
 */
bool
plugin_identifier_validate_slot_type_slot_combo (
  ZPluginSlotType slot_type,
  int             slot);

void
plugin_identifier_print (const PluginIdentifier * self, char * str);

uint32_t
plugin_identifier_get_hash (const void * id);

/**
 * @}
 */

#endif
