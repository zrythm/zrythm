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

#include "io/serialization/iserializable.h"
#include "utils/format.h"
#include "utils/logger.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * Type of Region.
 *
 * Bitfield instead of plain enum so multiple values can be passed to some
 * functions (eg to collect all Regions of the given types in a Track).
 */
enum class RegionType
{
  Midi = 1 << 0,
  Audio = 1 << 1,
  Automation = 1 << 2,
  Chord = 1 << 3,
};

DEFINE_ENUM_FORMATTER (
  RegionType,
  RegionType,
  N_ ("MIDI"),
  N_ ("Audio"),
  N_ ("Automation"),
  N_ ("Chord"));

/**
 * Index/identifier for a Region, so we can get Region objects quickly with it
 * without searching by name.
 */
class RegionIdentifier : public ISerializable<RegionIdentifier>
{
public:
  RegionIdentifier () = default;
  RegionIdentifier (RegionType type) : type_ (type) { }
  bool validate () const { /* TODO? */ return true; };

  inline bool is_automation () const { return type_ == RegionType::Automation; }
  inline bool is_midi () const { return type_ == RegionType::Midi; }
  inline bool is_audio () const { return type_ == RegionType::Audio; }
  inline bool is_chord () const { return type_ == RegionType::Chord; }

  void print () const
  {
    z_debug (
      "Region identifier: type: %s, track name hash %u, lane pos %d, at index %d, index %d, link_group: %d",
      RegionType_to_string (type_), track_name_hash_, lane_pos_, at_idx_, idx_,
      link_group_);
  }

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

/**
 * @}
 */

#endif
