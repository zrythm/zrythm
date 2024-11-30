// SPDX-FileCopyrightText: © 2020-2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef ZRYTHM_DSP_PLUGIN_IDENTIFIER_H
#define ZRYTHM_DSP_PLUGIN_IDENTIFIER_H

#include "utils/iserializable.h"

namespace zrythm::dsp
{

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
class PluginIdentifier
    : public zrythm::utils::serialization::ISerializable<PluginIdentifier>
{
  friend auto
  operator<=> (const PluginIdentifier &lhs, const PluginIdentifier &rhs)
  {
    return std::tie (lhs.slot_type_, lhs.track_name_hash_, lhs.slot_)
           <=> std::tie (rhs.slot_type_, rhs.track_name_hash_, rhs.slot_);
  }

  // Needed for testing
  friend bool
  operator== (const PluginIdentifier &lhs, const PluginIdentifier &rhs)
  {
    return (lhs <=> rhs) == 0;
  }

public:
  [[nodiscard]] bool validate () const;

  /**
   * Verifies that @p slot_type and @p slot is a valid combination.
   */
  static bool validate_slot_type_slot_combo (PluginSlotType slot_type, int slot)
  {
    return (slot_type == PluginSlotType::Instrument && slot == -1)
           || (slot_type == PluginSlotType::Invalid && slot == -1)
           || (slot_type != PluginSlotType::Instrument && slot >= 0);
  }

  std::string print_to_str () const;

  uint32_t get_hash () const;

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

}; // namespace zrythm::dsp

#endif
