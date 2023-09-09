/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Custom image.
 */

#ifndef __GUI_WIDGETS_CUSTOM_IMAGE_H__
#define __GUI_WIDGETS_CUSTOM_IMAGE_H__

#include "utils/ui.h"

#include <gtk/gtk.h>

#define RW_CUSTOM_IMAGE_MARKER_SIZE 8
#define RW_CUE_MARKER_HEIGHT 12
#define RW_CUE_MARKER_WIDTH 7
#define RW_PLAYHEAD_TRIANGLE_WIDTH 12
#define RW_PLAYHEAD_TRIANGLE_HEIGHT 8
#define RW_RANGE_HEIGHT_DIVISOR 4

/**
 * Minimum number of pixels between beat lines.
 */
#define RW_PX_TO_HIDE_BEATS 40.0

#define CUSTOM_IMAGE_WIDGET_TYPE (custom_image_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  CustomImageWidget,
  custom_image_widget,
  Z,
  CUSTOM_IMAGE_WIDGET,
  GtkWidget)

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef struct _CustomImageWidget
{
  GtkWidget parent_instance;

  GdkTexture * texture;

  graphene_rect_t bounds;
} CustomImageWidget;

void
custom_image_widget_set_texture (CustomImageWidget * self, GdkTexture * texture);

/**
 * @}
 */

#endif
