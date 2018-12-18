/*
 * Copyright (C) 2018 Alexandros Theodotou
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

/** \file */

#ifndef __GUI_WIDGETS_AUDIO_REGION_H__
#define __GUI_WIDGETS_AUDIO_REGION_H__

#include "audio/region.h"
#include "gui/widgets/region.h"

#include <gtk/gtk.h>

#define AUDIO_REGION_WIDGET_TYPE (audio_region_widget_get_type ())
G_DECLARE_FINAL_TYPE (AudioRegionWidget,
                      audio_region_widget,
                      AUDIO_REGION,
                      WIDGET,
                      RegionWidget);

typedef struct AudioRegion AudioRegion;

typedef enum AudioRegionCursor
{
  AUDIO_REGION_CURSOR_DEFAULT,
  AUDIO_REGION_CURSOR_RESIZE_L,
  AUDIO_REGION_CURSOR_REPEAT_L,
  AUDIO_REGION_CURSOR_RESIZE_R,
  AUDIO_REGION_CURSOR_REPEAT_R
} AudioRegionCursor;

typedef struct _AudioRegionWidget
{
  RegionWidget             parent_instance;
  AudioRegionCursor         cursor_state;
} AudioRegionWidget;

/**
 * Creates a region.
 */
AudioRegionWidget *
audio_region_widget_new (AudioRegion * audio_region);

#endif
