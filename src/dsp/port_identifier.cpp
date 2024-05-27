// clang-format off
// SPDX-FileCopyrightText: © 2018-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

#include "dsp/port.h"
#include "dsp/port_identifier.h"
#include "plugins/plugin_identifier.h"
#include "utils/gtk.h"
#include "utils/hash.h"
#include "utils/objects.h"
#include "utils/string.h"
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
  plugin_identifier_init (&this->plugin_id);
}

/**
 * Port group comparator function where @ref p1 and
 * @ref p2 are pointers to Port.
 */
int
port_identifier_port_group_cmp (const void * p1, const void * p2)
{
  const Port * control1 = *(const Port **) p1;
  const Port * control2 = *(const Port **) p2;

  g_return_val_if_fail (IS_PORT (control1), -1);
  g_return_val_if_fail (IS_PORT (control2), -1);

  /* use index for now - this assumes that ports
   * inside port groups are declared in sequence */
  return control1->id.port_index - control2->id.port_index;

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
  schema_version = other.schema_version;
  string_copy_w_realloc (&label, other.label);
  string_copy_w_realloc (&sym, other.sym);
  string_copy_w_realloc (&uri, other.uri);
  string_copy_w_realloc (&comment, other.comment);
  this->owner_type = other.owner_type;
  this->type = other.type;
  this->flow = other.flow;
  this->flags = other.flags;
  this->flags2 = other.flags2;
  this->unit = other.unit;
  plugin_identifier_copy (&this->plugin_id, &other.plugin_id);
  string_copy_w_realloc (&this->ext_port_id, other.ext_port_id);
  string_copy_w_realloc (&this->port_group, other.port_group);
  this->track_name_hash = other.track_name_hash;
  this->port_index = other.port_index;
  return *this;
}

bool
PortIdentifier::is_equal (const PortIdentifier &other) const
{
  bool eq =
    owner_type == other.owner_type && unit == other.unit && type == other.type
    && flow == other.flow && flags == other.flags && flags2 == other.flags2
    && track_name_hash == other.track_name_hash;
  if (!eq)
    return false;

  if (owner_type == PortIdentifier::OwnerType::PLUGIN)
    {
      eq = eq && plugin_identifier_is_equal (&plugin_id, &other.plugin_id);
    }

  /* if LV2 (has symbol) check symbol match,
   * other.wise check index match and label match */
  if (sym)
    {
      eq = eq && string_is_equal (sym, other.sym);
    }
  else
    {
      eq =
        eq && port_index == other.port_index
        && string_is_equal (label, other.label);
    }

  /* do string comparisons at the end */
  eq =
    eq && string_is_equal (uri, other.uri)
    && string_is_equal (port_group, other.port_group)
    && string_is_equal (ext_port_id, other.ext_port_id);

  return eq;
}

/**
 * To be used as GEqualFunc.
 */
int
port_identifier_is_equal_func (const void * a, const void * b)
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
  if (owner_type == PortIdentifier::OwnerType::PLUGIN)
    {
      plugin_identifier_print (&plugin_id, pl_buf);
    }
  snprintf (
    buf, buf_sz,
    "[PortIdentifier %p | hash %u]\nlabel: %s\n"
    "sym: %s\nuri: %s\ncomment: %s\nowner type: %s\n"
    "type: %s\nflow: %s\nflags: %s %s\nunit: %s\n"
    "port group: %s\next port id: %s\n"
    "track name hash: %u\nport idx: %d\nplugin: %s",
    this, get_hash (), label, sym, uri, comment, ENUM_NAME (owner_type),
    ENUM_NAME (type), ENUM_NAME (flow),
    ENUM_BITSET_TO_STRING (PortIdentifier::Flags, flags),
    ENUM_BITSET_TO_STRING (PortIdentifier::Flags2, flags2), ENUM_NAME (unit),
    port_group, ext_port_id, track_name_hash, port_index, pl_buf);
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
    plugin_identifier_validate (&this->plugin_id), false);
  return true;
}

OPTIMIZE_O3
uint32_t
PortIdentifier::get_hash () const
{
  const PortIdentifier * id = static_cast<const PortIdentifier *> (this);
  uint32_t               hash = 0;
  if (id->sym)
    {
      hash = hash ^ g_str_hash (id->sym);
    }
  /* don't check label when have symbol because label might be
   * localized */
  else if (id->label)
    {
      hash = hash ^ g_str_hash (id->label);
    }

  if (id->uri)
    hash = hash ^ g_str_hash (id->uri);
  hash = hash ^ g_int_hash (&id->owner_type);
  hash = hash ^ g_int_hash (&id->type);
  hash = hash ^ g_int_hash (&id->flow);
  hash = hash ^ g_int_hash (&id->flags);
  hash = hash ^ g_int_hash (&id->flags2);
  hash = hash ^ g_int_hash (&id->unit);
  hash = hash ^ plugin_identifier_get_hash (&id->plugin_id);
  if (id->port_group)
    hash = hash ^ g_str_hash (id->port_group);
  if (id->ext_port_id)
    hash = hash ^ g_str_hash (id->ext_port_id);
  hash = hash ^ id->track_name_hash;
  hash = hash ^ g_int_hash (&id->port_index);
  return hash;
}

PortIdentifier::PortIdentifier (const PortIdentifier &other)
{
  this->schema_version = PORT_IDENTIFIER_SCHEMA_VERSION;
  *this = other;
}

PortIdentifier::~PortIdentifier ()
{
  g_free_and_null (label);
  g_free_and_null (sym);
  g_free_and_null (uri);
  g_free_and_null (comment);
  g_free_and_null (port_group);
  g_free_and_null (ext_port_id);
}

const char *
port_identifier_get_label (void * data)
{
  PortIdentifier * self = static_cast<PortIdentifier *> (data);
  return self->get_label ();
}

void
port_identifier_destroy_notify (void * data)
{
  PortIdentifier * self = static_cast<PortIdentifier *> (data);
  delete self;
}

uint32_t
port_identifier_get_hash (const void * data)
{
  const PortIdentifier * self = static_cast<const PortIdentifier *> (data);
  return self->get_hash ();
}