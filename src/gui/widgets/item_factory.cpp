// SPDX-FileCopyrightText: © 2021-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/channel_send.h"
#include "dsp/midi_mapping.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/backend/file_manager.h"
#include "gui/backend/wrapped_object_with_change_signal.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/digital_meter.h"
#include "gui/widgets/greeter.h"
#include "gui/widgets/item_factory.h"
#include "gui/widgets/popovers/popover_menu_bin.h"
#include "plugins/collection.h"
#include "plugins/collections.h"
#include "plugins/plugin_manager.h"
#include "project.h"
#include "settings/chord_preset_pack_manager.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

/**
 * Data for individual widget callbacks.
 */
typedef struct ItemFactoryData
{
  ItemFactory *                   factory;
  WrappedObjectWithChangeSignal * obj;
} ItemFactoryData;

static ItemFactoryData *
item_factory_data_new (ItemFactory * factory, WrappedObjectWithChangeSignal * obj)
{
  ItemFactoryData * self = object_new (ItemFactoryData);
  self->factory = factory;
  self->obj = obj;

  return self;
}

static void
item_factory_data_destroy_closure (gpointer user_data, GClosure * closure)
{
  ItemFactoryData * data = (ItemFactoryData *) user_data;

  object_zero_and_free (data);
}

static void
on_toggled (GtkCheckButton * self, gpointer user_data)
{
  ItemFactoryData * data = (ItemFactoryData *) user_data;
  ItemFactory *     factory = data->factory;

  WrappedObjectWithChangeSignal * obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data->obj);

  bool active = gtk_check_button_get_active (self);

  switch (obj->type)
    {
    case WrappedObjectType::WRAPPED_OBJECT_TYPE_MIDI_MAPPING:
      {
        MidiMapping * mapping = (MidiMapping *) obj->obj;
        int      mapping_idx = midi_mapping_get_index (MIDI_MAPPINGS, mapping);
        GError * err = NULL;
        bool     ret =
          midi_mapping_action_perform_enable (mapping_idx, active, &err);
        if (!ret)
          {
            HANDLE_ERROR (err, "%s", _ ("Failed to enable binding"));
          }
      }
      break;
    case WrappedObjectType::WRAPPED_OBJECT_TYPE_TRACK:
      {
        Track * track = (Track *) obj->obj;
        if (string_is_equal (factory->column_name, _ ("Visibility")))
          {
            track->visible = active;
            EVENTS_PUSH (EventType::ET_TRACK_VISIBILITY_CHANGED, track);
          }
      }
      break;
    default:
      break;
    }
}

#if 0
static void
on_list_item_selected (
  GObject    * gobject,
  GParamSpec * pspec,
  gpointer     user_data)
{
  GtkListItem * list_item = GTK_LIST_ITEM (gobject);
  GtkWidget * child =
    gtk_list_item_get_child (list_item);

  if (gtk_list_item_get_selected (list_item))
    gtk_widget_add_css_class (child, "selected");
  else
    gtk_widget_remove_css_class (child, "selected");
}
#endif

static GdkContentProvider *
on_dnd_drag_prepare (GtkDragSource * source, double x, double y, gpointer user_data)
{
  ItemFactoryData * data = (ItemFactoryData *) user_data;

  g_debug ("dnd prepare from item factory");

  g_return_val_if_fail (
    Z_IS_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data->obj), NULL);
  GdkContentProvider * content_providers[] = {
    gdk_content_provider_new_typed (
      WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE, data->obj),
  };

  return gdk_content_provider_new_union (
    content_providers, G_N_ELEMENTS (content_providers));
}

typedef struct EditableChangedInfo
{
  ArrangerObject * obj;
  char *           column_name;
  char *           text;
} EditableChangedInfo;

static void
editable_changed_info_free (gpointer data)
{
  EditableChangedInfo * self = (EditableChangedInfo *) data;
  g_free_and_null (self->column_name);
  g_free_and_null (self->text);
  object_zero_and_free (self);
}

static void
handle_arranger_object_position_change (
  ArrangerObject *           obj,
  ArrangerObject *           prev_obj,
  const char *               text,
  ArrangerObjectPositionType type)
{
  Position pos;
  bool     ret = position_parse (&pos, text);
  if (ret)
    {
      position_print (&pos);
      Position prev_pos;
      arranger_object_get_position_from_type (obj, &prev_pos, type);
      if (arranger_object_is_position_valid (obj, &pos, type))
        {
          if (!position_is_equal (&pos, &prev_pos))
            {
              arranger_object_set_position (obj, &pos, type, F_NO_VALIDATE);
              arranger_object_edit_position_finish (obj);
            }
        }
      else
        {
          ui_show_error_message (_ ("Invalid Position"), _ ("Invalid position"));
        }
    }
  else
    {
      ui_show_error_message (
        _ ("Invalid Position"), _ ("Failed to parse position"));
    }
}

/**
 * Source function for performing changes
 * later.
 */
static int
editable_label_changed_source (gpointer user_data)
{
  EditableChangedInfo * self = (EditableChangedInfo *) user_data;
  const char *          column_name = self->column_name;
  const char *          text = self->text;
  ArrangerObject *      obj = self->obj;
  ArrangerWidget *      arranger = arranger_object_get_arranger (obj);
  ArrangerObject *      prev_obj = arranger->start_object;
  if (
    string_is_equal (column_name, _ ("Start"))
    || string_is_equal (column_name, _ ("Position")))
    {
      handle_arranger_object_position_change (
        obj, prev_obj, text,
        ArrangerObjectPositionType::ARRANGER_OBJECT_POSITION_TYPE_START);
    }
  else if (string_is_equal (column_name, _ ("End")))
    {
      handle_arranger_object_position_change (
        obj, prev_obj, text,
        ArrangerObjectPositionType::ARRANGER_OBJECT_POSITION_TYPE_END);
    }
  else if (string_is_equal (column_name, _ ("Clip start")))
    {
      handle_arranger_object_position_change (
        obj, prev_obj, text,
        ArrangerObjectPositionType::ARRANGER_OBJECT_POSITION_TYPE_CLIP_START);
    }
  else if (string_is_equal (column_name, _ ("Loop start")))
    {
      handle_arranger_object_position_change (
        obj, prev_obj, text,
        ArrangerObjectPositionType::ARRANGER_OBJECT_POSITION_TYPE_LOOP_START);
    }
  else if (string_is_equal (column_name, _ ("Loop end")))
    {
      handle_arranger_object_position_change (
        obj, prev_obj, text,
        ArrangerObjectPositionType::ARRANGER_OBJECT_POSITION_TYPE_LOOP_END);
    }
  else if (string_is_equal (column_name, _ ("Fade in")))
    {
      handle_arranger_object_position_change (
        obj, prev_obj, text,
        ArrangerObjectPositionType::ARRANGER_OBJECT_POSITION_TYPE_FADE_IN);
    }
  else if (string_is_equal (column_name, _ ("Fade out")))
    {
      handle_arranger_object_position_change (
        obj, prev_obj, text,
        ArrangerObjectPositionType::ARRANGER_OBJECT_POSITION_TYPE_FADE_OUT);
    }
  else if (string_is_equal (column_name, _ ("Name")))
    {
      if (arranger_object_validate_name (obj, text))
        {
          arranger_object_set_name (obj, text, F_NO_PUBLISH_EVENTS);
          arranger_object_edit_finish (
            obj,
            ArrangerSelectionsActionEditType::
              ARRANGER_SELECTIONS_ACTION_EDIT_NAME);
        }
      else
        {
          ui_show_error_message (_ ("Invalid Name"), _ ("Invalid name"));
        }
    }
  else if (string_is_equal (column_name, _ ("Velocity")))
    {
      uint8_t val;
      int     res = sscanf (text, "%hhu", &val);
      if (res != 1 || res == EOF || val < 1 || val > 127)
        {
          ui_show_error_message (_ ("Invalid Velocity"), _ ("Invalid velocity"));
        }
      else
        {
          MidiNote * mn = (MidiNote *) obj;
          if (mn->vel->vel != val)
            {
              mn->vel->vel = val;
              arranger_object_edit_finish (
                obj,
                ArrangerSelectionsActionEditType::
                  ARRANGER_SELECTIONS_ACTION_EDIT_PRIMITIVE);
            }
        }
    }
  else if (string_is_equal (column_name, _ ("Pitch")))
    {
      uint8_t val;
      int     res = sscanf (text, "%hhu", &val);
      if (res != 1 || res == EOF || val < 1 || val > 127)
        {
          ui_show_error_message (_ ("Invalid Pitch"), _ ("Invalid pitch"));
        }
      else
        {
          MidiNote * mn = (MidiNote *) obj;
          if (mn->val != val)
            {
              midi_note_set_val (mn, val);
              arranger_object_edit_finish (
                obj,
                ArrangerSelectionsActionEditType::
                  ARRANGER_SELECTIONS_ACTION_EDIT_PRIMITIVE);
            }
        }
    }
  else if (string_is_equal (column_name, _ ("Value")))
    {
      AutomationPoint * ap = (AutomationPoint *) obj;
      Port *            port = automation_point_get_port (ap);

      float val;
      int   res = sscanf (text, "%f", &val);
      if (res != 1 || res == EOF || val < port->minf || val > port->maxf)
        {
          ui_show_error_message (_ ("Invalid Value"), _ ("Invalid value"));
        }
      else
        {
          AutomationPoint * prev_ap = (AutomationPoint *) prev_obj;
          if (!math_floats_equal (prev_ap->fvalue, val))
            {
              automation_point_set_fvalue (
                ap, val, F_NOT_NORMALIZED, F_NO_PUBLISH_EVENTS);
              arranger_object_edit_finish (
                obj,
                ArrangerSelectionsActionEditType::
                  ARRANGER_SELECTIONS_ACTION_EDIT_PRIMITIVE);
            }
        }
    }
  else if (string_is_equal (column_name, _ ("Curviness")))
    {
      float val;
      int   res = sscanf (text, "%f", &val);
      if (res != 1 || res == EOF || val < -1.f || val > 1.f)
        {
          ui_show_error_message (_ ("Invalid Value"), _ ("Invalid value"));
        }
      else
        {
          AutomationPoint * prev_ap = (AutomationPoint *) prev_obj;
          AutomationPoint * ap = (AutomationPoint *) obj;
          ap->curve_opts.curviness = val;
          if (!curve_options_are_equal (&prev_ap->curve_opts, &ap->curve_opts))
            {
              arranger_object_edit_finish (
                obj,
                ArrangerSelectionsActionEditType::
                  ARRANGER_SELECTIONS_ACTION_EDIT_PRIMITIVE);
            }
        }
    }

  /* make sure temp object is freed */
  object_free_w_func_and_null (arranger_object_free, arranger->start_object);

  return G_SOURCE_REMOVE;
}

static void
on_editable_label_editing_changed (
  GObject *    gobject,
  GParamSpec * pspec,
  gpointer     user_data)
{
  ItemFactoryData * data = (ItemFactoryData *) user_data;

  GtkEditableLabel * editable_lbl = GTK_EDITABLE_LABEL (gobject);
  const char *       text = gtk_editable_get_text (GTK_EDITABLE (editable_lbl));

  bool editing = gtk_editable_label_get_editing (editable_lbl);

  g_debug ("editing changed: %d", editing);
  g_debug ("text: %s", text);

  /* perform change */
  WrappedObjectWithChangeSignal * wrapped_obj = data->obj;
  switch (wrapped_obj->type)
    {
    case WrappedObjectType::WRAPPED_OBJECT_TYPE_ARRANGER_OBJECT:
      {
        ArrangerObject * obj = (ArrangerObject *) wrapped_obj->obj;
        if (editing)
          {
            arranger_object_edit_begin (obj);
          }
        else
          {
            EditableChangedInfo * nfo = object_new (EditableChangedInfo);
            nfo->column_name = g_strdup (data->factory->column_name);
            nfo->text = g_strdup (text);
            nfo->obj = obj;
            g_idle_add_full (
              G_PRIORITY_DEFAULT_IDLE, editable_label_changed_source, nfo,
              editable_changed_info_free);
          }
      }
      break;
    default:
      break;
    }
}

static void
item_factory_setup_cb (
  GtkSignalListItemFactory * factory,
  GtkListItem *              listitem,
  gpointer                   user_data)
{
  ItemFactory * self = (ItemFactory *) user_data;

  PopoverMenuBinWidget * bin = popover_menu_bin_widget_new ();
  gtk_widget_add_css_class (GTK_WIDGET (bin), "list-item-child");

  switch (self->type)
    {
    case ItemFactoryType::ITEM_FACTORY_TOGGLE:
      {
        GtkWidget * check_btn = gtk_check_button_new ();
        if (!self->editable)
          {
            gtk_widget_set_sensitive (GTK_WIDGET (check_btn), false);
          }
        popover_menu_bin_widget_set_child (bin, check_btn);
      }
      break;
    case ItemFactoryType::ITEM_FACTORY_TEXT:
    case ItemFactoryType::ITEM_FACTORY_INTEGER:
    case ItemFactoryType::ITEM_FACTORY_POSITION:
      {
        GtkWidget * label;
        const int   max_chars = 20;
        if (self->editable)
          {
            label = gtk_editable_label_new ("");
            gtk_editable_set_max_width_chars (GTK_EDITABLE (label), max_chars);
            GtkWidget * inner_label =
              z_gtk_widget_find_child_of_type (label, GTK_TYPE_LABEL);
            g_return_if_fail (inner_label);
            if (self->ellipsize_label)
              {
                gtk_label_set_ellipsize (
                  GTK_LABEL (inner_label), PANGO_ELLIPSIZE_END);
              }
            gtk_label_set_max_width_chars (GTK_LABEL (inner_label), max_chars);
          }
        else
          {
            label = gtk_label_new ("");
            if (self->ellipsize_label)
              {
                gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
              }
            gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_START);
            gtk_label_set_max_width_chars (GTK_LABEL (label), max_chars);
          }
        popover_menu_bin_widget_set_child (bin, label);
      }
      break;
    case ItemFactoryType::ITEM_FACTORY_ICON_AND_TEXT:
      {
        g_return_if_fail (!self->editable);
        GtkBox *    box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6));
        GtkImage *  img = GTK_IMAGE (gtk_image_new ());
        GtkWidget * label = gtk_label_new ("");
        gtk_box_append (GTK_BOX (box), GTK_WIDGET (img));
        gtk_box_append (GTK_BOX (box), label);
        popover_menu_bin_widget_set_child (bin, GTK_WIDGET (box));
      }
      break;
    case ItemFactoryType::ITEM_FACTORY_ICON:
      {
        g_return_if_fail (!self->editable);
        GtkImage * img = GTK_IMAGE (gtk_image_new ());
        popover_menu_bin_widget_set_child (bin, GTK_WIDGET (img));
      }
      break;
    case ItemFactoryType::ITEM_FACTORY_COLOR:
      {
        ColorAreaWidget * ca =
          Z_COLOR_AREA_WIDGET (g_object_new (COLOR_AREA_WIDGET_TYPE, NULL));
        popover_menu_bin_widget_set_child (bin, GTK_WIDGET (ca));
      }
    default:
      break;
    }

  gtk_list_item_set_child (listitem, GTK_WIDGET (bin));

#if 0
  g_signal_connect (
    G_OBJECT (listitem), "notify::selected",
    G_CALLBACK (on_list_item_selected), self);
#endif
}

static void
add_plugin_descr_context_menu (
  PopoverMenuBinWidget * bin,
  PluginDescriptor *     descr)
{
  GMenuModel * model = plugin_descriptor_generate_context_menu (descr);

  if (model)
    {
      popover_menu_bin_widget_set_menu_model (bin, G_MENU_MODEL (model));
    }
}

static void
add_supported_file_context_menu (
  PopoverMenuBinWidget * bin,
  SupportedFile *        descr)
{
  GMenu *     menu = g_menu_new ();
  GMenuItem * menuitem;
  char        tmp[600];

  if (descr->type == ZFileType::FILE_TYPE_DIR)
    {
      sprintf (tmp, "app.panel-file-browser-add-bookmark::%p", descr);
      menuitem = z_gtk_create_menu_item (
        _ ("Add Bookmark"), "gnome-icon-library-starred-symbolic", tmp);
      g_menu_append_item (menu, menuitem);
    }

  popover_menu_bin_widget_set_menu_model (bin, G_MENU_MODEL (menu));
}

static void
add_chord_pset_pack_context_menu (
  PopoverMenuBinWidget * bin,
  ChordPresetPack *      pack)
{
  GMenuModel * model = chord_preset_pack_generate_context_menu (pack);

  if (model)
    {
      popover_menu_bin_widget_set_menu_model (bin, G_MENU_MODEL (model));
    }
}

static void
add_chord_pset_context_menu (PopoverMenuBinWidget * bin, ChordPreset * pset)
{
  GMenuModel * model = chord_preset_generate_context_menu (pset);

  if (model)
    {
      popover_menu_bin_widget_set_menu_model (bin, G_MENU_MODEL (model));
    }
}

static void
add_plugin_collection_context_menu (
  PopoverMenuBinWidget *   bin,
  const PluginCollection * coll)
{
  GMenuModel * model = plugin_collection_generate_context_menu (coll);

  if (model)
    {
      popover_menu_bin_widget_set_menu_model (bin, G_MENU_MODEL (model));
    }
}

static void
add_file_browser_location_context_menu (
  PopoverMenuBinWidget *      bin,
  const FileBrowserLocation * loc)
{
  GMenuModel * model = file_browser_location_generate_context_menu (loc);

  if (model)
    {
      popover_menu_bin_widget_set_menu_model (bin, G_MENU_MODEL (model));
    }
}

static void
item_factory_bind_cb (
  GtkSignalListItemFactory * factory,
  GtkListItem *              listitem,
  gpointer                   user_data)
{
  ItemFactory * self = (ItemFactory *) user_data;

  GObject * gobj = G_OBJECT (gtk_list_item_get_item (listitem));
  WrappedObjectWithChangeSignal * obj;
  if (GTK_IS_TREE_LIST_ROW (gobj))
    {
      GtkTreeListRow * row = GTK_TREE_LIST_ROW (gobj);
      obj =
        Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gtk_tree_list_row_get_item (row));
    }
  else
    {
      obj =
        Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gtk_list_item_get_item (listitem));
    }

  switch (self->type)
    {
    case ItemFactoryType::ITEM_FACTORY_TOGGLE:
      {
        PopoverMenuBinWidget * bin =
          Z_POPOVER_MENU_BIN_WIDGET (gtk_list_item_get_child (listitem));
        GtkWidget * check_btn = popover_menu_bin_widget_get_child (bin);

        switch (obj->type)
          {
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_MIDI_MAPPING:
            {
              if (string_is_equal (self->column_name, _ ("On")))
                {
                  MidiMapping * mapping = (MidiMapping *) obj->obj;
                  gtk_check_button_set_active (
                    GTK_CHECK_BUTTON (check_btn), mapping->enabled);
                }
            }
            break;
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_TRACK:
            {
              if (string_is_equal (self->column_name, _ ("Visibility")))
                {
                  Track * track = (Track *) obj->obj;
                  gtk_check_button_set_active (
                    GTK_CHECK_BUTTON (check_btn), track->visible);
                }
            }
            break;
          default:
            break;
          }

        ItemFactoryData * data = item_factory_data_new (self, obj);
        g_object_set_data (G_OBJECT (bin), "item-factory-data", data);
        g_signal_connect_data (
          G_OBJECT (check_btn), "toggled", G_CALLBACK (on_toggled), data,
          item_factory_data_destroy_closure, G_CONNECT_AFTER);
      }
      break;
    case ItemFactoryType::ITEM_FACTORY_TEXT:
    case ItemFactoryType::ITEM_FACTORY_INTEGER:
    case ItemFactoryType::ITEM_FACTORY_POSITION:
      {
        PopoverMenuBinWidget * bin =
          Z_POPOVER_MENU_BIN_WIDGET (gtk_list_item_get_child (listitem));
        GtkWidget * label = popover_menu_bin_widget_get_child (bin);

        char str[600];
        switch (obj->type)
          {
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_MIDI_MAPPING:
            {
              MidiMapping * mm = (MidiMapping *) obj->obj;
              if (string_is_equal (self->column_name, _ ("Note/Control")))
                {
                  char ctrl[60];
                  int  ctrl_change_ch =
                    midi_ctrl_change_get_ch_and_description (mm->key, ctrl);
                  if (ctrl_change_ch > 0)
                    {
                      sprintf (
                        str, "%02X-%02X (%s)", mm->key[0], mm->key[1], ctrl);
                    }
                  else
                    {
                      sprintf (str, "%02X-%02X", mm->key[0], mm->key[1]);
                    }
                }
              if (string_is_equal (self->column_name, _ ("Destination")))
                {
                  Port * port = Port::find_from_identifier (&mm->dest_id);
                  port_get_full_designation (port, str);

                  char min_str[40], max_str[40];
                  sprintf (min_str, "%.4f", (double) port->minf);
                  sprintf (max_str, "%.4f", (double) port->maxf);
                }
            }
            break;
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_ARRANGER_OBJECT:
            {
              ArrangerObject * arr_obj = (ArrangerObject *) obj->obj;

              if (string_is_equal (self->column_name, _ ("Note")))
                {
                  MidiNote * mn = (MidiNote *) arr_obj;
                  midi_note_get_val_as_string (
                    mn, str,
                    PianoRollNoteNotation::PIANO_ROLL_NOTE_NOTATION_MUSICAL, 0);
                }
              else if (string_is_equal (self->column_name, _ ("Pitch")))
                {
                  MidiNote * mn = (MidiNote *) arr_obj;
                  sprintf (str, "%u", mn->val);
                }
              else if (
                string_is_equal (self->column_name, _ ("Start"))
                || string_is_equal (self->column_name, _ ("Position")))
                {
                  position_to_string_full (&arr_obj->pos, str, 3);
                }
              else if (string_is_equal (self->column_name, _ ("End")))
                {
                  if (arranger_object_type_has_length (arr_obj->type))
                    {
                      position_to_string_full (&arr_obj->end_pos, str, 3);
                    }
                  else
                    {
                      strcpy (str, _ ("N/A"));
                    }
                }
              else if (string_is_equal (self->column_name, _ ("Loop start")))
                {
                  if (arranger_object_type_can_loop (arr_obj->type))
                    {
                      position_to_string_full (&arr_obj->loop_start_pos, str, 3);
                    }
                  else
                    {
                      strcpy (str, _ ("N/A"));
                    }
                }
              else if (string_is_equal (self->column_name, _ ("Loop end")))
                {
                  if (arranger_object_type_can_loop (arr_obj->type))
                    {
                      position_to_string_full (&arr_obj->loop_end_pos, str, 3);
                    }
                  else
                    {
                      strcpy (str, _ ("N/A"));
                    }
                }
              else if (string_is_equal (self->column_name, _ ("Clip start")))
                {
                  if (arranger_object_type_can_loop (arr_obj->type))
                    {
                      position_to_string_full (&arr_obj->clip_start_pos, str, 3);
                    }
                  else
                    {
                      strcpy (str, _ ("N/A"));
                    }
                }
              else if (string_is_equal (self->column_name, _ ("Fade in")))
                {
                  if (arranger_object_can_fade (arr_obj))
                    {
                      position_to_string_full (&arr_obj->fade_in_pos, str, 3);
                    }
                  else
                    {
                      strcpy (str, _ ("N/A"));
                    }
                }
              else if (string_is_equal (self->column_name, _ ("Fade out")))
                {
                  if (arranger_object_can_fade (arr_obj))
                    {
                      position_to_string_full (&arr_obj->fade_out_pos, str, 3);
                    }
                  else
                    {
                      strcpy (str, _ ("N/A"));
                    }
                }
              else if (string_is_equal (self->column_name, _ ("Name")))
                {
                  char * obj_name =
                    arranger_object_gen_human_readable_name (arr_obj);
                  g_return_if_fail (obj_name);
                  strcpy (str, obj_name);
                  g_free (obj_name);
                }
              else if (string_is_equal (self->column_name, _ ("Type")))
                {
                  const char * untranslated_type =
                    arranger_object_get_type_as_string (arr_obj->type);
                  strcpy (str, _ (untranslated_type));
                }
              else if (string_is_equal (self->column_name, _ ("Velocity")))
                {
                  MidiNote * mn = (MidiNote *) arr_obj;
                  sprintf (str, "%u", mn->vel->vel);
                }
              else if (string_is_equal (self->column_name, _ ("Index")))
                {
                  AutomationPoint * ap = (AutomationPoint *) arr_obj;
                  sprintf (str, "%d", ap->index);
                }
              else if (string_is_equal (self->column_name, _ ("Value")))
                {
                  AutomationPoint * ap = (AutomationPoint *) arr_obj;
                  sprintf (str, "%f", ap->fvalue);
                }
              else if (string_is_equal (self->column_name, _ ("Curve type")))
                {
                  AutomationPoint * ap = (AutomationPoint *) arr_obj;
                  curve_algorithm_get_localized_name (ap->curve_opts.algo, str);
                }
              else if (string_is_equal (self->column_name, _ ("Curviness")))
                {
                  AutomationPoint * ap = (AutomationPoint *) arr_obj;
                  sprintf (str, "%f", ap->curve_opts.curviness);
                }
            }
            break;
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_PROJECT_INFO:
            {
              ProjectInfo * nfo = (ProjectInfo *) obj->obj;

              if (
                string_is_equal (self->column_name, _ ("Name"))
                || string_is_equal (self->column_name, _ ("Template Name")))
                {
                  strcpy (str, nfo->name);
                }
              else if (string_is_equal (self->column_name, _ ("Path")))
                {
                  strcpy (str, nfo->filename);
                }
              else if (string_is_equal (self->column_name, _ ("Last Modified")))
                {
                  strcpy (str, nfo->modified_str);
                }
            }
            break;
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_TRACK:
            {
              Track * track = (Track *) obj->obj;

              if (string_is_equal (self->column_name, _ ("Name")))
                {
                  strcpy (str, track->name);
                }
            }
            break;
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_PORT:
            {
              Port * port = (Port *) obj->obj;
              port_get_full_designation (port, str);
            }
            break;
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_PLUGIN_COLLECTION:
            {
              PluginCollection * coll = (PluginCollection *) obj->obj;
              strcpy (str, coll->name);
              add_plugin_collection_context_menu (bin, coll);
            }
            break;
          default:
            break;
          }

        if (self->editable)
          {
            gtk_editable_set_text (GTK_EDITABLE (label), str);
            ItemFactoryData * data = item_factory_data_new (self, obj);
            g_object_set_data (G_OBJECT (bin), "item-factory-data", data);
            g_signal_handlers_disconnect_by_func (
              bin, (gpointer) on_editable_label_editing_changed, data);
            g_signal_connect_data (
              G_OBJECT (label), "notify::editing",
              G_CALLBACK (on_editable_label_editing_changed), data,
              item_factory_data_destroy_closure, G_CONNECT_AFTER);
          }
        else
          {
            gtk_label_set_text (GTK_LABEL (label), str);
          }
      }
      break;
    case ItemFactoryType::ITEM_FACTORY_ICON_AND_TEXT:
      {
        PopoverMenuBinWidget * bin =
          Z_POPOVER_MENU_BIN_WIDGET (gtk_list_item_get_child (listitem));
        GtkBox *   box = GTK_BOX (popover_menu_bin_widget_get_child (bin));
        GtkImage * img =
          GTK_IMAGE (gtk_widget_get_first_child (GTK_WIDGET (box)));
        GtkLabel * lbl =
          GTK_LABEL (gtk_widget_get_last_child (GTK_WIDGET (box)));

        switch (obj->type)
          {
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_PLUGIN_DESCR:
            {
              PluginDescriptor * descr = (PluginDescriptor *) obj->obj;
              gtk_image_set_from_icon_name (
                img, plugin_descriptor_get_icon_name (descr));
              gtk_label_set_text (lbl, descr->name);

              /* set as drag source */
              GtkDragSource * drag_source = gtk_drag_source_new ();
              gtk_drag_source_set_actions (
                drag_source, GDK_ACTION_COPY | GDK_ACTION_MOVE);
              ItemFactoryData * data = item_factory_data_new (self, obj);
              g_object_set_data (G_OBJECT (bin), "item-factory-data", data);
              g_signal_connect_data (
                G_OBJECT (drag_source), "prepare",
                G_CALLBACK (on_dnd_drag_prepare), data,
                item_factory_data_destroy_closure, (GConnectFlags) 0);
              g_object_set_data (G_OBJECT (bin), "drag-source", drag_source);
              gtk_widget_add_controller (
                GTK_WIDGET (bin), GTK_EVENT_CONTROLLER (drag_source));

              add_plugin_descr_context_menu (bin, descr);
            }
            break;
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_SUPPORTED_FILE:
            {
              SupportedFile * descr = (SupportedFile *) obj->obj;

              gtk_image_set_from_icon_name (
                img, supported_file_get_icon_name (descr));
              gtk_label_set_text (lbl, descr->label);

              /* set as drag source */
              GtkDragSource * drag_source = gtk_drag_source_new ();
              gtk_drag_source_set_actions (
                drag_source, GDK_ACTION_COPY | GDK_ACTION_MOVE);
              ItemFactoryData * data = item_factory_data_new (self, obj);
              g_object_set_data (G_OBJECT (bin), "item-factory-data", data);
              g_signal_connect_data (
                G_OBJECT (drag_source), "prepare",
                G_CALLBACK (on_dnd_drag_prepare), data,
                item_factory_data_destroy_closure, (GConnectFlags) 0);
              g_object_set_data (G_OBJECT (bin), "drag-source", drag_source);
              gtk_widget_add_controller (
                GTK_WIDGET (bin), GTK_EVENT_CONTROLLER (drag_source));

              add_supported_file_context_menu (bin, descr);
            }
            break;
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_CHORD_PSET_PACK:
            {
              ChordPresetPack * pack = (ChordPresetPack *) obj->obj;

              gtk_image_set_from_icon_name (img, "minuet-chords");
              gtk_label_set_text (lbl, pack->name);

              add_chord_pset_pack_context_menu (bin, pack);
            }
            break;
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_CHORD_PSET:
            {
              ChordPreset * pset = (ChordPreset *) obj->obj;

              gtk_image_set_from_icon_name (img, "minuet-chords");
              gtk_label_set_text (lbl, pset->name);

              add_chord_pset_context_menu (bin, pset);
            }
            break;
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_CHANNEL_SEND_TARGET:
            {
              ChannelSendTarget * target = (ChannelSendTarget *) obj->obj;

              char * icon_name = channel_send_target_get_icon (target);
              gtk_image_set_from_icon_name (img, icon_name);
              g_free (icon_name);

              char * label = channel_send_target_describe (target);
              gtk_label_set_text (lbl, label);
              g_free (label);
            }
            break;
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_FILE_BROWSER_LOCATION:
            {
              FileBrowserLocation * loc = (FileBrowserLocation *) obj->obj;

              const char * icon_name = file_browser_location_get_icon_name (loc);
              gtk_image_set_from_icon_name (img, icon_name);

              gtk_label_set_text (lbl, loc->label);
              add_file_browser_location_context_menu (bin, loc);
            }
            break;
          default:
            break;
          }
      }
      break;
    case ItemFactoryType::ITEM_FACTORY_ICON:
      {
        PopoverMenuBinWidget * bin =
          Z_POPOVER_MENU_BIN_WIDGET (gtk_list_item_get_child (listitem));
        GtkImage * img = GTK_IMAGE (popover_menu_bin_widget_get_child (bin));

        switch (obj->type)
          {
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_TRACK:
            {
              Track *      track = (Track *) obj->obj;
              const char * icon_name = track->icon_name;
              gtk_image_set_from_icon_name (img, icon_name);
            }
            break;
          default:
            break;
          }
      }
      break;
    case ItemFactoryType::ITEM_FACTORY_COLOR:
      {
        PopoverMenuBinWidget * bin =
          Z_POPOVER_MENU_BIN_WIDGET (gtk_list_item_get_child (listitem));
        ColorAreaWidget * ca =
          Z_COLOR_AREA_WIDGET (popover_menu_bin_widget_get_child (bin));

        switch (obj->type)
          {
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_TRACK:
            {
              Track * track = (Track *) obj->obj;
              color_area_widget_setup_generic (ca, &track->color);
            }
            break;
          default:
            break;
          }
      }
    default:
      break;
    }
}

static void
item_factory_unbind_cb (
  GtkSignalListItemFactory * factory,
  GtkListItem *              listitem,
  gpointer                   user_data)
{
  ItemFactory * self = (ItemFactory *) user_data;

  GObject * gobj = G_OBJECT (gtk_list_item_get_item (listitem));
  WrappedObjectWithChangeSignal * obj;
  if (GTK_IS_TREE_LIST_ROW (gobj))
    {
      GtkTreeListRow * row = GTK_TREE_LIST_ROW (gobj);
      obj =
        Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gtk_tree_list_row_get_item (row));
    }
  else
    {
      obj =
        Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gtk_list_item_get_item (listitem));
    }

  PopoverMenuBinWidget * bin =
    Z_POPOVER_MENU_BIN_WIDGET (gtk_list_item_get_child (listitem));
  GtkWidget * widget = popover_menu_bin_widget_get_child (bin);

  switch (self->type)
    {
    case ItemFactoryType::ITEM_FACTORY_TOGGLE:
      {
        gpointer data = g_object_get_data (G_OBJECT (bin), "item-factory-data");
        g_signal_handlers_disconnect_by_func (
          widget, (gpointer) on_toggled, data);
      }
      break;
    case ItemFactoryType::ITEM_FACTORY_TEXT:
    case ItemFactoryType::ITEM_FACTORY_POSITION:
    case ItemFactoryType::ITEM_FACTORY_INTEGER:
      {
        gpointer data = g_object_get_data (G_OBJECT (bin), "item-factory-data");
        g_signal_handlers_disconnect_by_func (
          widget, (gpointer) on_editable_label_editing_changed, data);
      }
      break;
    case ItemFactoryType::ITEM_FACTORY_ICON_AND_TEXT:
      {
        GtkBox * box = GTK_BOX (popover_menu_bin_widget_get_child (bin));
        (void) box;

        switch (obj->type)
          {
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_PLUGIN_DESCR:
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_SUPPORTED_FILE:
            {
              /* remove drag source */
              GtkDragSource * drag_source = GTK_DRAG_SOURCE (
                g_object_get_data (G_OBJECT (bin), "drag-source"));
              gtk_widget_remove_controller (
                GTK_WIDGET (bin), GTK_EVENT_CONTROLLER (drag_source));
            }
            break;
          default:
            break;
          }
      }
      break;
    case ItemFactoryType::ITEM_FACTORY_ICON:
    case ItemFactoryType::ITEM_FACTORY_COLOR:
      /* nothing to do */
      break;
    default:
      break;
    }
}

static void
item_factory_teardown_cb (
  GtkSignalListItemFactory * factory,
  GtkListItem *              listitem,
  gpointer                   user_data)
{
  /*WrappedObjectWithChangeSignal * obj =*/
  /*Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (*/
  /*gtk_list_item_get_item (listitem));*/

  gtk_list_item_set_child (listitem, NULL);
}

/**
 * Creates a new item factory.
 *
 * @param editable Whether the item should be
 *   editable.
 * @param column_name Column name, if column view,
 *   otherwise NULL.
 */
ItemFactory *
item_factory_new (ItemFactoryType type, bool editable, const char * column_name)
{
  ItemFactory * self = object_new (ItemFactory);

  self->type = type;

  self->ellipsize_label = true;

  self->editable = editable;
  if (column_name)
    self->column_name = g_strdup (column_name);

  GtkListItemFactory * list_item_factory = gtk_signal_list_item_factory_new ();

  g_signal_connect (
    G_OBJECT (list_item_factory), "setup", G_CALLBACK (item_factory_setup_cb),
    self);
  g_signal_connect (
    G_OBJECT (list_item_factory), "bind", G_CALLBACK (item_factory_bind_cb),
    self);
  g_signal_connect (
    G_OBJECT (list_item_factory), "unbind", G_CALLBACK (item_factory_unbind_cb),
    self);
  g_signal_connect (
    G_OBJECT (list_item_factory), "teardown",
    G_CALLBACK (item_factory_teardown_cb), self);

  self->list_item_factory = GTK_LIST_ITEM_FACTORY (list_item_factory);

  return self;
}

/**
 * Shorthand to generate and append a column to
 * a column view.
 *
 * @return The newly created ItemFactory, for
 *   convenience.
 */
ItemFactory *
item_factory_generate_and_append_column (
  GtkColumnView * column_view,
  GPtrArray *     item_factories,
  ItemFactoryType type,
  bool            editable,
  bool            resizable,
  GtkSorter *     sorter,
  const char *    column_name)
{
  ItemFactory * item_factory = item_factory_new (type, editable, column_name);
  GtkListItemFactory *  list_item_factory = item_factory->list_item_factory;
  GtkColumnViewColumn * column =
    gtk_column_view_column_new (column_name, list_item_factory);
  gtk_column_view_column_set_resizable (column, true);
  gtk_column_view_column_set_expand (column, true);
  if (sorter)
    {
      gtk_column_view_column_set_sorter (column, sorter);
    }
  gtk_column_view_append_column (column_view, column);
  g_ptr_array_add (item_factories, item_factory);

  return item_factory;
}

void
item_factory_free (ItemFactory * self)
{
  g_free_and_null (self->column_name);

  object_zero_and_free (self);
}

void
item_factory_free_func (void * self)
{
  item_factory_free ((ItemFactory *) self);
}
