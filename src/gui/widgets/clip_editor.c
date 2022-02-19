/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "gui/backend/clip_editor.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/automation_arranger.h"
#include "gui/widgets/automation_editor_space.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_arranger.h"
#include "gui/widgets/chord_editor_space.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/editor_toolbar.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/ruler.h"
#include "project.h"
#include "utils/resources.h"
#include "zrythm_app.h"

G_DEFINE_TYPE (
  ClipEditorWidget, clip_editor_widget,
  GTK_TYPE_BOX)

void
clip_editor_widget_setup (
  ClipEditorWidget * self)
{
  clip_editor_inner_widget_setup (
    self->clip_editor_inner);

  gtk_stack_set_visible_child (
    GTK_STACK (self->stack),
    GTK_WIDGET (self->no_selection_page));
}

/**
 * Called when setting clip editor region using
 * g_idle_add to give the widgets time to have
 * widths.
 */
static int
refresh_editor_ruler_and_arranger (
  void * user_data)
{
  if (!ui_is_widget_revealed (EDITOR_RULER))
    {
      return TRUE;
    }

  /* ruler must be refreshed first to get the
   * correct px when calling ui_* functions */
  ruler_widget_refresh (
    (RulerWidget *) EDITOR_RULER);

  CLIP_EDITOR->region_changed = 0;

  return FALSE;
}

/**
 * To be called when the region changes.
 */
void
clip_editor_widget_on_region_changed (
  ClipEditorWidget * self)
{
  ZRegion * r =
    clip_editor_get_region (CLIP_EDITOR);

  if (r)
    {
      gtk_stack_set_visible_child (
        GTK_STACK (self->stack),
        GTK_WIDGET (self->main_box));

      clip_editor_inner_widget_refresh (
        MW_CLIP_EDITOR_INNER);

      g_idle_add (
        refresh_editor_ruler_and_arranger,
        NULL);

      /* update the toolbar */
      editor_toolbar_widget_refresh (
        self->editor_toolbar);
    }
  else
    {
      gtk_stack_set_visible_child (
        GTK_STACK (self->stack),
        GTK_WIDGET (self->no_selection_page));
    }
}

ClipEditorWidget *
clip_editor_widget_new (void)
{
  ClipEditorWidget * self =
    g_object_new (
      CLIP_EDITOR_WIDGET_TYPE, NULL);

  return self;
}

static void
clip_editor_widget_init (ClipEditorWidget * self)
{
  g_type_ensure (CLIP_EDITOR_INNER_WIDGET_TYPE);
  g_type_ensure (EDITOR_TOOLBAR_WIDGET_TYPE);
  g_type_ensure (ADW_TYPE_STATUS_PAGE);

  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
clip_editor_widget_class_init (
  ClipEditorWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "clip_editor.ui");

  gtk_widget_class_set_css_name (
    klass, "clip-editor");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, ClipEditorWidget, x)

  BIND_CHILD (stack);
  BIND_CHILD (main_box);
  BIND_CHILD (editor_toolbar);
  BIND_CHILD (clip_editor_inner);
  BIND_CHILD (no_selection_page);

#undef BIND_CHILD
}
