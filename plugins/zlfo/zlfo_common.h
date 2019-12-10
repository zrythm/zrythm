/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of ZPlugins
 *
 * ZPlugins is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * ZPlugins is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU General Affero Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Common code for both the DSP and the UI.
 */

#ifndef __Z_LFO_COMMON_H__
#define __Z_LFO_COMMON_H__

#include <string.h>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/log/log.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"

typedef struct ZLfoUris
{
  LV2_URID atom_eventTransfer;
  LV2_URID atom_Blank;
  LV2_URID atom_Object;
  LV2_URID atom_Float;
  LV2_URID atom_Int;
  LV2_URID log_Entry;
  LV2_URID log_Error;
  LV2_URID log_Note;
  LV2_URID log_Trace;
  LV2_URID log_Warning;
} ZLfoUris;

typedef enum PortIndex
{
  /** GUI to plugin communication. */
  LFO_CONTROL,
  /** Plugin to UI communication. */
  LFO_NOTIFY,
  LFO_FREQ,
  LFO_PHASE,
  LFO_FOLLOW_TIME,
  LFO_SINE,
  LFO_SAW_UP,
  LFO_SAW_DOWN,
  LFO_TRIANGLE,
  LFO_SQUARE,
  LFO_RANDOM,
} PortIndex;

static inline void
map_uris (
  LV2_URID_Map* map,
  ZLfoUris* uris)
{
#define MAP(x,uri) \
  uris->x = map->map (map->handle, uri)

  MAP (atom_Blank, LV2_ATOM__Blank);
  MAP (atom_Object, LV2_ATOM__Object);
  MAP (atom_Float, LV2_ATOM__Float);
  MAP (atom_Int, LV2_ATOM__Int);
  MAP (atom_eventTransfer, LV2_ATOM__eventTransfer);
  MAP (log_Entry, LV2_LOG__Entry);
  MAP (log_Error, LV2_LOG__Error);
  MAP (log_Note, LV2_LOG__Note);
  MAP (log_Trace, LV2_LOG__Trace);
  MAP (log_Warning, LV2_LOG__Warning);
}

/**
 * Logs an error.
 */
static inline void
log_error (
  LV2_Log_Log * log,
  ZLfoUris *    uris,
  const char *  _fmt,
  ...)
{
  va_list args;
  va_start (args, _fmt);

  char fmt[900];
  strcpy (fmt, _fmt);
  strcat (fmt, "\n");

  if (log)
    {
      log->vprintf (
        log->handle, uris->log_Error,
        fmt, args);
    }
  else
    {
      vfprintf (stderr, fmt, args);
    }
  va_end (args);
}

#endif
