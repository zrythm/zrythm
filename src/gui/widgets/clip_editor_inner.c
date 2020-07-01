/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "audio/channel.h"
#include "audio/region.h"
#include "audio/track.h"
#include "gui/backend/piano_roll.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/audio_editor_space.h"
#include "gui/widgets/automation_editor_space.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/ruler.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  ClipEditorInnerWidget,
  clip_editor_inner_widget,
  GTK_TYPE_BOX)

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
      gtk_size_group_add_widget (
        self->left_of_ruler_size_group, widget);
      g_message ("%s: adding %s",
        __func__,
        gtk_widget_get_name (widget));
    }
  else
    {
      GSList * list =
        gtk_size_group_get_widgets (
          self->left_of_ruler_size_group);
      if (g_slist_index (list, widget) >= 0)
        {
          gtk_size_group_remove_widget (
            self->left_of_ruler_size_group, widget);
          g_message ("%s: removing %s",
            __func__,
            gtk_widget_get_name (widget));
        }
    }
}

void
clip_editor_inner_widget_refresh (
  ClipEditorInnerWidget * self)
{
  g_message ("refreshing...");

  ZRegion * r =
    clip_editor_get_region (CLIP_EDITOR);
  ArrangerObject * r_obj =
    (ArrangerObject *) r;
  Track * track = NULL;

  if (r)
    {
      track = arranger_object_get_track (r_obj);

      color_area_widget_set_color (
        self->color_bar,
        &track->color);
      gtk_label_set_text (
        self->track_name_label,
        track_get_name (track));

      /* remove all from the size group */
      GtkWidget * visible_w =
        gtk_stack_get_visible_child (
          self->editor_stack);
      if (visible_w ==
          GTK_WIDGET (self->midi_editor_space))
        {
          midi_editor_space_widget_update_size_group (
            self->midi_editor_space, 0);
        }
      else if (visible_w ==
          GTK_WIDGET (self->audio_editor_space))
        {
          audio_editor_space_widget_update_size_group (
            self->audio_editor_space, 0);
        }
      else if (visible_w ==
          GTK_WIDGET (self->chord_editor_space))
        {
          chord_editor_space_widget_update_size_group (
            self->chord_editor_space, 0);
        }
      else if (visible_w ==
          GTK_WIDGET (self->automation_editor_space))
        {
          automation_editor_space_widget_update_size_group (
            self->automation_editor_space, 0);
        }
      else
        {
          g_warn_if_reached ();
        }

      /* add one to the size group */
      switch (r->id.type)
        {
        case REGION_TYPE_MIDI:
          gtk_stack_set_visible_child (
            self->editor_stack,
            GTK_WIDGET (self->midi_editor_space));
          midi_editor_space_widget_update_size_group (
            self->midi_editor_space, 1);
          midi_editor_space_widget_refresh (
            self->midi_editor_space);
          break;
        case REGION_TYPE_AUDIO:
          gtk_stack_set_visible_child (
            self->editor_stack,
            GTK_WIDGET (MW_AUDIO_EDITOR_SPACE));
          audio_editor_space_widget_update_size_group (
            self->audio_editor_space, 1);
          audio_editor_space_widget_refresh (
            self->audio_editor_space);
          break;
        case REGION_TYPE_CHORD:
          gtk_stack_set_visible_child (
            self->editor_stack,
            GTK_WIDGET (MW_CHORD_EDITOR_SPACE));
          chord_editor_space_widget_update_size_group (
            self->chord_editor_space, 1);
          chord_editor_space_widget_refresh (
            self->chord_editor_space);
          break;
        case REGION_TYPE_AUTOMATION:
          gtk_stack_set_visible_child (
            self->editor_stack,
            GTK_WIDGET (
              MW_AUTOMATION_EDITOR_SPACE));
          automation_editor_space_widget_update_size_group (
            self->automation_editor_space, 1);
          automation_editor_space_widget_refresh (
            self->automation_editor_space);
          break;
        }
    }

  g_message ("done");
}

void
clip_editor_inner_widget_setup (
  ClipEditorInnerWidget * self)
{
  audio_editor_space_widget_setup (
    self->audio_editor_space);
  midi_editor_space_widget_setup (
    self->midi_editor_space);
  chord_editor_space_widget_setup (
    self->chord_editor_space);
  automation_editor_space_widget_setup (
    self->automation_editor_space);

  clip_editor_inner_widget_refresh (self);
}

static void
clip_editor_inner_widget_init (
  ClipEditorInnerWidget * self)
{
  g_type_ensure (COLOR_AREA_WIDGET_TYPE);
  g_type_ensure (RULER_WIDGET_TYPE);
  g_type_ensure (AUDIO_EDITOR_SPACE_WIDGET_TYPE);
  g_type_ensure (MIDI_EDITOR_SPACE_WIDGET_TYPE);
  g_type_ensure (CHORD_EDITOR_SPACE_WIDGET_TYPE);
  g_type_ensure (
    AUTOMATION_EDITOR_SPACE_WIDGET_TYPE);

  self->ruler_arranger_hsize_group =
    gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

  gtk_widget_init_template (GTK_WIDGET (self));

  /* add all arrangers and the ruler to the same
   * size group */
  self->ruler->type =
    RULER_WIDGET_TYPE_EDITOR;
  gtk_size_group_add_widget (
    self->ruler_arranger_hsize_group,
    GTK_WIDGET (self->ruler));
  gtk_size_group_add_widget (
    self->ruler_arranger_hsize_group,
    GTK_WIDGET (self->midi_editor_space->arranger));
  gtk_size_group_add_widget (
    self->ruler_arranger_hsize_group,
    GTK_WIDGET (
      self->midi_editor_space->modifier_arranger));
  gtk_size_group_add_widget (
    self->ruler_arranger_hsize_group,
    GTK_WIDGET (self->audio_editor_space->arranger));
  gtk_size_group_add_widget (
    self->ruler_arranger_hsize_group,
    GTK_WIDGET (self->chord_editor_space->arranger));
  gtk_size_group_add_widget (
    self->ruler_arranger_hsize_group,
    GTK_WIDGET (
      self->automation_editor_space->arranger));

  gtk_label_set_text (
    self->track_name_label,
    _("Select a region..."));

  self->left_of_ruler_size_group =
    gtk_size_group_new (
      GTK_SIZE_GROUP_HORIZONTAL);
  gtk_size_group_add_widget (
    self->left_of_ruler_size_group,
    GTK_WIDGET (self->left_of_ruler_box));

  gtk_widget_set_size_request (
    GTK_WIDGET (self->ruler_scroll), -1, 32);

  GdkRGBA color;
  gdk_rgba_parse (&color, "gray");
  color_area_widget_set_color (
    self->color_bar, &color);
}

static void
finalize (
  ClipEditorInnerWidget * self)
{
  if (self->left_of_ruler_size_group)
    g_object_unref (self->left_of_ruler_size_group);

  G_OBJECT_CLASS (
    clip_editor_inner_widget_parent_class)->
      finalize (G_OBJECT (self));
}

static void
clip_editor_inner_widget_class_init (
  ClipEditorInnerWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass,
    "clip_editor_inner.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, \
    ClipEditorInnerWidget, \
    x);

  BIND_CHILD (color_bar);
  BIND_CHILD (bot_of_arranger_toolbar);
  BIND_CHILD (track_name_label);
  BIND_CHILD (left_of_ruler_box);
  BIND_CHILD (ruler_scroll);
  BIND_CHILD (ruler_viewport);
  BIND_CHILD (ruler);
  BIND_CHILD (toggle_notation);
  BIND_CHILD (editor_stack);
  BIND_CHILD (midi_editor_space);
  BIND_CHILD (audio_editor_space);
  BIND_CHILD (chord_editor_space);
  BIND_CHILD (automation_editor_space);

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;
}
