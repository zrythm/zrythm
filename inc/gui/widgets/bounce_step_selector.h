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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_BOUNCE_STEP_SELECTOR_H__
#define __GUI_WIDGETS_BOUNCE_STEP_SELECTOR_H__

/**
 * \file
 *
 * Bounce step selector.
 */

#include <gtk/gtk.h>

#define BOUNCE_STEP_SELECTOR_WIDGET_TYPE \
  (bounce_step_selector_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  BounceStepSelectorWidget,
  bounce_step_selector_widget,
  Z,
  BOUNCE_STEP_SELECTOR_WIDGET,
  GtkBox)

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef struct _BounceStepSelectorWidget
{
  GtkBox parent_instance;

  GtkToggleButton * before_inserts_toggle;
  gulong            before_inserts_toggle_id;
  GtkToggleButton * pre_fader_toggle;
  gulong            pre_fader_toggle_id;
  GtkToggleButton * post_fader_toggle;
  gulong            post_fader_toggle_id;

} BounceStepSelectorWidget;

/**
 * Creates a BounceStepSelectorWidget.
 */
BounceStepSelectorWidget *
bounce_step_selector_widget_new (void);

/**
 * @}
 */

#endif
