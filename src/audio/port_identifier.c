/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/port.h"
#include "audio/port_identifier.h"
#include "plugins/plugin_identifier.h"
#include "utils/objects.h"
#include "utils/string.h"

void
port_identifier_init (
  PortIdentifier * self)
{
  self->schema_version =
    PORT_IDENTIFIER_SCHEMA_VERSION;
  self->track_pos = -1;
  plugin_identifier_init (&self->plugin_id);
}

/**
 * Port group comparator function where @ref p1 and
 * @ref p2 are pointers to Port.
 */
int
port_identifier_port_group_cmp (
  const void* p1, const void* p2)
{
  const Port * control1 = *(const Port **)p1;
  const Port * control2 = *(const Port **)p2;

  g_return_val_if_fail (IS_PORT (control1), -1);
  g_return_val_if_fail (IS_PORT (control2), -1);

  /* use index for now - this assumes that ports
   * inside port groups are declared in sequence */
  return
    control1->id.port_index -
    control2->id.port_index;

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
port_identifier_copy (
  PortIdentifier * dest,
  PortIdentifier * src)
{
  g_return_if_fail (dest != src);
  dest->schema_version = src->schema_version;
  string_copy_w_realloc (
    &dest->label, src->label);
  string_copy_w_realloc (
    &dest->sym, src->sym);
  string_copy_w_realloc (
    &dest->uri, src->uri);
  string_copy_w_realloc (
    &dest->comment, src->comment);
  dest->owner_type = src->owner_type;
  dest->type = src->type;
  dest->flow = src->flow;
  dest->flags = src->flags;
  dest->flags2 = src->flags2;
  plugin_identifier_copy (
    &dest->plugin_id, &src->plugin_id);
  string_copy_w_realloc (
    &dest->ext_port_id, src->ext_port_id);
  string_copy_w_realloc (
    &dest->port_group, src->port_group);
  dest->track_pos = src->track_pos;
  dest->port_index = src->port_index;
}

/**
 * Returns if the 2 PortIdentifier's are equal.
 */
bool
port_identifier_is_equal (
  PortIdentifier * src,
  PortIdentifier * dest)
{
  bool eq =
    string_is_equal (dest->label, src->label) &&
    string_is_equal (dest->uri, src->uri) &&
    string_is_equal (dest->comment, src->comment) &&
    dest->owner_type == src->owner_type &&
    dest->type == src->type &&
    dest->flow == src->flow &&
    dest->flags == src->flags &&
    dest->flags2 == src->flags2 &&
    dest->track_pos == src->track_pos &&
    string_is_equal (
      dest->port_group, src->port_group) &&
    string_is_equal (
      dest->ext_port_id, src->ext_port_id);
  if (dest->owner_type == PORT_OWNER_TYPE_PLUGIN)
    {
      eq =
        eq &&
        plugin_identifier_is_equal (
          &dest->plugin_id, &src->plugin_id);
    }

  /* if LV2 (has symbol) check symbol match,
   * otherwise check index match */
  if (dest->sym)
    {
      eq =
        eq &&
        string_is_equal (dest->sym, src->sym);
    }
  else
    {
      eq =
        eq &&
        dest->port_index == src->port_index;
    }

  return eq;
}

void
port_identifier_print (
  PortIdentifier * self)
{
  g_message (
    "[PortIdentifier %p]\nlabel: %s\n"
    "sym: %s\nuri: %s\ncomment: %s\nowner type: %s\n"
    "type: %s\nflow: %s\nflags: %d %d\n"
    "port group: %s\next port id: %s\n"
    "track pos: %d\nport idx: %d",
    self,
    self->label, self->sym, self->uri, self->comment,
    port_owner_type_strings[self->owner_type].str,
    port_type_strings[self->type].str,
    port_flow_strings[self->flow].str,
    self->flags, self->flags2, self->port_group,
    self->ext_port_id, self->track_pos,
    self->port_index);
}

bool
port_identifier_validate (
  PortIdentifier * self)
{
  g_return_val_if_fail (
    self->schema_version ==
      PORT_IDENTIFIER_SCHEMA_VERSION &&
    plugin_identifier_validate (&self->plugin_id),
    false);
  return true;
}

void
port_identifier_free_members (
  PortIdentifier * self)
{
  g_free_and_null (self->label);
  g_free_and_null (self->sym);
  g_free_and_null (self->uri);
  g_free_and_null (self->comment);
  g_free_and_null (self->port_group);
  g_free_and_null (self->ext_port_id);
  object_set_to_zero (self);
}
