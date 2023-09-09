// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/event_viewer.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/pinned_tracklist.h"
#include "gui/widgets/right_dock_edge.h"
#include "gui/widgets/timeline_panel.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <adwaita.h>

G_DEFINE_TYPE (CenterDockWidget, center_dock_widget, GTK_TYPE_WIDGET)

static bool
on_center_dock_mapped (GtkWidget * widget, CenterDockWidget * self)
{
  if (self->first_draw)
    {
      self->first_draw = false;

      /* TODO set divider positions */
    }

  return false;
}

void
center_dock_widget_setup (CenterDockWidget * self)
{
  bot_dock_edge_widget_setup (self->bot_dock_edge);
  left_dock_edge_widget_setup (self->left_dock_edge);
  right_dock_edge_widget_setup (self->right_dock_edge);
  main_notebook_widget_setup (self->main_notebook);

  g_signal_connect (
    G_OBJECT (self), "map", G_CALLBACK (on_center_dock_mapped), self);
}

/**
 * Prepare for finalization.
 */
void
center_dock_widget_tear_down (CenterDockWidget * self)
{
  if (self->left_dock_edge)
    {
      left_dock_edge_widget_tear_down (self->left_dock_edge);
    }
  if (self->main_notebook)
    {
      main_notebook_widget_tear_down (self->main_notebook);
    }
}

static void
dispose (CenterDockWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->dock));

  G_OBJECT_CLASS (center_dock_widget_parent_class)->dispose (G_OBJECT (self));
}

static void
center_dock_widget_init (CenterDockWidget * self)
{
  self->first_draw = true;

  g_type_ensure (BOT_DOCK_EDGE_WIDGET_TYPE);
  g_type_ensure (RIGHT_DOCK_EDGE_WIDGET_TYPE);
  g_type_ensure (LEFT_DOCK_EDGE_WIDGET_TYPE);
  g_type_ensure (MAIN_NOTEBOOK_WIDGET_TYPE);
  g_type_ensure (PANEL_TYPE_PANED);

  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
center_dock_widget_class_init (CenterDockWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "center_dock.ui");

  gtk_widget_class_set_css_name (klass, "center-dock");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, CenterDockWidget, x)

  BIND_CHILD (dock);
  BIND_CHILD (main_notebook);
  BIND_CHILD (left_dock_edge);
  BIND_CHILD (bot_dock_edge);
  BIND_CHILD (right_dock_edge);

#undef BIND_CHILD

  gtk_widget_class_set_layout_manager_type (klass, GTK_TYPE_BIN_LAYOUT);

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}
