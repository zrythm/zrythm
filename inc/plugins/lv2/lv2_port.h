/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

/*
  Copyright 2007-2016 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/**
 * \file
 *
 * LV2 Ports.
 */

#ifndef __PLUGINS_LV2_PORT_H__
#define __PLUGINS_LV2_PORT_H__

typedef struct Port Port;

typedef struct Lv2Port
{
  /** Parent port identifier. */
  PortIdentifier  port_id;

  /** Pointer back to Zrythm Port. */
  Port *          port;

  /** LV2 port. */
  const LilvPort* lilv_port;

  /** For MIDI ports, otherwise NULL. */
  LV2_Evbuf*      evbuf;

  /**
   * Control widget, if applicable.
   *
   * Only used for generic UIs.
   */
  void*           widget;

  /** Custom buffer size, or 0. */
  size_t          buf_size;

  /** Port index. */
  uint32_t        index;

  /** Pointer to control, if control. */
  Lv2Control *    lv2_control;

  /**
   * Whether the port received a UI event from
   * the plugin UI in this cycle.
   *
   * This is used to avoid re-sending that event
   * to the plugin.
   *
   * @note for control ports only.
   */
  int             received_ui_event;

  /**
   * Whether the port should send an event that
   * it has changed.
   *
   * This should happen if it wwas changed through
   * zrythm and not through its UI.
   *
   * This is used to avoid sending an event to
   * the plugin unless the port changed via other
   * means than from the plugin's UI.
   *
   * Whent he port is changed via other means,
   * this should be set to 1.
   */
  //int             pending_send_event;

  /** The value in the previous cycle. */
  //float           prev_control;

  /** 1 if this value was set via automation. */
  int             automating;

  /** True for event, false for atom. */
  int             old_api;
} Lv2Port;

static const cyaml_schema_field_t
  lv2_port_fields_schema[] =
{
  CYAML_FIELD_MAPPING (
    "port_id", CYAML_FLAG_DEFAULT,
    Lv2Port, port_id,
    port_identifier_fields_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  lv2_port_schema =
{
  CYAML_VALUE_MAPPING (CYAML_FLAG_DEFAULT,
  Lv2Port, lv2_port_fields_schema),
};

/**
 * Function to get a port's value from its string
 * symbol.
 *
 * Used when saving the state.
 * This function MUST set size and type
 * appropriately.
 */
const void *
lv2_port_get_value_from_symbol (
  const char * port_sym,
  void       * user_data,
  uint32_t   * size,
  uint32_t   * type);

/**
 * Returns the Lv2Port corresponding to the given
 * symbol.
 *
 * TODO: Build an index to make this faster,
 * currently O(n) which may be
 * a problem when restoring the state of plugins
 * with many ports.
 */
Lv2Port*
lv2_port_get_by_symbol (
  Lv2Plugin* plugin,
  const char* sym);

/**
 * Gets the symbol as a string.
 */
const char *
lv2_port_get_symbol_as_string (
  Lv2Plugin * plugin,
  Lv2Port *   port);

#endif
