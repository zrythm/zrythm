/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
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
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/chord_pad.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/clip_editor_inner.h"
#include "gui/widgets/event_viewer.h"
#include "gui/widgets/foldable_notebook.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_editor_space.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/modulator_view.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm_app.h"

G_DEFINE_TYPE (
  BotDockEdgeWidget, bot_dock_edge_widget,
  GTK_TYPE_BOX)

static void
on_notebook_switch_page (
  GtkNotebook *       notebook,
  GtkWidget *         page,
  guint               page_num,
  BotDockEdgeWidget * self)
{
  g_debug (
    "setting bot dock page to %u", page_num);

  g_settings_set_int (
    S_UI, "bot-panel-tab", (int) page_num);
}

void
bot_dock_edge_widget_setup (
  BotDockEdgeWidget * self)
{
  foldable_notebook_widget_setup (
    self->bot_notebook,
    MW_CENTER_DOCK->center_paned,
    GTK_POS_BOTTOM);

  event_viewer_widget_setup (
    self->event_viewer,
    EVENT_VIEWER_TYPE_EDITOR);

  chord_pad_widget_setup (self->chord_pad);

  /* set event viewer visibility */
  bool visibility = false;
  ZRegion * region =
    clip_editor_get_region (CLIP_EDITOR);
  if (g_settings_get_boolean (
        S_UI, "editor-event-viewer-visible")
      && region)
    {
      visibility = true;
    }
  gtk_widget_set_visible (
    GTK_WIDGET (self->event_viewer), visibility);

  int page_num =
    g_settings_get_int (S_UI, "bot-panel-tab");
  gtk_notebook_set_current_page (
    GTK_NOTEBOOK (self->bot_notebook),
    page_num);

  g_signal_connect (
    G_OBJECT (self->bot_notebook),
    "switch-page",
    G_CALLBACK (on_notebook_switch_page), self);
}

/**
 * Brings up the clip editor.
 *
 * @param navigate_to_region_start Whether to adjust
 *   the view to start at the region start.
 */
void
bot_dock_edge_widget_show_clip_editor (
  BotDockEdgeWidget * self,
  bool                navigate_to_region_start)
{
  int num_pages =
    gtk_notebook_get_n_pages (
      GTK_NOTEBOOK (self->bot_notebook));
  for (int i = 0; i < num_pages; i++)
    {
      GtkWidget * widget =
        gtk_notebook_get_nth_page (
          GTK_NOTEBOOK (self->bot_notebook), i);
      if (widget ==
            GTK_WIDGET (self->clip_editor_box))
        {
          gtk_widget_set_visible (
            GTK_WIDGET (self), true);
          foldable_notebook_widget_set_visibility (
            self->bot_notebook, true);
          gtk_notebook_set_current_page (
            GTK_NOTEBOOK (
              MW_BOT_DOCK_EDGE->bot_notebook), i);
          break;
        }
    }

  if (navigate_to_region_start
      && CLIP_EDITOR->has_region)
    {
      ZRegion * r =
        clip_editor_get_region (CLIP_EDITOR);
      g_return_if_fail (IS_REGION_AND_NONNULL (r));
      ArrangerObject * r_obj =
        (ArrangerObject *) r;
      int px =
        ui_pos_to_px_editor (&r_obj->pos, false);
      GtkScrolledWindow * scroll =
        arranger_widget_get_scrolled_window (
          Z_ARRANGER_WIDGET (MW_MIDI_ARRANGER));
      GtkAdjustment * hadj =
        gtk_scrolled_window_get_hadjustment (scroll);
      gtk_adjustment_set_value (hadj, px);
    }
}

static void
bot_dock_edge_widget_init (BotDockEdgeWidget * self)
{
  g_type_ensure (MIXER_WIDGET_TYPE);
  g_type_ensure (CLIP_EDITOR_WIDGET_TYPE);
  g_type_ensure (MODULATOR_VIEW_WIDGET_TYPE);
  g_type_ensure (CHORD_PAD_WIDGET_TYPE);
  g_type_ensure (FOLDABLE_NOTEBOOK_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  /* set icons */
  gtk_button_set_image (
    GTK_BUTTON (self->mixer->channels_add),
    resources_get_icon (ICON_TYPE_ZRYTHM,
                        "plus.svg"));
}

static void
bot_dock_edge_widget_class_init (
  BotDockEdgeWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "bot_dock_edge.ui");

  gtk_widget_class_set_css_name (
    klass, "bot-dock-edge");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, BotDockEdgeWidget, x)

  BIND_CHILD (bot_notebook);
  BIND_CHILD (clip_editor);
  BIND_CHILD (clip_editor_box);
  BIND_CHILD (mixer);
  BIND_CHILD (mixer_box);
  BIND_CHILD (event_viewer);
  BIND_CHILD (modulator_view);
  BIND_CHILD (modulator_view_box);
  BIND_CHILD (chord_pad);
  BIND_CHILD (chord_pad_box);
}
