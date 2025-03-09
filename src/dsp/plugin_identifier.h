// SPDX-FileCopyrightText: © 2020-2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef ZRYTHM_DSP_PLUGIN_IDENTIFIER_H
#define ZRYTHM_DSP_PLUGIN_IDENTIFIER_H

#include "utils/format.h"
#include "utils/iserializable.h"
#include "utils/uuid_identifiable_object.h"

class Track;

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

class PluginSlot : public utils::serialization::ISerializable<PluginSlot>
{
public:
  using SlotNo = std::uint_fast8_t;

  friend auto operator<=> (const PluginSlot &lhs, const PluginSlot &rhs)
  {
    return std::tie (lhs.type_, lhs.slot_) <=> std::tie (rhs.type_, rhs.slot_);
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

  uint32_t get_hash () const;

  bool is_modulator () const { return type_ == PluginSlotType::Modulator; }

  /**
   * Verifies that slot_type and slot is a valid combination.
   */
  bool validate_slot_type_slot_combo () const
  {
    return (type_ == PluginSlotType::Instrument && !slot_.has_value ())
           || (type_ == PluginSlotType::Invalid && !slot_.has_value ())
           || (type_ != PluginSlotType::Instrument && slot_.has_value ());
  }

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  PluginSlotType        type_{ PluginSlotType::Invalid };
  std::optional<SlotNo> slot_;
};

/**
 * Plugin identifier.
 */
class PluginIdentifier
    : public zrythm::utils::serialization::ISerializable<PluginIdentifier>
{
public:
  using TrackUuid = utils::UuidIdentifiableObject<Track>::Uuid;

  friend auto
  operator<=> (const PluginIdentifier &lhs, const PluginIdentifier &rhs)
  {
    return std::tie (type_safe::get (lhs.track_id_), lhs.slot_)
           <=> std::tie (type_safe::get (rhs.track_id_), rhs.slot_);
  }

  // Needed for testing
  friend bool
  operator== (const PluginIdentifier &lhs, const PluginIdentifier &rhs)
  {
    return (lhs <=> rhs) == 0;
  }

  [[nodiscard]] bool validate () const;

  std::string print_to_str () const;

  uint32_t get_hash () const;

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  PluginSlot slot_;

  /** ID of the track this plugin belongs to. */
  TrackUuid track_id_;

  static_assert (type_safe::is_strong_typedef<TrackUuid>::value);
  static_assert (StrongTypedef<TrackUuid>);
  static_assert (std::is_same_v<type_safe::underlying_type<TrackUuid>, QUuid>);
  // static_assert (StrongTypedef<PluginSlotType>);
};

}; // namespace zrythm::dsp

DEFINE_OBJECT_FORMATTER (
  zrythm::dsp::PluginSlot,
  PluginSlot,
  [] (const zrythm::dsp::PluginSlot &slot) {
    if (slot.has_slot_index ())
      {
        auto ret = slot.get_slot_with_index ();
        return fmt::format ("{}::{}", ENUM_NAME (ret.first), ret.second);
      }
    return fmt::format ("{}", ENUM_NAME (slot.get_slot_type_only ()));
  });

#endif
