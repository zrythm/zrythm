/*
 * SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
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
