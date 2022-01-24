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

/**
 * \file
 *
 * Base widget class for Region's.
 */

#ifndef __GUI_WIDGETS_REGION_H__
#define __GUI_WIDGETS_REGION_H__

#include "audio/region.h"
#include "gui/widgets/arranger_object.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

/**
 * @addtogroup widgets
 *
 * @{
 */

#define REGION_NAME_FONT_NO_SIZE "Sans SemiBold"
#ifdef _WOE32
#define REGION_NAME_FONT \
  REGION_NAME_FONT_NO_SIZE " 7"
#else
#define REGION_NAME_FONT \
  REGION_NAME_FONT_NO_SIZE " 8"
#endif
#define REGION_NAME_PADDING_R 5
#define REGION_NAME_BOX_PADDING 2
#define REGION_NAME_BOX_CURVINESS 4.0

/**
 * Returns the lane rectangle for the region.
 */
void
region_get_lane_full_rect (
  ZRegion *       self,
  GdkRectangle * rect);

/**
 * Draws the ZRegion in the given cairo context in
 * relative coordinates.
 *
 * @param rect Arranger rectangle.
 */
HOT
void
region_draw (
  ZRegion *      self,
  GtkSnapshot *  snapshot,
  GdkRectangle * rect);

#endif
