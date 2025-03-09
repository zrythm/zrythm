// SPDX-FileCopyrightText: © 2021-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/dsp/port_connection.h"

PortConnection::PortConnection (QObject * parent) : QObject (parent) { }

PortConnection::PortConnection (
  const PortUuid &src,
  const PortUuid &dest,
  float           multiplier,
  bool            locked,
  bool            enabled,
  QObject *       parent)
    : QObject (parent), src_id_ (src), dest_id_ (dest),
      multiplier_ (multiplier), locked_ (locked), enabled_ (enabled)
{
}

void
PortConnection::init_after_cloning (
  const PortConnection &other,
  ObjectCloneType       clone_type)
{
  src_id_ = other.src_id_;
  dest_id_ = other.dest_id_;
  multiplier_ = other.multiplier_;
  locked_ = other.locked_;
  enabled_ = other.enabled_;
  base_value_ = other.base_value_;
}

bool
operator== (const PortConnection &lhs, const PortConnection &rhs)
{
  return lhs.src_id_ == rhs.src_id_ && lhs.dest_id_ == rhs.dest_id_
         && utils::math::floats_equal (lhs.multiplier_, rhs.multiplier_)
         && lhs.locked_ == rhs.locked_ && lhs.enabled_ == rhs.enabled_
         && utils::math::floats_equal (lhs.base_value_, rhs.base_value_);
}
