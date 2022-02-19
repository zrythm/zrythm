/*
 * Copyright (C) 2018-2022 Alexandros Theodotou <alex at zrythm dot org>
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
#include "gui/widgets/chord_pad_panel.h"
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

#include <glib/gi18n.h>

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

/**
 * Sets the appropriate stack page.
 */
void
bot_dock_edge_widget_update_event_viewer_stack_page (
  BotDockEdgeWidget * self)
{
  ZRegion * r =
    clip_editor_get_region (CLIP_EDITOR);
  if (r)
    {
      switch (r->id.type)
        {
        case REGION_TYPE_MIDI:
          gtk_stack_set_visible_child_name (
            self->event_viewer_stack, "midi");
          event_viewer_widget_refresh (
            self->event_viewer_midi, false);
          break;
        case REGION_TYPE_AUDIO:
          gtk_stack_set_visible_child_name (
            self->event_viewer_stack, "audio");
          event_viewer_widget_refresh (
            self->event_viewer_audio, false);
          break;
        case REGION_TYPE_CHORD:
          gtk_stack_set_visible_child_name (
            self->event_viewer_stack, "chord");
          event_viewer_widget_refresh (
            self->event_viewer_chord, false);
          break;
        case REGION_TYPE_AUTOMATION:
          gtk_stack_set_visible_child_name (
            self->event_viewer_stack, "automation");
          event_viewer_widget_refresh (
            self->event_viewer_automation, false);
          break;
        }
    }
}

void
bot_dock_edge_widget_setup (
  BotDockEdgeWidget * self)
{
  foldable_notebook_widget_setup (
    self->bot_notebook,
    MW_CENTER_DOCK->center_paned,
    GTK_POS_BOTTOM, true);

  event_viewer_widget_setup (
    self->event_viewer_midi,
    EVENT_VIEWER_TYPE_MIDI);
  event_viewer_widget_setup (
    self->event_viewer_audio,
    EVENT_VIEWER_TYPE_AUDIO);
  event_viewer_widget_setup (
    self->event_viewer_chord,
    EVENT_VIEWER_TYPE_CHORD);
  event_viewer_widget_setup (
    self->event_viewer_automation,
    EVENT_VIEWER_TYPE_AUTOMATION);
  bot_dock_edge_widget_update_event_viewer_stack_page (
    self);

  chord_pad_panel_widget_setup (
    self->chord_pad_panel);

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
    GTK_WIDGET (self->event_viewer_stack),
    visibility);

  GtkNotebook * notebook =
    foldable_notebook_widget_get_notebook (
      self->bot_notebook);
  int page_num =
    g_settings_get_int (S_UI, "bot-panel-tab");
  foldable_notebook_widget_set_current_page (
    self->bot_notebook, page_num, true);

  g_signal_connect (
    notebook, "switch-page",
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
  GtkNotebook * notebook =
    foldable_notebook_widget_get_notebook (
      self->bot_notebook);

  int num_pages =
    gtk_notebook_get_n_pages (notebook);
  for (int i = 0; i < num_pages; i++)
    {
      GtkWidget * widget =
        gtk_notebook_get_nth_page (notebook, i);
      if (widget ==
            GTK_WIDGET (self->clip_editor_box))
        {
          gtk_widget_set_visible (
            GTK_WIDGET (self), true);
          foldable_notebook_widget_set_visibility (
            self->bot_notebook, true);
          gtk_notebook_set_current_page (
            notebook, i);
          break;
        }
    }

  bot_dock_edge_widget_update_event_viewer_stack_page (
    self);

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
generate_bot_notebook (
  BotDockEdgeWidget * self)
{
  self->bot_notebook =
    foldable_notebook_widget_new (
      GTK_POS_BOTTOM, true);
  gtk_box_append (
    GTK_BOX (self),
    GTK_WIDGET (self->bot_notebook));

  self->clip_editor_box =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
  self->clip_editor_plus_event_viewer_paned =
    GTK_PANED (
      gtk_paned_new (GTK_ORIENTATION_HORIZONTAL));
  gtk_box_append (
    self->clip_editor_box,
    GTK_WIDGET (
      self->clip_editor_plus_event_viewer_paned));
  gtk_paned_set_shrink_start_child (
    self->clip_editor_plus_event_viewer_paned, false);
  gtk_paned_set_shrink_end_child (
    self->clip_editor_plus_event_viewer_paned, false);
  gtk_paned_set_resize_end_child (
    self->clip_editor_plus_event_viewer_paned, false);
  self->clip_editor = clip_editor_widget_new ();
  gtk_paned_set_start_child (
    self->clip_editor_plus_event_viewer_paned,
    GTK_WIDGET (self->clip_editor));
  self->event_viewer_stack =
    GTK_STACK (gtk_stack_new ());
  self->event_viewer_midi =
    event_viewer_widget_new ();
  gtk_stack_add_named (
    self->event_viewer_stack,
    GTK_WIDGET (self->event_viewer_midi), "midi");
  self->event_viewer_chord =
    event_viewer_widget_new ();
  gtk_stack_add_named (
    self->event_viewer_stack,
    GTK_WIDGET (self->event_viewer_chord), "chord");
  self->event_viewer_automation =
    event_viewer_widget_new ();
  gtk_stack_add_named (
    self->event_viewer_stack,
    GTK_WIDGET (self->event_viewer_automation),
    "automation");
  self->event_viewer_audio =
    event_viewer_widget_new ();
  gtk_stack_add_named (
    self->event_viewer_stack,
    GTK_WIDGET (self->event_viewer_audio), "audio");
  gtk_paned_set_end_child (
    self->clip_editor_plus_event_viewer_paned,
    GTK_WIDGET (self->event_viewer_stack));

  self->mixer_box =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
  self->mixer = mixer_widget_new ();
  gtk_box_append (
    self->mixer_box, GTK_WIDGET (self->mixer));

  self->modulator_view_box =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
  self->modulator_view =
    modulator_view_widget_new ();
  gtk_box_append (
    self->modulator_view_box,
    GTK_WIDGET (self->modulator_view));

  self->chord_pad_panel_box =
    GTK_BOX (
      gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
  self->chord_pad_panel =
    chord_pad_panel_widget_new ();
  gtk_box_append (
    self->chord_pad_panel_box,
    GTK_WIDGET (self->chord_pad_panel));

  /* add tabs */
  foldable_notebook_widget_add_page (
    self->bot_notebook,
    GTK_WIDGET (self->clip_editor_box),
    "piano-roll", _("Editor"), _("Editor view"));
  foldable_notebook_widget_add_page (
    self->bot_notebook,
    GTK_WIDGET (self->mixer_box),
    "mixer", _("Mixer"), _("Mixer view"));
  foldable_notebook_widget_add_page (
    self->bot_notebook,
    GTK_WIDGET (self->modulator_view_box),
    "modulator", _("Modulators"), _("Modulators"));
  foldable_notebook_widget_add_page (
    self->bot_notebook,
    GTK_WIDGET (self->chord_pad_panel_box),
    "chord-pad", _("Chord Pad"), _("Chord pad"));

  GtkNotebook * notebook =
    foldable_notebook_widget_get_notebook (
      self->bot_notebook);
  self->toggle_top_panel =
    GTK_BUTTON (
      gtk_button_new_from_icon_name (
        "gnome-builder-builder-"
        "view-top-pane-symbolic-light"));
  gtk_widget_set_tooltip_text (
    GTK_WIDGET (self->toggle_top_panel),
    _("Show/hide top panel"));
  gtk_actionable_set_action_name (
    GTK_ACTIONABLE (self->toggle_top_panel),
    "app.toggle-top-panel");
  gtk_notebook_set_action_widget (
    notebook, GTK_WIDGET (self->toggle_top_panel),
    GTK_PACK_END);

  gtk_notebook_set_tab_pos (
    notebook, GTK_POS_BOTTOM);
}

static void
bot_dock_edge_widget_init (
  BotDockEdgeWidget * self)
{
  gtk_orientable_set_orientation (
    GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);

  generate_bot_notebook (self);

  /* set icons */
  gtk_button_set_icon_name (
    GTK_BUTTON (self->mixer->channels_add), "add");
}

static void
bot_dock_edge_widget_class_init (
  BotDockEdgeWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "bot-dock-edge");
}
