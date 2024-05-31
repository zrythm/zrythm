// clang-format off
// SPDX-FileCopyrightText: © 2018-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

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
  plugin_identifier_init (&this->plugin_id_);
}

int
PortIdentifier::port_group_cmp (const void * p1, const void * p2)
{
  const Port * control1 = *(const Port **) p1;
  const Port * control2 = *(const Port **) p2;

  g_return_val_if_fail (IS_PORT (control1), -1);
  g_return_val_if_fail (IS_PORT (control2), -1);

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

bool
PortIdentifier::is_equal (const PortIdentifier &other) const
{
  bool eq =
    owner_type_ == other.owner_type_ && unit_ == other.unit_
    && type_ == other.type_ && flow_ == other.flow_ && flags_ == other.flags_
    && flags2_ == other.flags2_ && track_name_hash_ == other.track_name_hash_;
  if (!eq)
    return false;

  if (owner_type_ == PortIdentifier::OwnerType::PLUGIN)
    {
      eq = eq && plugin_identifier_is_equal (&plugin_id_, &other.plugin_id_);
    }

  /* if LV2 (has symbol) check symbol match, other.wise check index match and
   * label match */
  if (sym_.empty ())
    {
      eq = eq && port_index_ == other.port_index_ && label_ == other.label_;
    }
  else
    {
      eq = eq && sym_ == other.sym_;
    }

  /* do string comparisons at the end */
  eq =
    eq && uri_ == other.uri_ && port_group_ == other.port_group_
    && ext_port_id_ == other.ext_port_id_;

  return eq;
}

/**
 * To be used as GEqualFunc.
 */
int
PortIdentifier::is_equal_func (const void * a, const void * b)
{
  auto p1 = static_cast<const PortIdentifier *> (a);
  auto p2 = static_cast<const PortIdentifier *> (b);
  bool is_equal = p1->is_equal (*p2);
  return is_equal;
}

void
PortIdentifier::print_to_str (char * buf, size_t buf_sz) const
{
  char pl_buf[800];
  strcpy (pl_buf, "{none}");
  if (owner_type_ == PortIdentifier::OwnerType::PLUGIN)
    {
      plugin_identifier_print (&plugin_id_, pl_buf);
    }
  snprintf (
    buf, buf_sz,
    "[PortIdentifier %p | hash %u]\nlabel: %s\n"
    "sym: %s\nuri: %s\ncomment: %s\nowner type: %s\n"
    "type: %s\nflow: %s\nflags: %s %s\nunit: %s\n"
    "port group: %s\next port id: %s\n"
    "track name hash: %u\nport idx: %d\nplugin: %s",
    this, get_hash (), label_.c_str (), sym_.c_str (), uri_.c_str (),
    comment_.c_str (), ENUM_NAME (owner_type_), ENUM_NAME (type_),
    ENUM_NAME (flow_), ENUM_BITSET_TO_STRING (PortIdentifier::Flags, flags_),
    ENUM_BITSET_TO_STRING (PortIdentifier::Flags2, flags2_), ENUM_NAME (unit_),
    port_group_.c_str (), ext_port_id_.c_str (), track_name_hash_, port_index_,
    pl_buf);
}

void
PortIdentifier::print () const
{
  size_t size = 2000;
  char   buf[size];
  print_to_str (buf, size);
  g_message ("%s", buf);
}

bool
PortIdentifier::validate () const
{
  g_return_val_if_fail (
    /*self->schema_version == PORT_IDENTIFIER_SCHEMA_VERSION*/
    plugin_identifier_validate (&this->plugin_id_), false);
  return true;
}

OPTIMIZE_O3
uint32_t
PortIdentifier::get_hash () const
{
  const PortIdentifier * id = static_cast<const PortIdentifier *> (this);
  uint32_t               hash = 0;
  if (!id->sym_.empty ())
    {
      hash = hash ^ g_str_hash (id->sym_.c_str ());
    }
  /* don't check label when have symbol because label might be
   * localized */
  else if (!id->label_.empty ())
    {
      hash = hash ^ g_str_hash (id->label_.c_str ());
    }

  if (!id->uri_.empty ())
    hash = hash ^ g_str_hash (id->uri_.c_str ());
  hash = hash ^ g_int_hash (&id->owner_type_);
  hash = hash ^ g_int_hash (&id->type_);
  hash = hash ^ g_int_hash (&id->flow_);
  hash = hash ^ g_int_hash (&id->flags_);
  hash = hash ^ g_int_hash (&id->flags2_);
  hash = hash ^ g_int_hash (&id->unit_);
  hash = hash ^ plugin_identifier_get_hash (&id->plugin_id_);
  if (!id->port_group_.empty ())
    hash = hash ^ g_str_hash (id->port_group_.c_str ());
  if (!id->ext_port_id_.empty ())
    hash = hash ^ g_str_hash (id->ext_port_id_.c_str ());
  hash = hash ^ id->track_name_hash_;
  hash = hash ^ g_int_hash (&id->port_index_);
  return hash;
}

PortIdentifier::PortIdentifier (const PortIdentifier &other)
{
  *this = other;
}

PortIdentifier::~PortIdentifier () { }

const char *
PortIdentifier::get_label (void * data)
{
  PortIdentifier * self = static_cast<PortIdentifier *> (data);
  return self->get_label ().c_str ();
}

void
PortIdentifier::destroy_notify (void * data)
{
  PortIdentifier * self = static_cast<PortIdentifier *> (data);
  delete self;
}

uint32_t
PortIdentifier::get_hash (const void * data)
{
  const PortIdentifier * self = static_cast<const PortIdentifier *> (data);
  return self->get_hash ();
}