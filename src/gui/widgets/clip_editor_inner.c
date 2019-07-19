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
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/piano_roll_key.h"
#include "gui/widgets/piano_roll_key_label.h"
#include "gui/widgets/ruler.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  ClipEditorInnerWidget,
  clip_editor_inner_widget,
  GTK_TYPE_BOX)

void
clip_editor_inner_widget_refresh (
  ClipEditorInnerWidget * self)
{
}

void
clip_editor_inner_widget_setup (
  ClipEditorInnerWidget * self)
{
  audio_editor_space_widget_setup (
    self->audio_editor_space);
  midi_editor_space_widget_setup (
    self->midi_editor_space);

  clip_editor_inner_widget_refresh (self);
}

static void
clip_editor_inner_widget_init (ClipEditorInnerWidget * self)
{
  g_type_ensure (COLOR_AREA_WIDGET_TYPE);
  g_type_ensure (EDITOR_RULER_WIDGET_TYPE);
  g_type_ensure (AUDIO_EDITOR_SPACE_WIDGET_TYPE);
  g_type_ensure (MIDI_EDITOR_SPACE_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_label_set_text (
    self->track_name_label,
    _("Select a region..."));

  self->left_of_ruler_size_group =
    gtk_size_group_new (
      GTK_SIZE_GROUP_HORIZONTAL);
  gtk_size_group_add_widget (
    self->left_of_ruler_size_group,
    GTK_WIDGET (self->left_of_ruler_box));

  GdkRGBA * color = calloc (1, sizeof (GdkRGBA));
  gdk_rgba_parse (color, "gray");
  color_area_widget_set_color (
    self->color_bar, color);
}

static void
clip_editor_inner_widget_class_init (
  ClipEditorInnerWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass,
    "clip_editor_inner.ui");

  gtk_widget_class_bind_template_child (
    klass,
    ClipEditorInnerWidget,
    color_bar);
  gtk_widget_class_bind_template_child (
    klass,
    ClipEditorInnerWidget,
    bot_of_arranger_toolbar);
  gtk_widget_class_bind_template_child (
    klass,
    ClipEditorInnerWidget,
    track_name_label);
  gtk_widget_class_bind_template_child (
    klass,
    ClipEditorInnerWidget,
    left_of_ruler_box);
  gtk_widget_class_bind_template_child (
    klass,
    ClipEditorInnerWidget,
    ruler_scroll);
  gtk_widget_class_bind_template_child (
    klass,
    ClipEditorInnerWidget,
    ruler_viewport);
  gtk_widget_class_bind_template_child (
    klass,
    ClipEditorInnerWidget,
    ruler);
  gtk_widget_class_bind_template_child (
    klass,
    ClipEditorInnerWidget,
    toggle_notation);
  gtk_widget_class_bind_template_child (
    klass,
    ClipEditorInnerWidget,
    editor_stack);
  gtk_widget_class_bind_template_child (
    klass,
    ClipEditorInnerWidget,
    midi_editor_space);
  gtk_widget_class_bind_template_child (
    klass,
    ClipEditorInnerWidget,
    audio_editor_space);
}
