// SPDX-FileCopyrightText: © 2021-2022,2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_PORT_CONNECTION_H__
#define __AUDIO_PORT_CONNECTION_H__

#include "zrythm-config.h"

#include "dsp/port_identifier.h"
#include "utils/math.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * A connection between two ports.
 */
class PortConnection final : public ISerializable<PortConnection>
{
public:
  PortConnection () = default;
  PortConnection (
    PortIdentifier src,
    PortIdentifier dest,
    float          multiplier,
    bool           locked,
    bool           enabled)
      : src_id_ (std::move (src)), dest_id_ (std::move (dest)),
        multiplier_ (multiplier), locked_ (locked), enabled_ (enabled)
  {
  }

  void update (float multiplier, bool locked, bool enabled)
  {
    multiplier_ = multiplier;
    locked_ = locked;
    enabled_ = enabled;
  }

  bool is_send () const;

  std::string print_to_str () const;

  void print () const;

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  PortIdentifier src_id_ = {};
  PortIdentifier dest_id_ = {};

  /**
   * Multiplier to apply, where applicable.
   *
   * Range: 0 to 1.
   * Default: 1.
   */
  float multiplier_ = 1.0f;

  /**
   * Whether the connection can be removed or the multiplier edited by the user.
   *
   * Ignored when connecting things internally and only used to deter the user
   * from breaking necessary connections.
   */
  bool locked_ = false;

  /**
   * Whether the connection is enabled.
   *
   * @note The user can disable port connections only if they are not locked.
   */
  bool enabled_ = true;

  /** Used for CV -> control port connections. */
  float base_value_ = 0.0f;
};

inline bool
operator== (const PortConnection &lhs, const PortConnection &rhs)
{
  return lhs.src_id_ == rhs.src_id_ && lhs.dest_id_ == rhs.dest_id_
         && math_floats_equal (lhs.multiplier_, rhs.multiplier_)
         && lhs.locked_ == rhs.locked_ && lhs.enabled_ == rhs.enabled_
         && math_floats_equal (lhs.base_value_, rhs.base_value_);
}

/**
 * @}
 */

#endif /* __AUDIO_PORT_CONNECTION_H__ */
