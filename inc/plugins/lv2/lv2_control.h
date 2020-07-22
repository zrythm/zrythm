/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __PLUGINS_LV2_CONTROL_H__
#define __PLUGINS_LV2_CONTROL_H__

#include <stdint.h>

#include <lilv/lilv.h>

/** \file
 * Internal API for LV2 controls */

typedef struct Lv2Plugin Lv2Plugin;
typedef struct Lv2Port Lv2Port;

/* Controls */

typedef struct {
  float value;
  char* label;
} Lv2ScalePoint;


/**
 * Control change event, sent through ring buffers
 * for UI updates.
 */
typedef struct {
  uint32_t index;
  uint32_t protocol;
  uint32_t size;
  uint8_t  body[];
} Lv2ControlChange;

/** Type of plugin control. */
typedef enum {
  PORT,     ///< Control port
  PROPERTY  ///< Property (set via atom message)
} Lv2ControlType;

/** Plugin control. */
typedef struct Lv2Control
{
  Lv2Plugin*       plugin;
  Lv2ControlType type;
  LilvNode*   node;
  LilvNode*   symbol;          ///< Symbol
  LilvNode*   label;           ///< Human readable label
  LV2_URID    property;        ///< Iff type == PROPERTY
  uint32_t    index;           ///< Iff type == PORT
  Lv2Port *  port; ///< cache
  LilvNode*   group;           ///< Port/control group, or NULL
  void*       widget;          ///< Control Widget
  size_t      n_points;        ///< Number of scale points
  Lv2ScalePoint* points;          ///< Scale points
  LV2_URID    value_type;      ///< Type of control value
  LilvNode*   min;             ///< Minimum value
  float       minf;
  LilvNode*   max;             ///< Maximum value
  float       maxf;
  LilvNode*   def;             ///< Default value
  float       deff;
  bool        is_toggle;       ///< Boolean (0 and 1 only)
  bool        is_integer;      ///< Integer values only

  /* FIXME are these used ? */

  /** Control is a URI. */
  bool        is_uri;
  bool        is_enumeration;  ///< Point values only
  bool        is_logarithmic;  ///< Logarithmic scale
  bool        is_writable;     ///< Writable (input)
  bool        is_readable;     ///< Readable (output)
} Lv2Control;

typedef struct Lv2Controls
{
  int          n_controls;
  Lv2Control** controls;
} Lv2Controls;

Lv2Control*
lv2_new_port_control (
  Lv2Plugin* plugin, uint32_t index);

Lv2Control*
lv2_new_property_control (
  Lv2Plugin* plugin, const LilvNode* property);

void
lv2_add_control (
  Lv2Controls* controls,
  Lv2Control* control);

Lv2Control*
lv2_get_property_control (
  const Lv2Controls* controls,
  LV2_URID property);

/** Order scale points by value. */
int
lv2_scale_point_cmp (
  const Lv2ScalePoint* a, const Lv2ScalePoint* b);

void
lv2_control_set_control (
  const Lv2Control * control,
  uint32_t             size,
  LV2_URID             type,
  const void*          body);

/**
 * Returns the human readable control label.
 */
const char *
lv2_control_get_label (
  const Lv2Control * control);

/**
 * Returns the Lv2Control from the port index.
 */
Lv2Control *
lv2_control_get_from_port (
  Lv2Port * port);

#endif
