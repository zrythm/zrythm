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

#include "audio/port_identifier.h"
#include "utils/objects.h"
#include "utils/string.h"

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
  g_return_if_fail (dest && src && dest != src);
  g_free_and_null (dest->label);
  dest->label = g_strdup (src->label);
  dest->sym = g_strdup (src->sym);
  dest->owner_type = src->owner_type;
  dest->type = src->type;
  dest->flow = src->flow;
  dest->flags = src->flags;
  plugin_identifier_copy (
    &dest->plugin_id, &src->plugin_id);
  dest->ext_port_id = g_strdup (src->ext_port_id);
  dest->port_group = g_strdup (src->port_group);
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
    dest->owner_type == src->owner_type &&
    dest->type == src->type &&
    dest->flow == src->flow &&
    dest->flags == src->flags &&
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
    "[PortIdentifier]\nlabel: %s",
    self->label);
}

void
port_identifier_free_members (
  PortIdentifier * self)
{
  g_free_and_null (self->label);
  g_free_and_null (self->sym);
}
