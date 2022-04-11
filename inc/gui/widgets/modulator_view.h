// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Modulator view in the bottom panel.
 */

#ifndef __GUI_WIDGETS_MODULATOR_VIEW_H__
#define __GUI_WIDGETS_MODULATOR_VIEW_H__

#include <adwaita.h>
#include <gtk/gtk.h>

#define MODULATOR_VIEW_WIDGET_TYPE \
  (modulator_view_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ModulatorViewWidget,
  modulator_view_widget,
  Z,
  MODULATOR_VIEW_WIDGET,
  GtkWidget)

TYPEDEF_STRUCT_UNDERSCORED (ModulatorWidget);
TYPEDEF_STRUCT_UNDERSCORED (ModulatorMacroWidget);
TYPEDEF_STRUCT_UNDERSCORED (ColorAreaWidget);
TYPEDEF_STRUCT (Track);

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_MODULATOR_VIEW \
  MW_BOT_DOCK_EDGE->modulator_view

/**
 * Brings up the ModulatorViewWidget in the notebook.
 */
#define SHOW_MODULATOR_VIEW \
  gtk_notebook_set_current_page ( \
    MW_MODULATOR_VIEW->bot_notebook, 2)

/**
 * The ModulatorViewWidget contains the
 * ModulatorWidgets for the selected Track.
 */
typedef struct _ModulatorViewWidget
{
  GtkWidget              parent_instance;
  AdwStatusPage *        no_modulators_status_page;
  GtkBox *               modulators_box;
  GtkBox *               macros_box;
  ColorAreaWidget *      color;
  GtkLabel *             track_name_lbl;
  Track *                track;
  ModulatorWidget *      modulators[14];
  ModulatorMacroWidget * macros[8];
} ModulatorViewWidget;

void
modulator_view_widget_refresh (
  ModulatorViewWidget * self,
  Track *               track);

ModulatorViewWidget *
modulator_view_widget_new (void);

/**
 * @}
 */

#endif
