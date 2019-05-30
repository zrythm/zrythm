/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Tracklist backend for the special Track's below
 * the RulerWidget.
 */

#ifndef __AUDIO_PINNED_TRACKLIST_H__
#define __AUDIO_PINNED_TRACKLIST_H__

#include "utils/yaml.h"

/**
 * @addtogroup audio
 *
 * @{
 */

typedef struct Track Track;
typedef struct _PinnedTracklistWidget
  PinnedTracklistWidget;

#define PINNED_TRACKLIST (&PROJECT->pinned_tracklist)

/**
 * The PinnedTracklist contains the spacial Track's
 * that appear below the RulerWidget.
 */
typedef struct PinnedTracklist
{
  Track *           chord_track;
  Track *           marker_track;

  PinnedTracklistWidget * widget;
} PinnedTracklist;

static const cyaml_schema_field_t
  pinned_tracklist_fields_schema[] =
{

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
pinned_tracklist_schema = {
  CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
    PinnedTracklist, pinned_tracklist_fields_schema),
};

/**
 * Initialize the PinnedTracklist.
 */
void
pinned_tracklist_init (
  PinnedTracklist * self);

/**
 * @}
 */

#endif
