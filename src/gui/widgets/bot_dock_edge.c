// SPDX-FileCopyrightText: © 2018-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/clip_editor.h"
#include "gui/backend/editor_settings.h"
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
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (BotDockEdgeWidget, bot_dock_edge_widget, GTK_TYPE_WIDGET)

/* TODO implement after workspaces */
#if 0
static void
on_notebook_switch_page (
  GtkNotebook *       notebook,
  GtkWidget *         page,
  guint               page_num,
  BotDockEdgeWidget * self)
{
  g_debug ("setting bot dock page to %u", page_num);

  g_settings_set_int (
    S_UI, "bot-panel-tab", (int) page_num);
}
#endif

/**
 * Sets the appropriate stack page.
 */
void
bot_dock_edge_widget_update_event_viewer_stack_page (BotDockEdgeWidget * self)
{
  ZRegion * r = clip_editor_get_region (CLIP_EDITOR);
  if (r)
    {
      switch (r->id.type)
        {
        case REGION_TYPE_MIDI:
          gtk_stack_set_visible_child_name (self->event_viewer_stack, "midi");
          event_viewer_widget_refresh (self->event_viewer_midi, false);
          break;
        case REGION_TYPE_AUDIO:
          gtk_stack_set_visible_child_name (self->event_viewer_stack, "audio");
          event_viewer_widget_refresh (self->event_viewer_audio, false);
          break;
        case REGION_TYPE_CHORD:
          gtk_stack_set_visible_child_name (self->event_viewer_stack, "chord");
          event_viewer_widget_refresh (self->event_viewer_chord, false);
          break;
        case REGION_TYPE_AUTOMATION:
          gtk_stack_set_visible_child_name (
            self->event_viewer_stack, "automation");
          event_viewer_widget_refresh (self->event_viewer_automation, false);
          break;
        }
    }
}

void
bot_dock_edge_widget_setup (BotDockEdgeWidget * self)
{
  event_viewer_widget_setup (self->event_viewer_midi, EVENT_VIEWER_TYPE_MIDI);
  event_viewer_widget_setup (self->event_viewer_audio, EVENT_VIEWER_TYPE_AUDIO);
  event_viewer_widget_setup (self->event_viewer_chord, EVENT_VIEWER_TYPE_CHORD);
  event_viewer_widget_setup (
    self->event_viewer_automation, EVENT_VIEWER_TYPE_AUTOMATION);
  bot_dock_edge_widget_update_event_viewer_stack_page (self);

  chord_pad_panel_widget_setup (self->chord_pad_panel);

  /* set event viewer visibility */
  bool      visibility = false;
  ZRegion * region = clip_editor_get_region (CLIP_EDITOR);
  if (g_settings_get_boolean (S_UI, "editor-event-viewer-visible") && region)
    {
      visibility = true;
    }
  gtk_widget_set_visible (GTK_WIDGET (self->event_viewer_stack), visibility);

  /* TODO load from workspaces */
#if 0
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
#endif
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
  PanelWidget * panel_widget = PANEL_WIDGET (gtk_widget_get_ancestor (
    GTK_WIDGET (self->clip_editor_box), PANEL_TYPE_WIDGET));
  g_return_if_fail (panel_widget);
  panel_widget_raise (panel_widget);

  bot_dock_edge_widget_update_event_viewer_stack_page (self);

  if (navigate_to_region_start && CLIP_EDITOR->has_region)
    {
      clip_editor_widget_navigate_to_region_start (MW_CLIP_EDITOR, true);
    }
}

static void
dispose (BotDockEdgeWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->panel_frame));

  G_OBJECT_CLASS (bot_dock_edge_widget_parent_class)->dispose (G_OBJECT (self));
}

static void
bot_dock_edge_widget_init (BotDockEdgeWidget * self)
{
  self->panel_frame = PANEL_FRAME (panel_frame_new ());
  gtk_orientable_set_orientation (
    GTK_ORIENTABLE (self->panel_frame), GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_set_parent (GTK_WIDGET (self->panel_frame), GTK_WIDGET (self));

  PanelFrameHeader * header = panel_frame_get_header (self->panel_frame);
  header = PANEL_FRAME_HEADER (panel_frame_switcher_new ());
  panel_frame_set_header (self->panel_frame, header);

  self->clip_editor_box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
  self->clip_editor_plus_event_viewer_paned =
    GTK_PANED (gtk_paned_new (GTK_ORIENTATION_HORIZONTAL));
  gtk_box_append (
    self->clip_editor_box,
    GTK_WIDGET (self->clip_editor_plus_event_viewer_paned));
  gtk_paned_set_shrink_start_child (
    self->clip_editor_plus_event_viewer_paned, false);
  gtk_paned_set_shrink_end_child (
    self->clip_editor_plus_event_viewer_paned, false);
  gtk_paned_set_resize_end_child (
    self->clip_editor_plus_event_viewer_paned, false);
  self->clip_editor = clip_editor_widget_new ();
  gtk_paned_set_start_child (
    self->clip_editor_plus_event_viewer_paned, GTK_WIDGET (self->clip_editor));
  self->event_viewer_stack = GTK_STACK (gtk_stack_new ());
  self->event_viewer_midi = event_viewer_widget_new ();
  gtk_stack_add_named (
    self->event_viewer_stack, GTK_WIDGET (self->event_viewer_midi), "midi");
  self->event_viewer_chord = event_viewer_widget_new ();
  gtk_stack_add_named (
    self->event_viewer_stack, GTK_WIDGET (self->event_viewer_chord), "chord");
  self->event_viewer_automation = event_viewer_widget_new ();
  gtk_stack_add_named (
    self->event_viewer_stack, GTK_WIDGET (self->event_viewer_automation),
    "automation");
  self->event_viewer_audio = event_viewer_widget_new ();
  gtk_stack_add_named (
    self->event_viewer_stack, GTK_WIDGET (self->event_viewer_audio), "audio");
  gtk_paned_set_end_child (
    self->clip_editor_plus_event_viewer_paned,
    GTK_WIDGET (self->event_viewer_stack));

  self->mixer_box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
  self->mixer = mixer_widget_new ();
  gtk_box_append (self->mixer_box, GTK_WIDGET (self->mixer));

  self->modulator_view_box =
    GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
  self->modulator_view = modulator_view_widget_new ();
  gtk_box_append (self->modulator_view_box, GTK_WIDGET (self->modulator_view));

  self->chord_pad_panel_box =
    GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
  self->chord_pad_panel = chord_pad_panel_widget_new ();
  gtk_box_append (self->chord_pad_panel_box, GTK_WIDGET (self->chord_pad_panel));

  /* add tabs */
#define ADD_TAB(widget, icon, title) \
  { \
    PanelWidget * panel_widget = PANEL_WIDGET (panel_widget_new ()); \
    panel_widget_set_child (panel_widget, GTK_WIDGET (widget)); \
    panel_widget_set_icon_name (panel_widget, icon); \
    panel_widget_set_title (panel_widget, title); \
    panel_frame_add (self->panel_frame, panel_widget); \
  }

  ADD_TAB (self->clip_editor_box, "piano-roll", _ ("Editor view"));
  ADD_TAB (self->mixer_box, "mixer", _ ("Mixer view"));
  ADD_TAB (self->modulator_view_box, "modulator", _ ("Modulators"));
  ADD_TAB (self->chord_pad_panel_box, "chord-pad", _ ("Chord pad"));

#undef ADD_TAB
}

static void
bot_dock_edge_widget_class_init (BotDockEdgeWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (klass, "bot-dock-edge");

  gtk_widget_class_set_layout_manager_type (klass, GTK_TYPE_BIN_LAYOUT);

  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}
