// SPDX-FileCopyrightText: © 2018-2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/port.h"
#include "dsp/port_identifier.h"
#include "plugins/plugin_identifier.h"
#include "utils/types.h"

const char *
port_unit_to_str (const PortUnit unit)
{
  static const char * port_unit_strings[] = {
    "none", "Hz", "MHz", "dB", "°", "s", "ms", "μs",
  };
  return port_unit_strings[ENUM_VALUE_TO_INT (unit)];
}

void
PortIdentifier::init ()
{
  plugin_id_ = PluginIdentifier{};
}

int
PortIdentifier::port_group_cmp (const void * p1, const void * p2)
{
  const Port * control1 = *(const Port **) p1;
  const Port * control2 = *(const Port **) p2;

  z_return_val_if_fail (control1, -1);
  z_return_val_if_fail (control2, -1);

  /* use index for now - this assumes that ports inside port groups are declared
   * in sequence */
  return control1->id_.port_index_ - control2->id_.port_index_;

#if 0
  return
    (control1->id.port_group &&
     control2->id.port_group) ?
      strcmp (
        control1->id.port_group,
        control2->id.port_group) : 0;
#endif
}

#if 0
PortIdentifier &
PortIdentifier::operator= (const PortIdentifier &other)
{
  label_ = other.label_;
  sym_ = other.sym_;
  uri_ = other.uri_;
  comment_ = other.comment_;
  this->owner_type_ = other.owner_type_;
  this->type_ = other.type_;
  this->flow_ = other.flow_;
  this->flags_ = other.flags_;
  this->flags2_ = other.flags2_;
  this->unit_ = other.unit_;
  plugin_identifier_copy (&this->plugin_id_, &other.plugin_id_);
  ext_port_id_ = other.ext_port_id_;
  port_group_ = other.port_group_;
  this->track_name_hash_ = other.track_name_hash_;
  this->port_index_ = other.port_index_;
  return *this;
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

OPTIMIZE_O3
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
      hash = hash ^ g_str_hash (sym_.c_str ());
    }
  /* don't check label when have symbol because label might be
   * localized */
  else if (!label_.empty ())
    {
      hash = hash ^ g_str_hash (label_.c_str ());
    }

  if (!uri_.empty ())
    hash = hash ^ g_str_hash (uri_.c_str ());
  hash = hash ^ g_int_hash (&owner_type_);
  hash = hash ^ g_int_hash (&type_);
  hash = hash ^ g_int_hash (&flow_);
  hash = hash ^ g_int_hash (&flags_);
  hash = hash ^ g_int_hash (&flags2_);
  hash = hash ^ g_int_hash (&unit_);
  hash = hash ^ PluginIdentifier::get_hash (&plugin_id_);
  if (!port_group_.empty ())
    hash = hash ^ g_str_hash (port_group_.c_str ());
  if (!ext_port_id_.empty ())
    hash = hash ^ g_str_hash (ext_port_id_.c_str ());
  hash = hash ^ track_name_hash_;
  hash = hash ^ g_int_hash (&port_index_);
  // z_trace ("hash for {}: {}", sym_, hash);
  return hash;
}