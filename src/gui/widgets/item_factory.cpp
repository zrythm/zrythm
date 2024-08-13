// SPDX-FileCopyrightText: © 2021-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "actions/arranger_selections.h"
#include "actions/midi_mapping_action.h"
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
#include "plugins/collections.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/math.h"
#include "utils/midi.h"
#include "utils/objects.h"
#include "utils/rt_thread_id.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

/**
 * Data for individual widget callbacks.
 */
struct ItemFactoryData
{
  ItemFactoryData ();
  ItemFactoryData (const ItemFactory &factory, WrappedObjectWithChangeSignal &obj)
      : factory_ (factory), obj_ (&obj)
  {
  }

  static void destroy_closure (gpointer user_data, GClosure * closure)
  {
    auto * data = (ItemFactoryData *) user_data;
    delete data;
  }

public:
  ItemFactory                     factory_;
  WrappedObjectWithChangeSignal * obj_;
};

static void
on_toggled (GtkCheckButton * self, gpointer user_data)
{
  ItemFactoryData * data = (ItemFactoryData *) user_data;
  ItemFactory      &factory = data->factory_;

  WrappedObjectWithChangeSignal * obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data->obj_);

  bool active = gtk_check_button_get_active (self);

  switch (obj->type)
    {
    case WrappedObjectType::WRAPPED_OBJECT_TYPE_MIDI_MAPPING:
      {
        auto * mapping = (MidiMapping *) obj->obj;
        int    mapping_idx = MIDI_MAPPINGS->get_mapping_index (*mapping);
        try
          {
            UNDO_MANAGER->perform (
              std::make_unique<MidiMappingAction> (mapping_idx, active));
          }
        catch (const ZrythmException &e)
          {
            e.handle (_ ("Failed to enable binding"));
          }
      }
      break;
    case WrappedObjectType::WRAPPED_OBJECT_TYPE_TRACK:
      {
        auto track = (Track *) obj->obj;
        if (factory.column_name_ == std::string (_ ("Visibility")))
          {
            track->visible_ = active;
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

  z_debug ("dnd prepare from item factory");

  z_return_val_if_fail (
    Z_IS_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data->obj_), nullptr);
  GdkContentProvider * content_providers[] = {
    gdk_content_provider_new_typed (
      WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE, data->obj_),
  };

  return gdk_content_provider_new_union (
    content_providers, G_N_ELEMENTS (content_providers));
}

struct EditableChangedInfo
{
  ArrangerObject * obj;
  std::string      column_name;
  std::string      text;

  static void free_notify_func (gpointer user_data)
  {
    auto * self = (EditableChangedInfo *) user_data;
    delete self;
  }
};

static void
handle_arranger_object_position_change (
  ArrangerObject *             obj,
  ArrangerObject *             prev_obj,
  const std::string           &text,
  ArrangerObject::PositionType type)
{
  try
    {
      Position pos{ text.c_str () };
      pos.print ();
      Position prev_pos;
      obj->get_position_from_type (&prev_pos, type);
      if (obj->is_position_valid (pos, type))
        {
          if (pos != prev_pos)
            {
              obj->set_position (&pos, type, false);
              obj->edit_position_finish ();
            }
        }
      else
        {
          ui_show_error_message (_ ("Invalid Position"), _ ("Invalid position"));
        }
    }
  catch (const ZrythmException &e)
    {
      ui_show_error_message (_ ("Invalid Position"), e.what ());
    }
}

/**
 * Source function for performing changes later.
 */
static int
editable_label_changed_source (gpointer user_data)
{
  auto        self = static_cast<EditableChangedInfo *> (user_data);
  const auto &column_name = self->column_name;
  const auto &text = self->text;
  auto *      obj = self->obj;
  auto *      arranger = obj->get_arranger ();
  auto *      prev_obj = arranger->start_object.get ();

  const auto handle_position_change = [&] (ArrangerObject::PositionType type) {
    handle_arranger_object_position_change (obj, prev_obj, text, type);
  };

  const auto parse_uint8 = [&text] (uint8_t &val) {
    return std::from_chars (text.data (), text.data () + text.size (), val).ec
           == std::errc{};
  };

  const auto parse_float = [&text] (float &val) {
    return std::from_chars (text.data (), text.data () + text.size (), val).ec
           == std::errc{};
  };

  using enum ArrangerObject::PositionType;
  using enum ArrangerSelectionsAction::EditType;

  static const std::unordered_map<std::string, std::function<void ()>> actions = {
    {_ ("Start"),       [&] { handle_position_change (Start); }    },
    { _ ("Position"),   [&] { handle_position_change (Start); }    },
    { _ ("End"),        [&] { handle_position_change (End); }      },
    { _ ("Clip start"), [&] { handle_position_change (ClipStart); }},
    { _ ("Loop start"), [&] { handle_position_change (LoopStart); }},
    { _ ("Loop end"),   [&] { handle_position_change (LoopEnd); }  },
    { _ ("Fade in"),    [&] { handle_position_change (FadeIn); }   },
    { _ ("Fade out"),   [&] { handle_position_change (FadeOut); }  },
    { _ ("Name"),
     [&] {
        auto *                                                         nameable_object = dynamic_cast<NameableObject *> (obj);
        if (!nameable_object)
          return;
        if (nameable_object->validate_name (text))
          {
            nameable_object->set_name (text, false);
            obj->edit_finish (static_cast<int> (Name));
          }
        else
          {
            ui_show_error_message (_ ("Invalid Name"), _ ("Invalid name"));
          }
      }                                                            },
    { _ ("Velocity"),
     [&] {
        uint8_t                                                        val;
        if (!parse_uint8 (val) || val < 1 || val > 127)
          {
            ui_show_error_message (
              _ ("Invalid Velocity"), _ ("Invalid velocity"));
          }
        else if (
          auto * mn = dynamic_cast<MidiNote *> (obj);
          mn && mn->vel_->vel_ != val)
          {
            mn->vel_->vel_ = val;
            obj->edit_finish (static_cast<int> (Primitive));
          }
      }                                                            },
    { _ ("Pitch"),
     [&] {
        uint8_t                                                        val;
        if (!parse_uint8 (val) || val < 1 || val > 127)
          {
            ui_show_error_message (_ ("Invalid Pitch"), _ ("Invalid pitch"));
          }
        else if (
          auto * mn = dynamic_cast<MidiNote *> (obj); mn && mn->val_ != val)
          {
            mn->set_val (val);
            obj->edit_finish (static_cast<int> (Primitive));
          }
      }                                                            },
    { _ ("Value"),
     [&] {
        if (auto * ap = dynamic_cast<AutomationPoint *> (obj))
          {
            auto  port = ap->get_port ();
            float val;
            if (!parse_float (val) || val < port->minf_ || val > port->maxf_)
              {
                ui_show_error_message (_ ("Invalid Value"), _ ("Invalid value"));
              }
            else if (
              auto * prev_ap = dynamic_cast<AutomationPoint *> (prev_obj);
              !math_floats_equal (prev_ap->fvalue_, val))
              {
                ap->set_fvalue (val, false, false);
                obj->edit_finish (static_cast<int> (Primitive));
              }
          }
      }                                                            },
    { _ ("Curviness"),
     [&] {
        float                                                          val;
        if (!parse_float (val) || val < -1.f || val > 1.f)
          {
            ui_show_error_message (_ ("Invalid Value"), _ ("Invalid value"));
          }
        else if (
          auto * ap = dynamic_cast<AutomationPoint *> (obj);
          auto * prev_ap = dynamic_cast<AutomationPoint *> (prev_obj))
          {
            ap->curve_opts_.curviness_ = val;
            if (prev_ap->curve_opts_ != ap->curve_opts_)
              {
                obj->edit_finish (static_cast<int> (Primitive));
              }
          }
      }                                                            }
  };

  if (auto it = actions.find (column_name); it != actions.end ())
    {
      it->second ();
    }

  arranger->start_object.reset ();
  return G_SOURCE_REMOVE;
}

static void
on_editable_label_editing_changed (
  GObject *    gobject,
  GParamSpec * pspec,
  gpointer     user_data)
{
  auto * data = (ItemFactoryData *) user_data;

  GtkEditableLabel * editable_lbl = GTK_EDITABLE_LABEL (gobject);
  const char *       text = gtk_editable_get_text (GTK_EDITABLE (editable_lbl));

  bool editing = gtk_editable_label_get_editing (editable_lbl);

  z_debug ("editing changed: %d", editing);
  z_debug ("text: %s", text);

  /* perform change */
  auto wrapped_obj = data->obj_;
  switch (wrapped_obj->type)
    {
    case WrappedObjectType::WRAPPED_OBJECT_TYPE_ARRANGER_OBJECT:
      {
        auto * obj = (ArrangerObject *) wrapped_obj->obj;
        if (editing)
          {
            obj->edit_begin ();
          }
        else
          {
            auto * nfo = object_new (EditableChangedInfo);
            nfo->column_name = data->factory_.column_name_;
            nfo->text = text;
            nfo->obj = obj;
            g_idle_add_full (
              G_PRIORITY_DEFAULT_IDLE, editable_label_changed_source, nfo,
              EditableChangedInfo::free_notify_func);
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
  auto * self = (ItemFactory *) user_data;

  PopoverMenuBinWidget * bin = popover_menu_bin_widget_new ();
  gtk_widget_add_css_class (GTK_WIDGET (bin), "list-item-child");

  switch (self->type_)
    {
    case ItemFactory::Type::Toggle:
      {
        GtkWidget * check_btn = gtk_check_button_new ();
        if (!self->editable_)
          {
            gtk_widget_set_sensitive (GTK_WIDGET (check_btn), false);
          }
        popover_menu_bin_widget_set_child (bin, check_btn);
      }
      break;
    case ItemFactory::Type::Text:
    case ItemFactory::Type::Integer:
    case ItemFactory::Type::Position:
      {
        GtkWidget * label;
        const int   max_chars = 20;
        if (self->editable_)
          {
            label = gtk_editable_label_new ("");
            gtk_editable_set_max_width_chars (GTK_EDITABLE (label), max_chars);
            GtkWidget * inner_label =
              z_gtk_widget_find_child_of_type (label, GTK_TYPE_LABEL);
            z_return_if_fail (inner_label);
            if (self->ellipsize_label_)
              {
                gtk_label_set_ellipsize (
                  GTK_LABEL (inner_label), PANGO_ELLIPSIZE_END);
              }
            gtk_label_set_max_width_chars (GTK_LABEL (inner_label), max_chars);
          }
        else
          {
            label = gtk_label_new ("");
            if (self->ellipsize_label_)
              {
                gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
              }
            gtk_widget_set_halign (GTK_WIDGET (label), GTK_ALIGN_START);
            gtk_label_set_max_width_chars (GTK_LABEL (label), max_chars);
          }
        popover_menu_bin_widget_set_child (bin, label);
      }
      break;
    case ItemFactory::Type::IconAndText:
      {
        z_return_if_fail (!self->editable_);
        GtkBox *    box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6));
        GtkImage *  img = GTK_IMAGE (gtk_image_new ());
        GtkWidget * label = gtk_label_new ("");
        gtk_box_append (GTK_BOX (box), GTK_WIDGET (img));
        gtk_box_append (GTK_BOX (box), label);
        popover_menu_bin_widget_set_child (bin, GTK_WIDGET (box));
      }
      break;
    case ItemFactory::Type::Icon:
      {
        z_return_if_fail (!self->editable_);
        GtkImage * img = GTK_IMAGE (gtk_image_new ());
        popover_menu_bin_widget_set_child (bin, GTK_WIDGET (img));
      }
      break;
    case ItemFactory::Type::Color:
      {
        ColorAreaWidget * ca =
          Z_COLOR_AREA_WIDGET (g_object_new (COLOR_AREA_WIDGET_TYPE, nullptr));
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
  GMenuModel * model = descr->generate_context_menu ();

  if (model)
    {
      popover_menu_bin_widget_set_menu_model (bin, G_MENU_MODEL (model));
    }
}

static void
add_supported_file_context_menu (
  PopoverMenuBinWidget * bin,
  FileDescriptor *       descr)
{
  GMenu *     menu = g_menu_new ();
  GMenuItem * menuitem;

  if (descr->type_ == FileType::Directory)
    {
      auto str = fmt::format (
        "app.panel-file-browser-add-bookmark::{}", fmt::ptr (descr));
      menuitem = z_gtk_create_menu_item (
        _ ("Add Bookmark"), "gnome-icon-library-starred-symbolic", str.c_str ());
      g_menu_append_item (menu, menuitem);
    }

  popover_menu_bin_widget_set_menu_model (bin, G_MENU_MODEL (menu));
}

static void
add_chord_pset_pack_context_menu (
  PopoverMenuBinWidget * bin,
  ChordPresetPack *      pack)
{
  GMenuModel * model = pack->generate_context_menu ();

  if (model)
    {
      popover_menu_bin_widget_set_menu_model (bin, G_MENU_MODEL (model));
    }
}

static void
add_chord_pset_context_menu (PopoverMenuBinWidget * bin, ChordPreset * pset)
{
  GMenuModel * model = pset->generate_context_menu ();

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
  GMenuModel * model = coll->generate_context_menu ()->gobj ();

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
  GMenuModel * model = loc->generate_context_menu ();

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
  auto self = static_cast<ItemFactory *> (user_data);

  auto gobj = G_OBJECT (gtk_list_item_get_item (listitem));
  WrappedObjectWithChangeSignal * obj;
  if (GTK_IS_TREE_LIST_ROW (gobj))
    {
      auto row = GTK_TREE_LIST_ROW (gobj);
      obj =
        Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gtk_tree_list_row_get_item (row));
    }
  else
    {
      obj =
        Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gtk_list_item_get_item (listitem));
    }

  auto bin = Z_POPOVER_MENU_BIN_WIDGET (gtk_list_item_get_child (listitem));

  switch (self->type_)
    {
    case ItemFactory::Type::Toggle:
      {
        auto check_btn = popover_menu_bin_widget_get_child (bin);

        switch (obj->type)
          {
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_MIDI_MAPPING:
            {
              if (self->column_name_ == _ ("On"))
                {
                  auto mapping = static_cast<MidiMapping *> (obj->obj);
                  gtk_check_button_set_active (
                    GTK_CHECK_BUTTON (check_btn), mapping->enabled_);
                }
            }
            break;
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_TRACK:
            {
              if (self->column_name_ == _ ("Visibility"))
                {
                  auto track = static_cast<Track *> (obj->obj);
                  gtk_check_button_set_active (
                    GTK_CHECK_BUTTON (check_btn), track->visible_);
                }
            }
            break;
          default:
            break;
          }

        auto data = new ItemFactoryData (*self, *obj);
        g_object_set_data (G_OBJECT (bin), "item-factory-data", data);
        g_signal_connect_data (
          G_OBJECT (check_btn), "toggled", G_CALLBACK (on_toggled), data,
          ItemFactoryData::destroy_closure, G_CONNECT_AFTER);
      }
      break;
    case ItemFactory::Type::Text:
    case ItemFactory::Type::Integer:
    case ItemFactory::Type::Position:
      {
        auto label = popover_menu_bin_widget_get_child (bin);

        std::string str;
        switch (obj->type)
          {
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_MIDI_MAPPING:
            {
              auto mm = static_cast<MidiMapping *> (obj->obj);
              if (self->column_name_ == _ ("Note/Control"))
                {
                  char ctrl[60];
                  int ctrl_change_ch = midi_ctrl_change_get_ch_and_description (
                    mm->key_.data (), ctrl);
                  if (ctrl_change_ch > 0)
                    {
                      str = fmt::format (
                        "{:02X}-{:02X} ({})", mm->key_[0], mm->key_[1], ctrl);
                    }
                  else
                    {
                      str =
                        fmt::format ("{:02X}-{:02X}", mm->key_[0], mm->key_[1]);
                    }
                }
              if (self->column_name_ == _ ("Destination"))
                {
                  auto port = Port::find_from_identifier (mm->dest_id_);
                  str = port->get_full_designation ();

                  // str += fmt::format (" ({:.4f} - {:.4f})", port->minf_,
                  // port->maxf_);
                }
            }
            break;
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_ARRANGER_OBJECT:
            {
              auto arr_obj = static_cast<ArrangerObject *> (obj->obj);

              if (self->column_name_ == _ ("Note"))
                {
                  auto mn = dynamic_cast<MidiNote *> (arr_obj);
                  str = mn->get_val_as_string (
                    PianoRoll::NoteNotation::Musical, false);
                }
              else if (self->column_name_ == _ ("Pitch"))
                {
                  auto mn = dynamic_cast<MidiNote *> (arr_obj);
                  str = fmt::format ("{}", mn->val_);
                }
              else if (
                self->column_name_ == _ ("Start")
                || self->column_name_ == _ ("Position"))
                {
                  str = arr_obj->pos_.to_string (3);
                }
              else if (self->column_name_ == _ ("End"))
                {
                  if (arr_obj->has_length ())
                    {
                      auto lengthable_object =
                        dynamic_cast<LengthableObject *> (arr_obj);
                      str = lengthable_object->end_pos_.to_string (3);
                    }
                  else
                    {
                      str = _ ("N/A");
                    }
                }
              else if (self->column_name_ == _ ("Loop start"))
                {
                  if (arr_obj->can_loop ())
                    {
                      auto loopable = dynamic_cast<LoopableObject *> (arr_obj);
                      str = loopable->loop_start_pos_.to_string (3);
                    }
                  else
                    {
                      str = _ ("N/A");
                    }
                }
              else if (self->column_name_ == _ ("Loop end"))
                {
                  if (arr_obj->can_loop ())
                    {
                      auto loopable = dynamic_cast<LoopableObject *> (arr_obj);
                      str = loopable->loop_end_pos_.to_string (3);
                    }
                  else
                    {
                      str = _ ("N/A");
                    }
                }
              else if (self->column_name_ == _ ("Clip start"))
                {
                  if (arr_obj->can_loop ())
                    {
                      auto loopable = dynamic_cast<LoopableObject *> (arr_obj);
                      str = loopable->clip_start_pos_.to_string (3);
                    }
                  else
                    {
                      str = _ ("N/A");
                    }
                }
              else if (self->column_name_ == _ ("Fade in"))
                {
                  if (arr_obj->can_fade ())
                    {
                      auto fadeable = dynamic_cast<FadeableObject *> (arr_obj);
                      str = fadeable->fade_in_pos_.to_string (3);
                    }
                  else
                    {
                      str = _ ("N/A");
                    }
                }
              else if (self->column_name_ == _ ("Fade out"))
                {
                  if (arr_obj->can_fade ())
                    {
                      auto fadeable = dynamic_cast<FadeableObject *> (arr_obj);
                      str = fadeable->fade_out_pos_.to_string (3);
                    }
                  else
                    {
                      str = _ ("N/A");
                    }
                }
              else if (self->column_name_ == _ ("Name"))
                {
                  str = arr_obj->gen_human_friendly_name ();
                }
              else if (self->column_name_ == _ ("Type"))
                {
                  str = _ (arr_obj->get_type_as_string ());
                }
              else if (self->column_name_ == _ ("Velocity"))
                {
                  auto mn = dynamic_cast<MidiNote *> (arr_obj);
                  str = fmt::format ("{}", mn->vel_->vel_);
                }
              else if (self->column_name_ == _ ("Index"))
                {
                  auto ap = dynamic_cast<AutomationPoint *> (arr_obj);
                  str = fmt::format ("{}", ap->index_);
                }
              else if (self->column_name_ == _ ("Value"))
                {
                  auto ap = dynamic_cast<AutomationPoint *> (arr_obj);
                  str = fmt::format ("{}", ap->fvalue_);
                }
              else if (self->column_name_ == _ ("Curve type"))
                {
                  auto ap = dynamic_cast<AutomationPoint *> (arr_obj);
                  str = CurveOptions_Algorithm_to_string (
                    ap->curve_opts_.algo_, true);
                }
              else if (self->column_name_ == _ ("Curviness"))
                {
                  auto ap = dynamic_cast<AutomationPoint *> (arr_obj);
                  str = fmt::format ("{}", ap->curve_opts_.curviness_);
                }
            }
            break;
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_PROJECT_INFO:
            {
              auto nfo = static_cast<ProjectInfo *> (obj->obj);

              if (
                self->column_name_ == _ ("Name")
                || self->column_name_ == _ ("Template Name"))
                {
                  str = nfo->name;
                }
              else if (self->column_name_ == _ ("Path"))
                {
                  str = nfo->filename;
                }
              else if (self->column_name_ == _ ("Last Modified"))
                {
                  str = nfo->modified_str;
                }
            }
            break;
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_TRACK:
            {
              auto track = static_cast<Track *> (obj->obj);

              if (self->column_name_ == _ ("Name"))
                {
                  str = track->name_;
                }
            }
            break;
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_PORT:
            {
              auto port = static_cast<Port *> (obj->obj);
              str = port->get_full_designation ();
            }
            break;
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_PLUGIN_COLLECTION:
            {
              auto coll = static_cast<PluginCollection *> (obj->obj);
              str = coll->get_name ();
              add_plugin_collection_context_menu (bin, coll);
            }
            break;
          default:
            break;
          }

        if (self->editable_)
          {
            gtk_editable_set_text (GTK_EDITABLE (label), str.c_str ());
            auto data = new ItemFactoryData (*self, *obj);
            g_object_set_data (G_OBJECT (bin), "item-factory-data", data);
            g_signal_handlers_disconnect_by_func (
              bin, (gpointer) on_editable_label_editing_changed, data);
            g_signal_connect_data (
              G_OBJECT (label), "notify::editing",
              G_CALLBACK (on_editable_label_editing_changed), data,
              ItemFactoryData::destroy_closure, G_CONNECT_AFTER);
          }
        else
          {
            gtk_label_set_text (GTK_LABEL (label), str.c_str ());
          }
      }
      break;
    case ItemFactory::Type::IconAndText:
      {
        auto box = GTK_BOX (popover_menu_bin_widget_get_child (bin));
        auto img = GTK_IMAGE (gtk_widget_get_first_child (GTK_WIDGET (box)));
        auto lbl = GTK_LABEL (gtk_widget_get_last_child (GTK_WIDGET (box)));

        switch (obj->type)
          {
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_PLUGIN_DESCR:
            {
              auto descr = static_cast<PluginDescriptor *> (obj->obj);
              gtk_image_set_from_icon_name (
                img, descr->get_icon_name ().c_str ());
              gtk_label_set_text (lbl, descr->name_.c_str ());

              /* set as drag source */
              auto drag_source = gtk_drag_source_new ();
              gtk_drag_source_set_actions (
                drag_source, GDK_ACTION_COPY | GDK_ACTION_MOVE);
              auto data = new ItemFactoryData (*self, *obj);
              g_object_set_data (G_OBJECT (bin), "item-factory-data", data);
              g_signal_connect_data (
                G_OBJECT (drag_source), "prepare",
                G_CALLBACK (on_dnd_drag_prepare), data,
                ItemFactoryData::destroy_closure, (GConnectFlags) 0);
              g_object_set_data (G_OBJECT (bin), "drag-source", drag_source);
              gtk_widget_add_controller (
                GTK_WIDGET (bin), GTK_EVENT_CONTROLLER (drag_source));

              add_plugin_descr_context_menu (bin, descr);
            }
            break;
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_SUPPORTED_FILE:
            {
              auto descr = static_cast<FileDescriptor *> (obj->obj);

              gtk_image_set_from_icon_name (img, descr->get_icon_name ());
              gtk_label_set_text (lbl, descr->label_.c_str ());

              /* set as drag source */
              auto drag_source = gtk_drag_source_new ();
              gtk_drag_source_set_actions (
                drag_source, GDK_ACTION_COPY | GDK_ACTION_MOVE);
              auto data = new ItemFactoryData (*self, *obj);
              g_object_set_data (G_OBJECT (bin), "item-factory-data", data);
              g_signal_connect_data (
                G_OBJECT (drag_source), "prepare",
                G_CALLBACK (on_dnd_drag_prepare), data,
                ItemFactoryData::destroy_closure, (GConnectFlags) 0);
              g_object_set_data (G_OBJECT (bin), "drag-source", drag_source);
              gtk_widget_add_controller (
                GTK_WIDGET (bin), GTK_EVENT_CONTROLLER (drag_source));

              add_supported_file_context_menu (bin, descr);
            }
            break;
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_CHORD_PSET_PACK:
            {
              auto pack = static_cast<ChordPresetPack *> (obj->obj);

              gtk_image_set_from_icon_name (img, "minuet-chords");
              gtk_label_set_text (lbl, pack->get_name ().c_str ());

              add_chord_pset_pack_context_menu (bin, pack);
            }
            break;
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_CHORD_PSET:
            {
              auto pset = static_cast<ChordPreset *> (obj->obj);

              gtk_image_set_from_icon_name (img, "minuet-chords");
              gtk_label_set_text (lbl, pset->name_.c_str ());

              add_chord_pset_context_menu (bin, pset);
            }
            break;
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_CHANNEL_SEND_TARGET:
            {
              auto target = (ChannelSend::Target *) obj->obj;

              auto icon_name = target->get_icon ();
              gtk_image_set_from_icon_name (img, icon_name.c_str ());

              auto label = target->describe ();
              gtk_label_set_text (lbl, label.c_str ());
            }
            break;
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_FILE_BROWSER_LOCATION:
            {
              auto * loc = (FileBrowserLocation *) obj->obj;

              const char * icon_name = loc->get_icon_name ();
              gtk_image_set_from_icon_name (img, icon_name);

              gtk_label_set_text (lbl, loc->label_.c_str ());
              add_file_browser_location_context_menu (bin, loc);
            }
            break;
          default:
            break;
          }
      }
      break;
    case ItemFactory::Type::Icon:
      {
        auto img = GTK_IMAGE (popover_menu_bin_widget_get_child (bin));

        switch (obj->type)
          {
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_TRACK:
            {
              auto * track = (Track *) obj->obj;
              gtk_image_set_from_icon_name (img, track->icon_name_.c_str ());
            }
            break;
          default:
            break;
          }
      }
      break;
    case ItemFactory::Type::Color:
      {
        auto ca = Z_COLOR_AREA_WIDGET (popover_menu_bin_widget_get_child (bin));

        switch (obj->type)
          {
          case WrappedObjectType::WRAPPED_OBJECT_TYPE_TRACK:
            {
              auto * track = (Track *) obj->obj;
              color_area_widget_setup_generic (ca, &track->color_);
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

  switch (self->type_)
    {
    case ItemFactory::Type::Toggle:
      {
        gpointer data = g_object_get_data (G_OBJECT (bin), "item-factory-data");
        g_signal_handlers_disconnect_by_func (
          widget, (gpointer) on_toggled, data);
      }
      break;
    case ItemFactory::Type::Text:
    case ItemFactory::Type::Position:
    case ItemFactory::Type::Integer:
      {
        gpointer data = g_object_get_data (G_OBJECT (bin), "item-factory-data");
        g_signal_handlers_disconnect_by_func (
          widget, (gpointer) on_editable_label_editing_changed, data);
      }
      break;
    case ItemFactory::Type::IconAndText:
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
    case ItemFactory::Type::Icon:
    case ItemFactory::Type::Color:
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
  gtk_list_item_set_child (listitem, nullptr);
}

ItemFactory::ItemFactory (Type type, bool editable, std::string column_name)
    : type_ (type), editable_ (editable), column_name_ (std::move (column_name))
{
  list_item_factory_ = gtk_signal_list_item_factory_new ();

  setup_id_ = g_signal_connect (
    G_OBJECT (list_item_factory_), "setup", G_CALLBACK (item_factory_setup_cb),
    this);
  bind_id_ = g_signal_connect (
    G_OBJECT (list_item_factory_), "bind", G_CALLBACK (item_factory_bind_cb),
    this);
  unbind_id_ = g_signal_connect (
    G_OBJECT (list_item_factory_), "unbind",
    G_CALLBACK (item_factory_unbind_cb), this);
  teardown_id_ = g_signal_connect (
    G_OBJECT (list_item_factory_), "teardown",
    G_CALLBACK (item_factory_teardown_cb), this);
}

ItemFactory::~ItemFactory ()
{
  g_object_unref (list_item_factory_);
}

std::unique_ptr<ItemFactory> &
ItemFactory::generate_and_append_column (
  GtkColumnView *                            column_view,
  std::vector<std::unique_ptr<ItemFactory>> &item_factories,
  ItemFactory::Type                          type,
  bool                                       editable,
  bool                                       resizable,
  GtkSorter *                                sorter,
  std::string                                column_name)
{
  auto item_factory =
    std::make_unique<ItemFactory> (type, editable, std::move (column_name));
  GtkListItemFactory *  list_item_factory = item_factory->list_item_factory_;
  GtkColumnViewColumn * column =
    gtk_column_view_column_new (column_name.c_str (), list_item_factory);
  gtk_column_view_column_set_resizable (column, true);
  gtk_column_view_column_set_expand (column, true);
  if (sorter)
    {
      gtk_column_view_column_set_sorter (column, sorter);
    }
  gtk_column_view_append_column (column_view, column);
  item_factories.emplace_back (std::move (item_factory));
  return item_factories.back ();
}