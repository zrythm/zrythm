// SPDX-FileCopyrightText: © 2018-2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/plugin_identifier.h"
#include "dsp/port_identifier.h"

namespace zrythm::dsp
{

std::string
PortIdentifier::port_unit_to_string (PortUnit unit)
{
  static const char * port_unit_strings[] = {
    "none", "Hz", "MHz", "dB", "°", "s", "ms", "μs",
  };
  return port_unit_strings[ENUM_VALUE_TO_INT (unit)];
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
    port_group_, ext_port_id_, track_name_hash_, port_index_,
    owner_type_ == OwnerType::Plugin ? plugin_id_.print_to_str () : "{none}");
}

void
PortIdentifier::print () const
{
  z_info (print_to_str ());
}

bool
PortIdentifier::validate () const
{
  z_return_val_if_fail (this->plugin_id_.validate (), false);
  return true;
}

uint32_t
PortIdentifier::get_hash () const
{
  // FIXME this hashing is very very weak, there are guaranteed many collisions
  // on every project... we need a better hashing style
  // same goes for other classes too (like PluginIdentifier)
  // TODO look into std::hash and provide a hash_combine helper
  uint32_t hash = 0;
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

  if (!uri_.empty ())
    hash = hash ^ qHash (uri_);
  hash = hash ^ qHash (owner_type_);
  hash = hash ^ qHash (type_);
  hash = hash ^ qHash (flow_);
  hash = hash ^ qHash (flags_);
  hash = hash ^ qHash (flags2_);
  hash = hash ^ qHash (unit_);
  hash = hash ^ plugin_id_.get_hash ();
  if (!port_group_.empty ())
    hash = hash ^ qHash (port_group_);
  if (!ext_port_id_.empty ())
    hash = hash ^ qHash (ext_port_id_);
  hash = hash ^ track_name_hash_;
  hash = hash ^ qHash (port_index_);
  // z_trace ("hash for {}: {}", sym_, hash);
  return hash;
}

void
PortIdentifier::define_fields (const Context &ctx)
{
  serialize_fields (
    ctx, make_field ("label", label_, true), make_field ("symbol", sym_, true),
    make_field ("uri", uri_, true), make_field ("comment", comment_, true),
    make_field ("ownerType", owner_type_), make_field ("type", type_),
    make_field ("flow", flow_), make_field ("unit", unit_),
    make_field ("flags", flags_), make_field ("flags2", flags2_),
    make_field ("trackNameHash", track_name_hash_),
    make_field ("pluginId", plugin_id_),
    make_field ("portGroup", port_group_, true),
    make_field ("externalPortId", ext_port_id_, true),
    make_field ("portIndex", port_index_));
}

bool
operator== (const PortIdentifier &lhs, const PortIdentifier &rhs)
{
  bool eq =
    lhs.owner_type_ == rhs.owner_type_ && lhs.unit_ == rhs.unit_
    && lhs.type_ == rhs.type_ && lhs.flow_ == rhs.flow_
    && lhs.flags_ == rhs.flags_ && lhs.flags2_ == rhs.flags2_
    && lhs.track_name_hash_ == rhs.track_name_hash_;
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
