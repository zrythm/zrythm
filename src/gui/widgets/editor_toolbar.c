/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "actions/undo_manager.h"
#include "actions/undoable_action.h"
#include "audio/quantize_options.h"
#include "gui/accel.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/button_with_menu.h"
#include "gui/widgets/editor_toolbar.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/quantize_box.h"
#include "gui/widgets/snap_grid.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  EditorToolbarWidget, editor_toolbar_widget,
  GTK_TYPE_TOOLBAR)

static void
on_highlighting_changed (
  GtkComboBox *widget,
  EditorToolbarWidget * self)
{
  piano_roll_set_highlighting (
    PIANO_ROLL,
    gtk_combo_box_get_active (widget));
}

/**
 * Refreshes relevant widgets.
 */
void
editor_toolbar_widget_refresh (
  EditorToolbarWidget * self)
{
  ZRegion * region =
    clip_editor_get_region (CLIP_EDITOR);
  if (!region)
    {
      return;
    }

  switch (region->id.type)
    {
    case REGION_TYPE_MIDI:
      {
        MidiFunctionType type =
          g_settings_get_int (S_UI, "midi-function");
        char * str =
          g_strdup_printf (
            _("Apply %s"),
          _(midi_function_type_to_string (type)));
        char * tooltip_str =
          g_strdup_printf (
            _("Apply %s with previous settings"),
          _(midi_function_type_to_string (type)));
        gtk_button_set_label (
          self->apply_function_btn, str);
        gtk_widget_set_tooltip_text (
          GTK_WIDGET (self->apply_function_btn),
          tooltip_str);
        g_free (str);
        g_free (tooltip_str);

        button_with_menu_widget_set_menu_model (
          self->functions_btn,
          self->midi_functions_menu);

        /* set visibility of each tool item */
        gtk_widget_set_visible (
          GTK_WIDGET (self->chord_highlight_tool_item),
          true);
        gtk_widget_set_visible (
          GTK_WIDGET (self->sep_after_chord_highlight),
          true);
      }
      break;
    case REGION_TYPE_AUTOMATION:
      {
        AutomationFunctionType type =
          g_settings_get_int (
            S_UI, "automation-function");
        char * str =
          g_strdup_printf (
            _("Apply %s"),
          _(automation_function_type_to_string (type)));
        char * tooltip_str =
          g_strdup_printf (
            _("Apply %s with previous settings"),
          _(automation_function_type_to_string (type)));
        gtk_button_set_label (
          self->apply_function_btn, str);
        gtk_widget_set_tooltip_text (
          GTK_WIDGET (self->apply_function_btn),
          tooltip_str);
        g_free (str);
        g_free (tooltip_str);

        button_with_menu_widget_set_menu_model (
          self->functions_btn,
          self->automation_functions_menu);

        /* set visibility of each tool item */
        gtk_widget_set_visible (
          GTK_WIDGET (self->chord_highlight_tool_item),
          false);
        gtk_widget_set_visible (
          GTK_WIDGET (self->sep_after_chord_highlight),
          false);
      }
      break;
    default:
      break;
    }
}

typedef enum HighlightColumns
{
  ICON_NAME_COL,
  LABEL_COL,
  ACTION_COL,
} HighlightColumns;

void
editor_toolbar_widget_setup (
  EditorToolbarWidget * self)
{
  /* setup bot toolbar */
  snap_grid_widget_setup (
    self->snap_grid_midi,
    &PROJECT->snap_grid_midi);
  quantize_box_widget_setup (
    self->quantize_box,
    QUANTIZE_OPTIONS_EDITOR);

  /* setup highlighting */
  GtkTreeIter iter;
  GtkListStore * list_store =
    gtk_list_store_new (
      3, G_TYPE_STRING, G_TYPE_STRING,
      G_TYPE_STRING);
  gtk_list_store_append (list_store, &iter);
  gtk_list_store_set (
    list_store, &iter,
    0, "draw-highlight",
    1, _("No Highlight"),
    2, "highlight",
    -1);
  gtk_list_store_append (list_store, &iter);
  gtk_list_store_set (
    list_store, &iter,
    0, "draw-highlight",
    1, _("Highlight Chord"),
    2, "chord",
    -1);
  gtk_list_store_append (list_store, &iter);
  gtk_list_store_set (
    list_store, &iter,
    0, "draw-highlight",
    1, _("Highlight Scale"),
    2, "scale",
    -1);
  gtk_list_store_append (list_store, &iter);
  gtk_list_store_set (
    list_store, &iter,
    0, "draw-highlight",
    1, _("Highlight Both"),
    2, "both",
    -1);
  GtkComboBox * combo =
    GTK_COMBO_BOX (self->chord_highlighting);
  gtk_combo_box_set_model (
    combo,
    GTK_TREE_MODEL (list_store));
  gtk_combo_box_set_active (
    combo,
    PIANO_ROLL->highlighting);
  GtkCellRenderer * cell =
    gtk_cell_renderer_pixbuf_new ();
  gtk_cell_layout_pack_start (
    GTK_CELL_LAYOUT (combo), cell, TRUE);
  gtk_cell_layout_set_attributes (
    GTK_CELL_LAYOUT (combo), cell,
    "icon-name", ICON_NAME_COL, NULL);
  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (
    GTK_CELL_LAYOUT (combo), cell, TRUE);
  gtk_cell_layout_set_attributes (
    GTK_CELL_LAYOUT (combo), cell,
    "text", LABEL_COL, NULL);

  /* setup signals */
  g_signal_connect (
    G_OBJECT (self->chord_highlighting), "changed",
    G_CALLBACK (on_highlighting_changed),  self);

  editor_toolbar_widget_refresh (self);
}

static void
editor_toolbar_widget_init (
  EditorToolbarWidget * self)
{
  g_type_ensure (SNAP_GRID_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

#define SET_TOOLTIP(x, tooltip) \
  z_gtk_set_tooltip_for_actionable ( \
    GTK_ACTIONABLE (self->x), \
    _(tooltip))
  /*SET_TOOLTIP (loop_selection, "Loop Selection");*/
#undef SET_TOOLTIP

  const char * str = "Legato";
  self->apply_function_btn =
    GTK_BUTTON (
      gtk_button_new_from_icon_name (
        /*"mathmode",*/
        "code-context",
        GTK_ICON_SIZE_SMALL_TOOLBAR));
  gtk_actionable_set_detailed_action_name (
    GTK_ACTIONABLE (self->apply_function_btn),
    "win.editor-function::current");
  gtk_button_set_label (
    self->apply_function_btn, str);
  gtk_button_set_always_show_image (
    self->apply_function_btn, true);
  gtk_widget_set_visible (
    GTK_WIDGET (self->apply_function_btn), true);
  button_with_menu_widget_setup (
    self->functions_btn,
    self->apply_function_btn,
    NULL, NULL, true, -1, str,
    _("Select function"));

  GtkMenuButton * menu_btn =
    button_with_menu_widget_get_menu_button (
      self->functions_btn);
  gtk_menu_button_set_use_popover (menu_btn, false);
}

static void
editor_toolbar_widget_class_init (EditorToolbarWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "editor_toolbar.ui");

  gtk_widget_class_set_css_name (
    klass, "editor-toolbar");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, EditorToolbarWidget, x)

  BIND_CHILD (chord_highlighting);
  BIND_CHILD (chord_highlight_tool_item);
  BIND_CHILD (sep_after_chord_highlight);
  BIND_CHILD (snap_grid_midi);
  BIND_CHILD (quantize_box);
  BIND_CHILD (event_viewer_toggle);
  BIND_CHILD (automation_functions_menu);
  BIND_CHILD (midi_functions_menu);
  BIND_CHILD (functions_btn);
}
