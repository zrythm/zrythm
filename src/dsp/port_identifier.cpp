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

const char *
port_unit_to_str (const ZPortUnit unit)
{
  static const char * port_unit_strings[] = {
    "none", "Hz", "MHz", "dB", "°", "s", "ms", "μs",
  };
  return port_unit_strings[ENUM_VALUE_TO_INT (unit)];
}

void
port_identifier_init (PortIdentifier * self)
{
  memset (self, 0, sizeof (PortIdentifier));
  self->schema_version = PORT_IDENTIFIER_SCHEMA_VERSION;
  plugin_identifier_init (&self->plugin_id);
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

/**
 * Copy the identifier content from \ref src to
 * \ref dest.
 *
 * @note This frees/allocates memory on \ref dest.
 */
void
port_identifier_copy (PortIdentifier * dest, const PortIdentifier * src)
{
  g_return_if_fail (dest != src);
  dest->schema_version = src->schema_version;
  string_copy_w_realloc (&dest->label, src->label);
  string_copy_w_realloc (&dest->sym, src->sym);
  string_copy_w_realloc (&dest->uri, src->uri);
  string_copy_w_realloc (&dest->comment, src->comment);
  dest->owner_type = src->owner_type;
  dest->type = src->type;
  dest->flow = src->flow;
  dest->flags = src->flags;
  dest->flags2 = src->flags2;
  dest->unit = src->unit;
  plugin_identifier_copy (&dest->plugin_id, &src->plugin_id);
  string_copy_w_realloc (&dest->ext_port_id, src->ext_port_id);
  string_copy_w_realloc (&dest->port_group, src->port_group);
  dest->track_name_hash = src->track_name_hash;
  dest->port_index = src->port_index;
}

bool
port_identifier_is_equal (const PortIdentifier * src, const PortIdentifier * dest)
{
  bool eq =
    dest->owner_type == src->owner_type && dest->unit == src->unit
    && dest->type == src->type && dest->flow == src->flow
    && dest->flags == src->flags && dest->flags2 == src->flags2
    && dest->track_name_hash == src->track_name_hash;
  if (!eq)
    return false;

  if (dest->owner_type == ZPortOwnerType::Z_PORT_OWNER_TYPE_PLUGIN)
    {
      eq = eq && plugin_identifier_is_equal (&dest->plugin_id, &src->plugin_id);
    }

  /* if LV2 (has symbol) check symbol match,
   * otherwise check index match and label match */
  if (dest->sym)
    {
      eq = eq && string_is_equal (dest->sym, src->sym);
    }
  else
    {
      eq =
        eq && dest->port_index == src->port_index
        && string_is_equal (dest->label, src->label);
    }

  /* do string comparisons at the end */
  eq =
    eq && string_is_equal (dest->uri, src->uri)
    && string_is_equal (dest->port_group, src->port_group)
    && string_is_equal (dest->ext_port_id, src->ext_port_id);

  return eq;
}

/**
 * To be used as GEqualFunc.
 */
int
port_identifier_is_equal_func (const void * a, const void * b)
{
  bool is_equal = port_identifier_is_equal (
    (const PortIdentifier *) a, (const PortIdentifier *) b);
  return is_equal;
}

void
port_identifier_print_to_str (
  const PortIdentifier * self,
  char *                 buf,
  size_t                 buf_sz)
{
  char pl_buf[800];
  strcpy (pl_buf, "{none}");
  if (self->owner_type == ZPortOwnerType::Z_PORT_OWNER_TYPE_PLUGIN)
    {
      plugin_identifier_print (&self->plugin_id, pl_buf);
    }
  snprintf (
    buf, buf_sz,
    "[PortIdentifier %p | hash %u]\nlabel: %s\n"
    "sym: %s\nuri: %s\ncomment: %s\nowner type: %s\n"
    "type: %s\nflow: %s\nflags: %s %s\nunit: %s\n"
    "port group: %s\next port id: %s\n"
    "track name hash: %u\nport idx: %d\nplugin: %s",
    self, port_identifier_get_hash (self), self->label, self->sym, self->uri,
    self->comment, ENUM_NAME (self->owner_type), ENUM_NAME (self->type),
    ENUM_NAME (self->flow), ENUM_BITSET_TO_STRING (ZPortFlags, self->flags),
    ENUM_BITSET_TO_STRING (ZPortFlags2, self->flags2), ENUM_NAME (self->unit),
    self->port_group, self->ext_port_id, self->track_name_hash,
    self->port_index, pl_buf);
}

void
port_identifier_print (const PortIdentifier * self)
{
  size_t size = 2000;
  char   buf[size];
  port_identifier_print_to_str (self, buf, size);
  g_message ("%s", buf);
}

bool
port_identifier_validate (PortIdentifier * self)
{
  g_return_val_if_fail (
    /*self->schema_version == PORT_IDENTIFIER_SCHEMA_VERSION*/
    plugin_identifier_validate (&self->plugin_id), false);
  return true;
}

OPTIMIZE_O3
uint32_t
port_identifier_get_hash (const void * self)
{
  const PortIdentifier * id = (const PortIdentifier *) self;
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

PortIdentifier *
port_identifier_clone (const PortIdentifier * src)
{
  PortIdentifier * self = object_new (PortIdentifier);
  self->schema_version = PORT_IDENTIFIER_SCHEMA_VERSION;

  port_identifier_copy (self, src);

  return self;
}

void
port_identifier_free_members (PortIdentifier * self)
{
  g_free_and_null (self->label);
  g_free_and_null (self->sym);
  g_free_and_null (self->uri);
  g_free_and_null (self->comment);
  g_free_and_null (self->port_group);
  g_free_and_null (self->ext_port_id);
  object_set_to_zero (self);
}

void
port_identifier_free (PortIdentifier * self)
{
  port_identifier_free_members (self);
  object_zero_and_free (self);
}

void
port_identifier_free_func (void * self)
{
  port_identifier_free ((PortIdentifier *) self);
}
