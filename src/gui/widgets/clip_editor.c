/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "gui/backend/clip_editor.h"
#include "gui/widgets/audio_clip_editor.h"
#include "gui/widgets/audio_ruler.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/piano_roll.h"
#include "utils/resources.h"

G_DEFINE_TYPE (ClipEditorWidget,
               clip_editor_widget,
               GTK_TYPE_STACK)

void
clip_editor_widget_setup (
  ClipEditorWidget * self,
  ClipEditor *       clip_editor)
{
  self->clip_editor = clip_editor;

  piano_roll_widget_setup (
    self->piano_roll,
    &clip_editor->piano_roll);
  audio_clip_editor_widget_setup (
    self->audio_clip_editor,
    &clip_editor->audio_clip_editor);

  gtk_stack_set_visible_child (
    GTK_STACK (self),
    GTK_WIDGET (self->no_selection_label));
}

static void
clip_editor_widget_init (ClipEditorWidget * self)
{
  gtk_widget_destroy (
    GTK_WIDGET (g_object_new (
      PIANO_ROLL_WIDGET_TYPE, NULL)));
  gtk_widget_destroy (
    GTK_WIDGET (g_object_new (
      AUDIO_CLIP_EDITOR_WIDGET_TYPE, NULL)));

  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
clip_editor_widget_class_init (
  ClipEditorWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass,
                                "clip_editor.ui");

  gtk_widget_class_set_css_name (klass,
                                 "clip-editor");

  gtk_widget_class_bind_template_child (
    klass,
    ClipEditorWidget,
    piano_roll);
  gtk_widget_class_bind_template_child (
    klass,
    ClipEditorWidget,
    audio_clip_editor);
  gtk_widget_class_bind_template_child (
    klass,
    ClipEditorWidget,
    no_selection_label);
}
