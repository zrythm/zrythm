/*
 * SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * \file
 */

#ifndef __GUI_WIDGETS_RANGE_ACTION_BUTTONS_H__
#define __GUI_WIDGETS_RANGE_ACTION_BUTTONS_H__

#include "gtk_wrapper.h"

/**
 * @addtogroup widgets
 *
 * @{
 */

#define RANGE_ACTION_BUTTONS_WIDGET_TYPE \
  (range_action_buttons_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  RangeActionButtonsWidget,
  range_action_buttons_widget,
  Z,
  RANGE_ACTION_BUTTONS_WIDGET,
  GtkBox)

typedef struct _SnapGridWidget SnapGridWidget;
typedef struct SnapGrid        SnapGrid;

typedef struct _RangeActionButtonsWidget
{
  GtkBox      parent_instance;
  GtkButton * insert_silence;
  GtkButton * remove;
} RangeActionButtonsWidget;

/**
 * @}
 */

#endif
