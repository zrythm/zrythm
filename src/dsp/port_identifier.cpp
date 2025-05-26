// SPDX-FileCopyrightText: © 2018-2021, 2023-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/port_identifier.h"

namespace zrythm::dsp
{

utils::Utf8String
PortIdentifier::port_unit_to_string (PortUnit unit)
{
  constexpr std::array<std::u8string_view, 8> port_unit_strings = {
    u8"none", u8"Hz", u8"MHz", u8"dB", u8"°", u8"s", u8"ms", u8"μs",
  };
  return port_unit_strings.at (ENUM_VALUE_TO_INT (unit));
}

#if 0
void
PortIdentifier::init ()
{
  plugin_id_ = {};
}
#endif

std::string
PortIdentifier::print_to_str () const
{
  return fmt::format (
    "[PortIdentifier {} | hash {}]\nlabel: {}\n"
    "sym: {}\nuri: {}\ncomment: {}\nowner type: {}\n"
    "type: {}\nflow: {}\nflags: {} {}\nunit: {}\n"
    "port group: {}\next port id: {}\n"
    "track name hash: {}\nport idx: {}\nplugin: {}",
    fmt::ptr (this), get_hash (), label_, sym_, uri_, comment_,
    ENUM_NAME (owner_type_), ENUM_NAME (type_), ENUM_NAME (flow_),
    ENUM_BITSET_TO_STRING (PortIdentifier::Flags, flags_),
    ENUM_BITSET_TO_STRING (PortIdentifier::Flags2, flags2_), ENUM_NAME (unit_),
    port_group_, ext_port_id_, track_id_, port_index_, plugin_id_);
}

void
PortIdentifier::print () const
{
  z_info (print_to_str ());
}

bool
PortIdentifier::validate () const
{
  return true;
}

size_t
PortIdentifier::get_hash () const
{
  // FIXME this hashing is very very weak, there are guaranteed many collisions
  // on every project... we need a better hashing style
  // same goes for other classes too (like PluginIdentifier)
  // TODO look into std::hash and provide a hash_combine helper
  size_t hash{};
  if (!sym_.empty ())
    {
      hash = hash ^ qHash (sym_);
    }
  /* don't check label when have symbol because label might be
   * localized */
  else if (!label_.empty ())
    {
      hash = hash ^ qHash (label_);
    }

  if (uri_)
    hash = hash ^ qHash (*uri_);
  hash = hash ^ qHash (owner_type_);
  hash = hash ^ qHash (type_);
  hash = hash ^ qHash (flow_);
  hash = hash ^ qHash (flags_);
  hash = hash ^ qHash (flags2_);
  hash = hash ^ qHash (unit_);
  if (plugin_id_.has_value ())
    hash = hash ^ qHash (type_safe::get (plugin_id_.value ()));
  if (port_group_)
    hash = hash ^ qHash (*port_group_);
  if (ext_port_id_)
    hash = hash ^ qHash (*ext_port_id_);
  if (track_id_.has_value ())
    hash = hash ^ qHash (type_safe::get (track_id_.value ()));
  hash = hash ^ qHash (port_index_);
  // z_debug ("hash for {}: {}", sym_, hash);
  return hash;
}

bool
operator== (const PortIdentifier &lhs, const PortIdentifier &rhs)
{
  bool eq =
    lhs.owner_type_ == rhs.owner_type_ && lhs.unit_ == rhs.unit_
    && lhs.type_ == rhs.type_ && lhs.flow_ == rhs.flow_
    && lhs.flags_ == rhs.flags_ && lhs.flags2_ == rhs.flags2_
    && lhs.track_id_ == rhs.track_id_;
  if (!eq)
    return false;

  if (lhs.owner_type_ == PortIdentifier::OwnerType::Plugin)
    {
      eq = eq && lhs.plugin_id_ == rhs.plugin_id_;
    }
  if (!eq)
    return false;

  /* if LV2 (has symbol) check symbol match, otherwise check index match and
   * label match */
  if (lhs.sym_.empty ())
    {
      eq = eq && lhs.port_index_ == rhs.port_index_ && lhs.label_ == rhs.label_;
    }
  else
    {
      eq = eq && lhs.sym_ == rhs.sym_;
    }
  if (!eq)
    return false;

  /* do string comparisons at the end */
  eq =
    eq && lhs.uri_ == rhs.uri_ && lhs.port_group_ == rhs.port_group_
    && lhs.ext_port_id_ == rhs.ext_port_id_;

  return eq;
}

}; // namespace zrythm::dsp
