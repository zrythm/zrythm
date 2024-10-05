// SPDX-FileCopyrightText: © 2019-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/track.h"
#include "common/dsp/tracklist.h"
#include "common/plugins/plugin.h"
#include "common/utils/gtk.h"
#include "common/utils/resources.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/gtk_widgets/color_area.h"
#include "gui/cpp/gtk_widgets/drag_dest_box.h"
#include "gui/cpp/gtk_widgets/gtk_flipper.h"
#include "gui/cpp/gtk_widgets/modulator.h"
#include "gui/cpp/gtk_widgets/modulator_macro.h"
#include "gui/cpp/gtk_widgets/modulator_view.h"

G_DEFINE_TYPE (ModulatorViewWidget, modulator_view_widget, GTK_TYPE_WIDGET)

void
modulator_view_widget_refresh (ModulatorViewWidget * self, ModulatorTrack * track)
{
  self->track = track;
  gtk_label_set_markup (self->track_name_lbl, track->name_.c_str ());
  color_area_widget_set_color (self->color, track->color_);

  z_gtk_widget_remove_all_children (GTK_WIDGET (self->modulators_box));

  for (auto &modulator : self->track->modulators_)
    {
      if (!modulator->modulator_widget_)
        {
          modulator->modulator_widget_ = modulator_widget_new (modulator.get ());
        }
      gtk_box_append (
        GTK_BOX (self->modulators_box),
        GTK_WIDGET (modulator->modulator_widget_));
    }

  DragDestBoxWidget * drag_dest = drag_dest_box_widget_new (
    GTK_ORIENTATION_HORIZONTAL, 0,
    DragDestBoxType::DRAG_DEST_BOX_TYPE_MODULATORS);
  gtk_box_append (GTK_BOX (self->modulators_box), GTK_WIDGET (drag_dest));

  gtk_widget_set_visible (
    GTK_WIDGET (self->no_modulators_status_page),
    self->track->modulators_.empty ());
}

ModulatorViewWidget *
modulator_view_widget_new (void)
{
  ModulatorViewWidget * self = static_cast<ModulatorViewWidget *> (
    g_object_new (MODULATOR_VIEW_WIDGET_TYPE, nullptr));

  return self;
}

static void
modulator_view_widget_init (ModulatorViewWidget * self)
{
  g_type_ensure (GTK_TYPE_FLIPPER);

  gtk_widget_init_template (GTK_WIDGET (self));

  GdkRGBA color;
  gdk_rgba_parse (&color, "gray");
  color_area_widget_set_color (self->color, Color (color));

  DragDestBoxWidget * drag_dest = drag_dest_box_widget_new (
    GTK_ORIENTATION_HORIZONTAL, 0,
    DragDestBoxType::DRAG_DEST_BOX_TYPE_MODULATORS);
  gtk_box_append (GTK_BOX (self->modulators_box), GTK_WIDGET (drag_dest));

  for (int i = 0; i < 8; i++)
    {
      self->macros[i] = modulator_macro_widget_new (i);
      gtk_widget_set_visible (GTK_WIDGET (self->macros[i]), true);
      gtk_box_append (GTK_BOX (self->macros_box), GTK_WIDGET (self->macros[i]));
    }
}

static void
modulator_view_widget_class_init (ModulatorViewWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "modulator_view.ui");

  gtk_widget_class_set_css_name (klass, "modulator-view");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, ModulatorViewWidget, x)

  BIND_CHILD (color);
  BIND_CHILD (track_name_lbl);
  BIND_CHILD (modulators_box);
  BIND_CHILD (macros_box);
  BIND_CHILD (no_modulators_status_page);

#undef BIND_CHILD

  gtk_widget_class_set_layout_manager_type (klass, GTK_TYPE_BOX_LAYOUT);
}
