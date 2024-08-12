// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/channel.h"
#include "dsp/region.h"
#include "dsp/track.h"
#include "gui/backend/piano_roll.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/arranger_wrapper.h"
#include "gui/widgets/audio_arranger.h"
#include "gui/widgets/audio_editor_space.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_editor_space.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_arranger.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/gtk_flipper.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/zoom_buttons.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (ClipEditorInnerWidget, clip_editor_inner_widget, GTK_TYPE_WIDGET)

/**
 * Adds or remove the widget from the
 * "left of ruler" size group.
 */
void
clip_editor_inner_widget_add_to_left_of_ruler_sizegroup (
  ClipEditorInnerWidget * self,
  GtkWidget *             widget,
  bool                    add)
{
  if (add)
    {
      gtk_size_group_add_widget (self->left_of_ruler_size_group, widget);
      g_message ("%s: adding %s", __func__, gtk_widget_get_name (widget));
    }
  else
    {
      GSList * list =
        gtk_size_group_get_widgets (self->left_of_ruler_size_group);
      if (g_slist_index (list, widget) >= 0)
        {
          gtk_size_group_remove_widget (self->left_of_ruler_size_group, widget);
          g_message ("%s: removing %s", __func__, gtk_widget_get_name (widget));
        }
    }
}

ArrangerWidget *
clip_editor_inner_widget_get_visible_arranger (ClipEditorInnerWidget * self)
{
  GtkWidget * visible_w = gtk_stack_get_visible_child (self->editor_stack);
  if (visible_w == GTK_WIDGET (self->midi_editor_space))
    {
      return MW_MIDI_ARRANGER;
    }
  else if (visible_w == GTK_WIDGET (self->audio_editor_space))
    {
      return MW_AUDIO_ARRANGER;
    }
  else if (visible_w == GTK_WIDGET (self->chord_editor_space))
    {
      return MW_CHORD_ARRANGER;
    }
  else if (visible_w == GTK_WIDGET (self->automation_editor_space))
    {
      return MW_AUTOMATION_ARRANGER;
    }
  else
    {
      g_return_val_if_reached (nullptr);
    }
}

void
clip_editor_inner_widget_refresh (ClipEditorInnerWidget * self)
{
  g_message ("refreshing...");

  Region * r = CLIP_EDITOR->get_region ();
  if (!r)
    return;

  auto r_variant = convert_to_variant<RegionPtrVariant> (r);
  std::visit (
    [&] (auto &&r) {
      auto track = r->get_track ();

      color_area_widget_set_color (self->color_bar, track->color_);
      gtk_label_set_markup (self->track_name_lbl, r->name_.c_str ());

      /* remove all from the size group */
      GtkWidget * visible_w = gtk_stack_get_visible_child (self->editor_stack);
      if (visible_w == GTK_WIDGET (self->midi_editor_space))
        {
          midi_editor_space_widget_update_size_group (
            self->midi_editor_space, false);
        }
      else if (visible_w == GTK_WIDGET (self->audio_editor_space))
        {
          audio_editor_space_widget_update_size_group (
            self->audio_editor_space, false);
        }
      else if (visible_w == GTK_WIDGET (self->chord_editor_space))
        {
          chord_editor_space_widget_update_size_group (
            self->chord_editor_space, false);
        }
      else if (visible_w == GTK_WIDGET (self->automation_editor_space))
        {
          automation_editor_space_widget_update_size_group (
            self->automation_editor_space, false);
        }
      else
        {
          g_warn_if_reached ();
        }

      /* hide all left-of-ruler buttons */
      gtk_widget_set_visible (GTK_WIDGET (self->toggle_notation), false);
      gtk_widget_set_visible (GTK_WIDGET (self->toggle_listen_notes), false);
      gtk_widget_set_visible (GTK_WIDGET (self->show_automation_values), false);

      /* add one to the size group */
      if constexpr (std::is_same_v<std::decay_t<decltype (r)>, MidiRegion>)
        {
          auto pr_track = dynamic_cast<PianoRollTrack *> (track);
          gtk_stack_set_visible_child (
            self->editor_stack, GTK_WIDGET (self->midi_editor_space));
          midi_editor_space_widget_update_size_group (
            self->midi_editor_space, true);
          midi_editor_space_widget_refresh (self->midi_editor_space);
          gtk_widget_set_visible (GTK_WIDGET (self->toggle_notation), true);
          gtk_actionable_set_action_name (
            GTK_ACTIONABLE (self->toggle_notation), nullptr);
          gtk_toggle_button_set_active (
            self->toggle_notation, pr_track->drum_mode_);
          gtk_actionable_set_action_name (
            GTK_ACTIONABLE (self->toggle_notation), "app.toggle-drum-mode");
          gtk_widget_set_visible (GTK_WIDGET (self->toggle_listen_notes), true);
        }
      else if constexpr (std::is_same_v<std::decay_t<decltype (r)>, AudioRegion>)
        {
          gtk_stack_set_visible_child (
            self->editor_stack, GTK_WIDGET (MW_AUDIO_EDITOR_SPACE));
          audio_editor_space_widget_update_size_group (
            self->audio_editor_space, true);
          audio_editor_space_widget_refresh (self->audio_editor_space);
        }
      else if constexpr (std::is_same_v<std::decay_t<decltype (r)>, ChordRegion>)
        {
          gtk_stack_set_visible_child (
            self->editor_stack, GTK_WIDGET (MW_CHORD_EDITOR_SPACE));
          chord_editor_space_widget_update_size_group (
            self->chord_editor_space, true);
          chord_editor_space_widget_refresh (self->chord_editor_space);
        }
      else if constexpr (
        std::is_same_v<std::decay_t<decltype (r)>, AutomationRegion>)
        {
          gtk_stack_set_visible_child (
            self->editor_stack, GTK_WIDGET (MW_AUTOMATION_EDITOR_SPACE));
          automation_editor_space_widget_update_size_group (
            self->automation_editor_space, true);
          automation_editor_space_widget_refresh (self->automation_editor_space);
#if ZRYTHM_TARGET_VER_MAJ > 1
          gtk_widget_set_visible (
            GTK_WIDGET (self->show_automation_values), true);
#endif
        }
    },
    r_variant);
}

void
clip_editor_inner_widget_setup (ClipEditorInnerWidget * self)
{
  audio_editor_space_widget_setup (self->audio_editor_space);
  midi_editor_space_widget_setup (self->midi_editor_space);
  chord_editor_space_widget_setup (self->chord_editor_space);
  automation_editor_space_widget_setup (self->automation_editor_space);

  clip_editor_inner_widget_refresh (self);
}

static void
finalize (ClipEditorInnerWidget * self)
{
  if (self->left_of_ruler_size_group)
    g_object_unref (self->left_of_ruler_size_group);

  G_OBJECT_CLASS (clip_editor_inner_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

static void
clip_editor_inner_widget_init (ClipEditorInnerWidget * self)
{
  g_type_ensure (COLOR_AREA_WIDGET_TYPE);
  g_type_ensure (RULER_WIDGET_TYPE);
  g_type_ensure (AUDIO_EDITOR_SPACE_WIDGET_TYPE);
  g_type_ensure (MIDI_EDITOR_SPACE_WIDGET_TYPE);
  g_type_ensure (CHORD_EDITOR_SPACE_WIDGET_TYPE);
  g_type_ensure (AUTOMATION_EDITOR_SPACE_WIDGET_TYPE);
  g_type_ensure (GTK_TYPE_FLIPPER);

  self->ruler_arranger_hsize_group =
    gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  gtk_widget_init_template (GTK_WIDGET (self));

  /* add all arrangers and the ruler to the same
   * size group */
  self->ruler->type = RulerWidgetType::Editor;
  gtk_size_group_add_widget (
    self->ruler_arranger_hsize_group, GTK_WIDGET (self->ruler));
  gtk_size_group_add_widget (
    self->ruler_arranger_hsize_group,
    GTK_WIDGET (self->midi_editor_space->arranger_wrapper->child));
  gtk_size_group_add_widget (
    self->ruler_arranger_hsize_group,
    GTK_WIDGET (self->midi_editor_space->modifier_arranger));
  gtk_size_group_add_widget (
    self->ruler_arranger_hsize_group,
    GTK_WIDGET (self->audio_editor_space->arranger));
  gtk_size_group_add_widget (
    self->ruler_arranger_hsize_group,
    GTK_WIDGET (self->chord_editor_space->arranger));
  gtk_size_group_add_widget (
    self->ruler_arranger_hsize_group,
    GTK_WIDGET (self->automation_editor_space->arranger));

  self->left_of_ruler_size_group =
    gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
  gtk_size_group_add_widget (
    self->left_of_ruler_size_group, GTK_WIDGET (self->left_of_ruler_box));

  gtk_widget_set_size_request (GTK_WIDGET (self->ruler), -1, 32);

  GdkRGBA color;
  gdk_rgba_parse (&color, "gray");
  color_area_widget_set_color (self->color_bar, Color (color));

  zoom_buttons_widget_setup (
    self->zoom_buttons, false, GTK_ORIENTATION_VERTICAL);
  gtk_widget_set_visible (GTK_WIDGET (self->zoom_buttons->original_size), false);
  gtk_widget_set_visible (GTK_WIDGET (self->zoom_buttons->best_fit), false);
}

static void
clip_editor_inner_widget_class_init (ClipEditorInnerWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "clip_editor_inner.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, ClipEditorInnerWidget, x);

  BIND_CHILD (color_bar);
  BIND_CHILD (bot_of_arranger_toolbar);
  BIND_CHILD (track_name_lbl);
  BIND_CHILD (left_of_ruler_box);
  BIND_CHILD (ruler);
  BIND_CHILD (toggle_notation);
  BIND_CHILD (toggle_listen_notes);
  BIND_CHILD (show_automation_values);
  BIND_CHILD (editor_stack);
  BIND_CHILD (midi_editor_space);
  BIND_CHILD (audio_editor_space);
  BIND_CHILD (chord_editor_space);
  BIND_CHILD (automation_editor_space);
  BIND_CHILD (zoom_buttons);

#undef BIND_CHILD

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;

  gtk_widget_class_set_layout_manager_type (klass, GTK_TYPE_BOX_LAYOUT);
}
