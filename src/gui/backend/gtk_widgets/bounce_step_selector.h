/*
 * SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#ifndef __GUI_WIDGETS_BOUNCE_STEP_SELECTOR_H__
#define __GUI_WIDGETS_BOUNCE_STEP_SELECTOR_H__

/**
 * @file
 *
 * Bounce step selector.
 */

#include "gui/backend/gtk_widgets/gtk_wrapper.h"

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
