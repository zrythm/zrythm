// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Region identifier.
 *
 * This is in its own file to avoid recursive inclusion.
 */

#ifndef __AUDIO_REGION_IDENTIFIER_H__
#define __AUDIO_REGION_IDENTIFIER_H__

#include "utils/format.h"
#include "utils/iserializable.h"
#include "utils/logger.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * Type of Region.
 */
enum class RegionType
{
  Midi,
  Audio,
  Automation,
  Chord,
};

DEFINE_ENUM_FORMATTER (
  RegionType,
  RegionType,
  QT_TR_NOOP_UTF8 ("MIDI"),
  QT_TR_NOOP_UTF8 ("Audio"),
  QT_TR_NOOP_UTF8 ("Automation"),
  QT_TR_NOOP_UTF8 ("Chord"));

/**
 * Index/identifier for a Region, so we can get Region objects quickly with it
 * without searching by name.
 */
class RegionIdentifier
    : public zrythm::utils::serialization::ISerializable<RegionIdentifier>
{
public:
  RegionIdentifier () = default;
  RegionIdentifier (RegionType type) : type_ (type) { }
  bool validate () const;

  bool is_automation () const { return type_ == RegionType::Automation; }
  bool is_midi () const { return type_ == RegionType::Midi; }
  bool is_audio () const { return type_ == RegionType::Audio; }
  bool is_chord () const { return type_ == RegionType::Chord; }

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  RegionType type_ = (RegionType) 0;

  /** Link group index, if any, or -1. */
  int link_group_ = -1;

  unsigned int track_name_hash_ = 0;
  int          lane_pos_ = 0;

  /** Automation track index in the automation tracklist, if automation region. */
  int at_idx_ = 0;

  /** Index inside lane or automation track. */
  int idx_ = 0;
};

inline bool
operator== (const RegionIdentifier &lhs, const RegionIdentifier &rhs)
{
  return lhs.type_ == rhs.type_ && lhs.link_group_ == rhs.link_group_
         && lhs.track_name_hash_ == rhs.track_name_hash_
         && lhs.lane_pos_ == rhs.lane_pos_ && lhs.at_idx_ == rhs.at_idx_
         && lhs.idx_ == rhs.idx_;
}

DEFINE_OBJECT_FORMATTER (
  RegionIdentifier,
  RegionIdentifier,
  [] (const RegionIdentifier &id) {
    return fmt::format (
      "RegionIdentifier {{ type: {}, track name hash {}, lane pos {}, at index {}, index {}, link_group: {} }}",
      RegionType_to_string (id.type_), id.track_name_hash_, id.lane_pos_,
      id.at_idx_, id.idx_, id.link_group_);
  });

/**
 * @}
 */

#endif
