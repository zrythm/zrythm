// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/control_room.h"
#include "common/utils/resources.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/gtk_widgets/center_dock.h"
#include "gui/backend/gtk_widgets/chord_pack_browser.h"
#include "gui/backend/gtk_widgets/main_window.h"
#include "gui/backend/gtk_widgets/monitor_section.h"
#include "gui/backend/gtk_widgets/panel_file_browser.h"
#include "gui/backend/gtk_widgets/plugin_browser.h"
#include "gui/backend/gtk_widgets/right_dock_edge.h"
#include "gui/backend/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (RightDockEdgeWidget, right_dock_edge_widget, GTK_TYPE_WIDGET)

/* TODO implement after workspaces */
#if 0
static void
on_notebook_switch_page (
  GtkNotebook *         notebook,
  GtkWidget *           page,
  guint                 page_num,
  RightDockEdgeWidget * self)
{
  z_debug (
    "setting right dock page to {}", page_num);

  g_settings_set_int (
    S_UI, "right-panel-tab", (int) page_num);
}
#endif

void
right_dock_edge_widget_setup (RightDockEdgeWidget * self)
{
  monitor_section_widget_setup (self->monitor_section, CONTROL_ROOM.get ());

  /* TODO load from workspaces */
#if 0
  GtkNotebook * notebook =
    foldable_notebook_widget_get_notebook (
      self->right_notebook);
  int page_num =
    g_settings_get_int (S_UI, "right-panel-tab");
  gtk_notebook_set_current_page (
    notebook, page_num);

  g_signal_connect (
    G_OBJECT (notebook), "switch-page",
    G_CALLBACK (on_notebook_switch_page), self);
#endif
}

static void
dispose (RightDockEdgeWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->panel_frame));

  G_OBJECT_CLASS (right_dock_edge_widget_parent_class)->dispose (G_OBJECT (self));
}

static void
right_dock_edge_widget_init (RightDockEdgeWidget * self)
{
  g_type_ensure (MONITOR_SECTION_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  GtkBox * box;

#define ADD_TAB(widget, icon, title) \
  { \
    PanelWidget * panel_widget = PANEL_WIDGET (panel_widget_new ()); \
    panel_widget_set_child (panel_widget, GTK_WIDGET (widget)); \
    panel_widget_set_icon_name (panel_widget, icon); \
    panel_widget_set_title (panel_widget, title); \
    panel_frame_add (self->panel_frame, panel_widget); \
  }

  /* add plugin browser */
  self->plugin_browser = plugin_browser_widget_new ();
  box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  self->plugin_browser_box = box;
  gtk_box_append (GTK_BOX (box), GTK_WIDGET (self->plugin_browser));
  ADD_TAB (
    GTK_WIDGET (box), "gnome-icon-library-puzzle-piece-symbolic",
    _ ("Plugin Browser"));

  /* add file browser */
  self->file_browser = panel_file_browser_widget_new ();
  box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  self->file_browser_box = box;
  gtk_box_append (GTK_BOX (box), GTK_WIDGET (self->file_browser));
  ADD_TAB (
    GTK_WIDGET (box), "gnome-icon-library-library-music-symbolic",
    _ ("File Browser"));

  /* add control room */
  self->monitor_section = monitor_section_widget_new ();
  box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  self->monitor_section_box = box;
  gtk_box_append (GTK_BOX (box), GTK_WIDGET (self->monitor_section));
  ADD_TAB (
    GTK_WIDGET (box), "gnome-icon-library-speakers-symbolic",
    _ ("Monitor Section"));

  /* add chord preset browser */
  self->chord_pack_browser = chord_pack_browser_widget_new ();
  box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  self->chord_pack_browser_box = box;
  gtk_box_append (GTK_BOX (box), GTK_WIDGET (self->chord_pack_browser));
  ADD_TAB (GTK_WIDGET (box), "minuet-chords", _ ("Chord Preset Browser"));

  /* TODO: uncomment after
   * https://gitlab.gnome.org/chergert/libpanel/-/issues/10 */
#if 0
  /* add file browser button */
  PanelFrameHeader * header =
    panel_frame_get_header (self->panel_frame);
  GtkButton * tb = GTK_BUTTON (
    gtk_button_new_from_icon_name ("hdd"));
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (tb), _ ("File Browser"));
  gtk_actionable_set_action_name (
    GTK_ACTIONABLE (tb), "app.show-file-browser");
  panel_frame_header_add_suffix (
    header, 0, GTK_WIDGET (tb));
#endif
}

static void
right_dock_edge_widget_class_init (RightDockEdgeWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "right_dock_edge.ui");

  gtk_widget_class_set_css_name (klass, "right-dock-edge");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, RightDockEdgeWidget, x)

  BIND_CHILD (panel_frame);

#undef BIND_CHILD

  gtk_widget_class_set_layout_manager_type (klass, GTK_TYPE_BIN_LAYOUT);

  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}
