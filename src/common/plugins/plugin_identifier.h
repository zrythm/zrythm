// SPDX-FileCopyrightText: © 2020-2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __PLUGINS_PLUGIN_IDENTIFIER_H__
#define __PLUGINS_PLUGIN_IDENTIFIER_H__

#include "zrythm-config.h"

#include <cstdint>

#include "common/io/serialization/iserializable.h"

/**
 * @addtogroup plugins
 *
 * @{
 */

enum class PluginSlotType
{
  Invalid,
  Insert,
  MidiFx,
  Instrument,

  /** Plugin is part of a modulator. */
  Modulator,
};

/**
 * Plugin identifier.
 */
class PluginIdentifier final : public ISerializable<PluginIdentifier>
{
public:
  // Rule of 0

  bool validate () const;

  /**
   * Verifies that @p slot_type and @p slot is a valid combination.
   */
  static inline bool
  validate_slot_type_slot_combo (PluginSlotType slot_type, int slot)
  {
    return (slot_type == PluginSlotType::Instrument && slot == -1)
           || (slot_type == PluginSlotType::Invalid && slot == -1)
           || (slot_type != PluginSlotType::Instrument && slot >= 0);
  }

  std::string print_to_str () const;

  uint32_t get_hash () const;

  static uint32_t get_hash (const void * id)
  {
    return static_cast<const PluginIdentifier *> (id)->get_hash ();
  }

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  PluginSlotType slot_type_ = PluginSlotType::Invalid;

  /** Track name hash. */
  unsigned int track_name_hash_ = 0;

  /**
   * The slot this plugin is in the channel, or the index if this is part of a
   * modulator.
   *
   * If PluginIdentifier.slot_type is an instrument, this must be set to -1.
   */
  int slot_ = -1;
};

inline bool
operator== (const PluginIdentifier &lhs, const PluginIdentifier &rhs)
{
  return lhs.slot_type_ == rhs.slot_type_
         && lhs.track_name_hash_ == rhs.track_name_hash_ && lhs.slot_ == rhs.slot_;
}

inline bool
operator< (const PluginIdentifier &lhs, const PluginIdentifier &rhs)
{
  return lhs.slot_ < rhs.slot_;
}

/**
 * @}
 */

#endif