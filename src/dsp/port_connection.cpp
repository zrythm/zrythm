// SPDX-FileCopyrightText: © 2021-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/port_connection.h"

namespace zrythm::dsp
{

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
  assert (!src.is_null ());
  assert (!dest.is_null ());
}

void
init_from (
  PortConnection        &obj,
  const PortConnection  &other,
  utils::ObjectCloneType clone_type)
{
  obj.src_id_ = other.src_id_;
  obj.dest_id_ = other.dest_id_;
  obj.multiplier_ = other.multiplier_;
  obj.locked_ = other.locked_;
  obj.enabled_ = other.enabled_;
  obj.base_value_ = other.base_value_;
}

}
