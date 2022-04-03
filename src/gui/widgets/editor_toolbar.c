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

#include "actions/actions.h"
#include "actions/undo_manager.h"
#include "actions/undoable_action.h"
#include "audio/audio_function.h"
#include "audio/quantize_options.h"
#include "gui/accel.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/button_with_menu.h"
#include "gui/widgets/editor_toolbar.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/playhead_scroll_buttons.h"
#include "gui/widgets/quantize_box.h"
#include "gui/widgets/snap_box.h"
#include "gui/widgets/snap_grid.h"
#include "gui/widgets/velocity_settings.h"
#include "gui/widgets/zoom_buttons.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

G_DEFINE_TYPE (
  EditorToolbarWidget,
  editor_toolbar_widget,
  GTK_TYPE_WIDGET)

static void
on_highlighting_changed (
  GtkComboBox *         widget,
  EditorToolbarWidget * self)
{
  piano_roll_set_highlighting (
    PIANO_ROLL, gtk_combo_box_get_active (widget));
}

static GtkButton *
gen_apply_funcs_btn (EditorToolbarWidget * self)
{
  const char * str = "Legato";
  GtkButton *  btn =
    GTK_BUTTON (gtk_button_new_from_icon_name (
      /*"mathmode",*/
      "code-context"));
  gtk_actionable_set_detailed_action_name (
    GTK_ACTIONABLE (btn),
    "app.editor-function::current");
  gtk_button_set_label (btn, str);
  /*gtk_button_set_always_show_image (*/
  /*self->apply_function_btn, true);*/

  return btn;
}

/**
 * Appends eligible plugins.
 */
static void
update_audio_funcs_menu (EditorToolbarWidget * self)
{
  self->audio_functions_menu =
    G_MENU_MODEL (g_menu_new ());
  for (int i = AUDIO_FUNCTION_INVERT;
       i < AUDIO_FUNCTION_CUSTOM_PLUGIN; i++)
    {
      /* not implemented yet */
      if (
        i == AUDIO_FUNCTION_NORMALIZE_RMS
        || i == AUDIO_FUNCTION_NORMALIZE_LUFS)
        continue;

      char * detailed_action =
        audio_function_get_detailed_action_for_type (
          i, "editor-toolbar.editor-function");
      GMenuItem * item = g_menu_item_new (
        audio_function_type_to_string (i),
        detailed_action);
      g_free (detailed_action);
      const char * icon_name =
        audio_function_get_icon_name_for_type (i);
      g_menu_item_set_attribute (
        item, G_MENU_ATTRIBUTE_ICON, "s", icon_name,
        NULL);
      g_menu_append_item (
        G_MENU (self->audio_functions_menu), item);
    }

    /* ignore plugins for now */
#if 0
  GMenu * plugins_menu = g_menu_new ();
  for (size_t i = 0;
       i < PLUGIN_MANAGER->plugin_descriptors->len;
       i++)
    {
      PluginDescriptor * descr =
        g_ptr_array_index (
          PLUGIN_MANAGER->plugin_descriptors, i);
      if (descr->protocol != PROT_LV2
          || !plugin_descriptor_is_effect (descr)
          || descr->num_audio_ins != 2
          || descr->num_audio_outs != 2
          || descr->category == PC_ANALYZER)
        continue;

      /* skip if open with carla by default */
      PluginSetting * setting =
        plugin_setting_new_default (descr);
      g_return_if_fail (setting);
      bool skip = false;
      if (setting->open_with_carla)
        {
          skip = true;
        }
      plugin_setting_free (setting);
      if (skip)
        continue;

      char * detailed_action =
        g_strdup_printf (
          "app.editor-function-lv2::%s", descr->uri);
      GMenuItem * item =
        g_menu_item_new (
          descr->name, detailed_action);
      g_menu_item_set_attribute (
        item, G_MENU_ATTRIBUTE_ICON,
        "s", "logo-lv2", NULL);
      g_menu_append_item (
        G_MENU (plugins_menu), item);
    }

  g_menu_append_submenu (
    G_MENU (self->audio_functions_menu),
    _("Plugin Effect"), G_MENU_MODEL (plugins_menu));
#endif

  g_menu_freeze (
    G_MENU (self->audio_functions_menu));

  self->audio_apply_function_btn =
    gen_apply_funcs_btn (self);
  button_with_menu_widget_setup (
    self->audio_functions_btn,
    self->audio_apply_function_btn, NULL, true, -1,
    gtk_button_get_label (
      self->audio_apply_function_btn),
    _ ("Select function"));
  button_with_menu_widget_set_menu_model (
    self->audio_functions_btn,
    self->audio_functions_menu);
}

static void
update_midi_funcs_menu (EditorToolbarWidget * self)
{
  self->midi_apply_function_btn =
    gen_apply_funcs_btn (self);
  button_with_menu_widget_setup (
    self->midi_functions_btn,
    self->midi_apply_function_btn, NULL, true, -1,
    gtk_button_get_label (
      self->midi_apply_function_btn),
    _ ("Select function"));
  button_with_menu_widget_set_menu_model (
    self->midi_functions_btn,
    self->midi_functions_menu);
}

static void
update_automation_funcs_menu (
  EditorToolbarWidget * self)
{
  self->automation_apply_function_btn =
    gen_apply_funcs_btn (self);
  button_with_menu_widget_setup (
    self->automation_functions_btn,
    self->automation_apply_function_btn, NULL, true,
    -1,
    gtk_button_get_label (
      self->automation_apply_function_btn),
    _ ("Select function"));
  button_with_menu_widget_set_menu_model (
    self->automation_functions_btn,
    self->automation_functions_menu);
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

  /* set visibility of each tool item */
  gtk_widget_set_visible (
    GTK_WIDGET (self->chord_highlight_box), false);
  gtk_widget_set_visible (
    GTK_WIDGET (self->sep_after_chord_highlight),
    false);

  switch (region->id.type)
    {
    case REGION_TYPE_MIDI:
      {
        gtk_stack_set_visible_child_name (
          self->functions_btn_stack, "midi-page");
        MidiFunctionType type = g_settings_get_int (
          S_UI, "midi-function");
        char * str = g_strdup_printf (
          _ ("Apply %s"),
          _ (midi_function_type_to_string (type)));
        char * tooltip_str = g_strdup_printf (
          _ ("Apply %s with previous settings"),
          _ (midi_function_type_to_string (type)));
        gtk_button_set_label (
          self->midi_apply_function_btn, str);
        gtk_widget_set_tooltip_text (
          GTK_WIDGET (self->midi_apply_function_btn),
          tooltip_str);
        g_free (str);
        g_free (tooltip_str);

        /* set visibility of each tool item */
        gtk_widget_set_visible (
          GTK_WIDGET (self->chord_highlight_box),
          true);
        gtk_widget_set_visible (
          GTK_WIDGET (
            self->sep_after_chord_highlight),
          true);
      }
      break;
    case REGION_TYPE_AUTOMATION:
      {
        gtk_stack_set_visible_child_name (
          self->functions_btn_stack,
          "automation-page");
        AutomationFunctionType type =
          g_settings_get_int (
            S_UI, "automation-function");
        char * str = g_strdup_printf (
          _ ("Apply %s"),
          _ (automation_function_type_to_string (
            type)));
        char * tooltip_str = g_strdup_printf (
          _ ("Apply %s with previous settings"),
          _ (automation_function_type_to_string (
            type)));
        gtk_button_set_label (
          self->automation_apply_function_btn, str);
        gtk_widget_set_tooltip_text (
          GTK_WIDGET (
            self->automation_apply_function_btn),
          tooltip_str);
        g_free (str);
        g_free (tooltip_str);
      }
      break;
    case REGION_TYPE_AUDIO:
      {
        gtk_stack_set_visible_child_name (
          self->functions_btn_stack, "audio-page");
        AudioFunctionType type = g_settings_get_int (
          S_UI, "audio-function");
        char * str = g_strdup_printf (
          _ ("Apply %s"),
          _ (audio_function_type_to_string (type)));
        char * tooltip_str = g_strdup_printf (
          _ ("Apply %s with previous settings"),
          _ (audio_function_type_to_string (type)));
        gtk_button_set_label (
          self->audio_apply_function_btn, str);
        gtk_widget_set_tooltip_text (
          GTK_WIDGET (self->audio_apply_function_btn),
          tooltip_str);
        g_free (str);
        g_free (tooltip_str);
      }
      break;
    default:
      /* TODO */
      gtk_stack_set_visible_child_name (
        self->functions_btn_stack, "empty-page");
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
  snap_box_widget_setup (
    self->snap_box, SNAP_GRID_EDITOR);
  quantize_box_widget_setup (
    self->quantize_box, QUANTIZE_OPTIONS_EDITOR);

  /* setup highlighting */
  GtkTreeIter    iter;
  GtkListStore * list_store = gtk_list_store_new (
    3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  gtk_list_store_append (list_store, &iter);
  gtk_list_store_set (
    list_store, &iter, 0, "draw-highlight", 1,
    _ ("No Highlight"), 2, "highlight", -1);
  gtk_list_store_append (list_store, &iter);
  gtk_list_store_set (
    list_store, &iter, 0, "draw-highlight", 1,
    _ ("Highlight Chord"), 2, "chord", -1);
  gtk_list_store_append (list_store, &iter);
  gtk_list_store_set (
    list_store, &iter, 0, "draw-highlight", 1,
    _ ("Highlight Scale"), 2, "scale", -1);
  gtk_list_store_append (list_store, &iter);
  gtk_list_store_set (
    list_store, &iter, 0, "draw-highlight", 1,
    _ ("Highlight Both"), 2, "both", -1);
  GtkComboBox * combo =
    GTK_COMBO_BOX (self->chord_highlighting);
  gtk_combo_box_set_model (
    combo, GTK_TREE_MODEL (list_store));
  gtk_combo_box_set_active (
    combo, PIANO_ROLL->highlighting);
  GtkCellRenderer * cell =
    gtk_cell_renderer_pixbuf_new ();
  gtk_cell_layout_pack_start (
    GTK_CELL_LAYOUT (combo), cell, TRUE);
  gtk_cell_layout_set_attributes (
    GTK_CELL_LAYOUT (combo), cell, "icon-name",
    ICON_NAME_COL, NULL);
  cell = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (
    GTK_CELL_LAYOUT (combo), cell, TRUE);
  gtk_cell_layout_set_attributes (
    GTK_CELL_LAYOUT (combo), cell, "text",
    LABEL_COL, NULL);

  /* setup signals */
  g_signal_connect (
    G_OBJECT (self->chord_highlighting), "changed",
    G_CALLBACK (on_highlighting_changed), self);

  editor_toolbar_widget_refresh (self);
}

static void
dispose (EditorToolbarWidget * self)
{
  gtk_widget_unparent (GTK_WIDGET (self->scroll));

  G_OBJECT_CLASS (editor_toolbar_widget_parent_class)
    ->dispose (G_OBJECT (self));
}

static void
editor_toolbar_widget_init (
  EditorToolbarWidget * self)
{
  g_type_ensure (
    PLAYHEAD_SCROLL_BUTTONS_WIDGET_TYPE);
  g_type_ensure (SNAP_BOX_WIDGET_TYPE);
  g_type_ensure (VELOCITY_SETTINGS_WIDGET_TYPE);
  g_type_ensure (ZOOM_BUTTONS_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  /* add action group wrapper */
  GSimpleActionGroup * action_group =
    g_simple_action_group_new ();
  const GActionEntry entries[] = {
    {"editor-function",
     activate_app_action_wrapper, "s"},
  };
  g_action_map_add_action_entries (
    G_ACTION_MAP (action_group), entries,
    G_N_ELEMENTS (entries), NULL);
  gtk_widget_insert_action_group (
    GTK_WIDGET (self), "editor-toolbar",
    G_ACTION_GROUP (action_group));

  gtk_actionable_set_action_name (
    GTK_ACTIONABLE (
      self->playhead_scroll->scroll_edges),
    "app.editor-playhead-scroll-edges");
  gtk_actionable_set_action_name (
    GTK_ACTIONABLE (self->playhead_scroll->follow),
    "app.editor-playhead-follow");

#define SET_TOOLTIP(x, tooltip) \
  z_gtk_set_tooltip_for_actionable ( \
    GTK_ACTIONABLE (self->x), _ (tooltip))
  /*SET_TOOLTIP (loop_selection, "Loop Selection");*/
#undef SET_TOOLTIP

  update_automation_funcs_menu (self);
  update_midi_funcs_menu (self);
  update_audio_funcs_menu (self);

  /* TODO */
#if 0
  GtkMenuButton * menu_btn =
    button_with_menu_widget_get_menu_button (
      self->functions_btn);
  gtk_menu_button_set_use_popover (menu_btn, false);
#endif

  zoom_buttons_widget_setup (
    self->zoom_buttons, false);
}

static void
editor_toolbar_widget_class_init (
  EditorToolbarWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (
    klass, "editor_toolbar.ui");

  gtk_widget_class_set_css_name (
    klass, "editor-toolbar");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, EditorToolbarWidget, x)

  BIND_CHILD (scroll);
  BIND_CHILD (chord_highlighting);
  BIND_CHILD (chord_highlight_box);
  BIND_CHILD (sep_after_chord_highlight);
  BIND_CHILD (snap_box);
  BIND_CHILD (quantize_box);
  BIND_CHILD (event_viewer_toggle);
  BIND_CHILD (automation_functions_menu);
  BIND_CHILD (midi_functions_menu);
  /*BIND_CHILD (audio_functions_menu);*/
  BIND_CHILD (functions_btn_stack);
  BIND_CHILD (audio_functions_btn);
  BIND_CHILD (midi_functions_btn);
  BIND_CHILD (automation_functions_btn);
  BIND_CHILD (velocity_settings);
  BIND_CHILD (sep_after_velocity_settings);
  BIND_CHILD (playhead_scroll);
  BIND_CHILD (zoom_buttons);

#undef BIND_CHILD

  gtk_widget_class_set_layout_manager_type (
    klass, GTK_TYPE_BIN_LAYOUT);

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->dispose = (GObjectFinalizeFunc) dispose;
}
