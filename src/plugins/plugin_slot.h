// SPDX-FileCopyrightText: © 2020-2021, 2023-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef ZRYTHM_DSP_PLUGIN_IDENTIFIER_H
#define ZRYTHM_DSP_PLUGIN_IDENTIFIER_H

#include "utils/format.h"
#include "utils/serialization.h"
#include "utils/uuid_identifiable_object.h"

namespace zrythm::plugins
{

enum class PluginSlotType
{
  Invalid,
  MidiFx,
  Instrument,
  Insert,

  /** Plugin is part of a modulator. */
  Modulator,
};

class PluginSlot
{
public:
  using SlotNo = std::uint_fast8_t;

  friend auto operator<=> (const PluginSlot &lhs, const PluginSlot &rhs)
  {
    if (lhs.type_ != rhs.type_)
      return lhs.type_ <=> rhs.type_;

    return lhs.slot_ <=> rhs.slot_;
  }

  // Needed for testing
  friend bool operator== (const PluginSlot &lhs, const PluginSlot &rhs)
  {
    return (lhs <=> rhs) == 0;
  }

public:
  PluginSlot () = default;

  /**
   * @brief Construct a new Plugin Slot object for a non-instrument.
   */
  explicit PluginSlot (PluginSlotType type, SlotNo slot)
      : type_ (type), slot_ (slot)
  {
  }

  /**
   * @brief Construct a new Plugin Slot object for an instrument.
   */
  explicit PluginSlot (PluginSlotType type) : type_ (type) { }

  bool has_slot_index () const { return slot_.has_value (); }

  auto get_slot_with_index () const
  {
    if (!has_slot_index ())
      {
        throw std::invalid_argument ("Slot has no index");
      }
    return std::make_pair (type_, slot_.value ());
  }
  auto get_slot_type_only () const
  {
    if (slot_.has_value ())
      {
        throw std::invalid_argument ("Slot has index");
      }
    return type_;
  }

  PluginSlot get_slot_after_n (PluginSlot::SlotNo n) const
  {
    if (!slot_.has_value ())
      {
        throw std::invalid_argument ("Slot has no index");
      }
    return PluginSlot (type_, slot_.value () + n);
  }

  size_t get_hash () const;

  bool is_modulator () const { return type_ == PluginSlotType::Modulator; }

  /**
   * Verifies that slot_type and slot is a valid combination.
   */
  bool validate_slot_type_slot_combo () const
  {
    if (type_ == PluginSlotType::Invalid)
      return false;

    return (type_ == PluginSlotType::Instrument && !slot_.has_value ())
           || (type_ != PluginSlotType::Instrument && slot_.has_value ());
  }

  NLOHMANN_DEFINE_TYPE_INTRUSIVE (PluginSlot, type_, slot_)

private:
  PluginSlotType        type_{ PluginSlotType::Invalid };
  std::optional<SlotNo> slot_;
};

}; // namespace zrythm::dsp

DEFINE_OBJECT_FORMATTER (
  zrythm::plugins::PluginSlot,
  PluginSlot,
  [] (const zrythm::plugins::PluginSlot &slot) {
    if (slot.has_slot_index ())
      {
        auto ret = slot.get_slot_with_index ();
        return fmt::format ("{}::{}", ENUM_NAME (ret.first), ret.second);
      }
    return fmt::format ("{}", ENUM_NAME (slot.get_slot_type_only ()));
  });

#endif
