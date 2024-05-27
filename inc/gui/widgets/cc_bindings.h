/*
 * SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * @file
 *
 * CC Bindings matrix.
 */

#ifndef __GUI_WIDGETS_CC_BINDINGS_H__
#define __GUI_WIDGETS_CC_BINDINGS_H__

#include "gtk_wrapper.h"

typedef struct _CcBindingsTreeWidget CcBindingsTreeWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define CC_BINDINGS_WIDGET_TYPE (cc_bindings_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  CcBindingsWidget,
  cc_bindings_widget,
  Z,
  CC_BINDINGS_WIDGET,
  GtkBox)

#define MW_CC_BINDINGS (MW_MAIN_NOTEBOOK->cc_bindings)

/**
 * Left dock widget.
 */
typedef struct _CcBindingsWidget
{
  GtkBox parent_instance;

  CcBindingsTreeWidget * bindings_tree;
} CcBindingsWidget;

CcBindingsWidget *
cc_bindings_widget_new (void);

void
cc_bindings_widget_refresh (CcBindingsWidget * self);

/**
 * @}
 */

#endif
