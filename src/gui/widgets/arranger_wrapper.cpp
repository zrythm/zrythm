// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/widgets/arranger.h"
#include "gui/widgets/arranger_minimap.h"
#include "gui/widgets/arranger_wrapper.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/main_notebook.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/timeline_panel.h"
#include "gui/widgets/tracklist.h"
#include "zrythm_app.h"

#include "gtk_wrapper.h"

G_DEFINE_TYPE (ArrangerWrapperWidget, arranger_wrapper_widget, GTK_TYPE_WIDGET);

void
arranger_wrapper_widget_setup (
  ArrangerWrapperWidget *   self,
  ArrangerWidgetType        type,
  std::shared_ptr<SnapGrid> snap_grid)
{
  arranger_widget_setup (self->child, type, snap_grid);

  GtkAdjustment * adj_to_bind_to = NULL;
  if (type == ArrangerWidgetType::Timeline)
    {
      adj_to_bind_to =
        gtk_scrolled_window_get_vadjustment (MW_TRACKLIST->unpinned_scroll);
    }
  else if (type == ArrangerWidgetType::Midi)
    {
      adj_to_bind_to = gtk_scrolled_window_get_vadjustment (
        MW_MIDI_EDITOR_SPACE->piano_roll_keys_scroll);
      /* no minimap for MIDI arranger for now */
      gtk_widget_unparent (GTK_WIDGET (self->minimap));
    }
  self->right_scrollbar = GTK_SCROLLBAR (
    gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL, adj_to_bind_to));
  gtk_overlay_add_overlay (self->overlay, GTK_WIDGET (self->right_scrollbar));
  gtk_widget_set_halign (GTK_WIDGET (self->right_scrollbar), GTK_ALIGN_END);
}

static void
dispose (ArrangerWrapperWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->overlay));
  gtk_widget_unparent (GTK_WIDGET (self->minimap));

  G_OBJECT_CLASS (arranger_wrapper_widget_parent_class)
    ->dispose (G_OBJECT (self));
}

static void
finalize (ArrangerWrapperWidget * self)
{
  G_OBJECT_CLASS (arranger_wrapper_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

static void
arranger_wrapper_widget_init (ArrangerWrapperWidget * self)
{
  GtkBoxLayout * box_layout =
    GTK_BOX_LAYOUT (gtk_box_layout_new (GTK_ORIENTATION_VERTICAL));
  gtk_widget_set_layout_manager (
    GTK_WIDGET (self), GTK_LAYOUT_MANAGER (box_layout));

  gtk_widget_set_vexpand (GTK_WIDGET (self), true);

  self->overlay = GTK_OVERLAY (gtk_overlay_new ());
  gtk_widget_set_parent (GTK_WIDGET (self->overlay), GTK_WIDGET (self));

  self->child = static_cast<ArrangerWidget *> (
    g_object_new (ARRANGER_WIDGET_TYPE, nullptr));
  gtk_widget_set_vexpand (GTK_WIDGET (self->child), true);
  gtk_overlay_set_child (self->overlay, GTK_WIDGET (self->child));

  self->minimap = static_cast<ArrangerMinimapWidget *> (
    g_object_new (ARRANGER_MINIMAP_WIDGET_TYPE, nullptr));
  gtk_widget_set_parent (GTK_WIDGET (self->minimap), GTK_WIDGET (self));
}

static void
arranger_wrapper_widget_class_init (ArrangerWrapperWidgetClass * _klass)
{
  GObjectClass * klass = G_OBJECT_CLASS (_klass);
  /*GtkWidgetClass * wklass = GTK_WIDGET_CLASS (_klass);*/

  klass->dispose = (GObjectFinalizeFunc) dispose;
  klass->finalize = (GObjectFinalizeFunc) finalize;
}
