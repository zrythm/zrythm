// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "common/dsp/engine.h"
#include "common/dsp/hardware_processor.h"
#include "common/dsp/track.h"
#include "common/utils/gtk.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/wrapped_object_with_change_signal.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/gtk_widgets/ext_input_selection_dropdown.h"

static char *
get_str (void * data)
{
  if (Z_IS_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data))
    {
      WrappedObjectWithChangeSignal * wrapped_obj =
        Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (data);
      return wrapped_object_with_change_signal_get_display_name (wrapped_obj);
    }
  else if (GTK_IS_STRING_OBJECT (data))
    {
      GtkStringObject * str_obj = GTK_STRING_OBJECT (data);
      return g_strdup (gtk_string_object_get_string (str_obj));
    }
  z_return_val_if_reached (nullptr);
}

static void
on_ext_input_changed (
  Track *       track,
  GtkDropDown * dropdown,
  const bool    midi,
  const bool    left)
{
  auto idx = gtk_drop_down_get_selected (dropdown);

  z_debug ("ext input: {} (midi? {} left? {})", idx, midi, left);

  auto ch_track = dynamic_cast<ChannelTrack *> (track);
  z_return_if_fail (ch_track);
  auto &ch = *ch_track->get_channel ();
  if (idx == 0)
    {
      if (midi)
        {
          if (ch.all_midi_ins_)
            return;
          ch.all_midi_ins_ = true;
        }
      else if (left)
        {
          if (ch.all_stereo_l_ins_)
            return;
          ch.all_stereo_l_ins_ = true;
        }
      else
        {
          if (ch.all_stereo_r_ins_)
            return;
          ch.all_stereo_r_ins_ = true;
        }
    }
  else if (idx == 1)
    {
      if (midi)
        {
          ch.all_midi_ins_ = false;
          ch.ext_midi_ins_.clear ();
        }
      else if (left)
        {
          ch.all_stereo_l_ins_ = false;
          ch.ext_stereo_l_ins_.clear ();
        }
      else
        {
          ch.all_stereo_r_ins_ = false;
          ch.ext_stereo_r_ins_.clear ();
        }
    }
  else
    {
      if (midi)
        {
          ch.ext_midi_ins_.clear ();
          ch.all_midi_ins_ = false;
        }
      else if (left)
        {
          ch.ext_stereo_l_ins_.clear ();
          ch.all_stereo_l_ins_ = false;
        }
      else
        {
          ch.ext_stereo_r_ins_.clear ();
          ch.all_stereo_r_ins_ = false;
        }

      auto wobj = Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (
        gtk_drop_down_get_selected_item (dropdown));
      auto port = std::get<ExtPort *> (wobj->obj);
      if (midi)
        {
          ch.ext_midi_ins_.emplace_back (std::make_unique<ExtPort> (*port));
        }
      else if (left)
        {
          ch.ext_stereo_l_ins_.emplace_back (std::make_unique<ExtPort> (*port));
        }
      else
        {
          ch.ext_stereo_r_ins_.emplace_back (std::make_unique<ExtPort> (*port));
        }
    }

  ch.reconnect_ext_input_ports ();
}

static void
on_midi_input_changed (GtkDropDown * gobject, GParamSpec * pspec, Track * self)
{
  on_ext_input_changed (self, gobject, true, false);
}

static void
on_stereo_l_input_changed (GtkDropDown * gobject, GParamSpec * pspec, Track * self)
{
  on_ext_input_changed (self, gobject, false, true);
}

static void
on_stereo_r_input_changed (GtkDropDown * gobject, GParamSpec * pspec, Track * self)
{
  on_ext_input_changed (self, gobject, false, false);
}

static void
on_header_bind (
  GtkSignalListItemFactory * factory,
  GObject *                  list_item,
  gpointer                   user_data)
{
  GtkListHeader * self = GTK_LIST_HEADER (list_item);
  GtkWidget *     child = gtk_list_header_get_child (self);
  GObject *       item = G_OBJECT (gtk_list_header_get_item (self));

  if (GTK_IS_STRING_OBJECT (item))
    {
      gtk_label_set_markup (GTK_LABEL (child), _ ("Macros"));
    }
  else
    {
      gtk_label_set_markup (GTK_LABEL (child), _ ("Device Inputs"));
    }
}

static void
on_bind (
  GtkSignalListItemFactory * factory,
  GObject *                  list_item,
  gpointer                   user_data)
{
  GtkListItem * self = GTK_LIST_ITEM (list_item);
  GtkWidget *   child = gtk_list_item_get_child (self);
  GObject *     item = G_OBJECT (gtk_list_item_get_item (self));

  char * str = get_str (item);
  gtk_label_set_markup (GTK_LABEL (child), get_str (item));
  g_free (str);
}

static void
preselect_current_port (GtkDropDown * dropdown, ExtPort * ch_port)
{
  GListModel * model = gtk_drop_down_get_model (dropdown);
  guint        n_items = g_list_model_get_n_items (model);
  bool         found = false;
  for (size_t i = 2; i < n_items; i++)
    {
      WrappedObjectWithChangeSignal * wobj =
        Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (g_list_model_get_object (model, i));
      ExtPort * port = std::get<ExtPort *> (wobj->obj);
      if (*port == *ch_port)
        {
          gtk_drop_down_set_selected (dropdown, i);
          found = true;
          break;
        }
    }
  if (!found)
    gtk_drop_down_set_selected (dropdown, 2);
}

void
ext_input_selection_dropdown_widget_refresh (
  GtkDropDown *  dropdown,
  ChannelTrack * track,
  bool           left)
{
  bool midi = track->in_signal_type_ == PortType::Event;

  /* --- disconnect existing signals --- */

  auto cur_track =
    (ChannelTrack *) g_object_get_data (G_OBJECT (dropdown), "cur-track");
  if (cur_track)
    {
      g_signal_handlers_disconnect_by_data (dropdown, cur_track);
    }
  g_object_set_data (G_OBJECT (dropdown), "cur-track", track);

  /* --- set header factory --- */

  GtkListItemFactory * header_factory = gtk_signal_list_item_factory_new ();
  g_signal_connect (
    header_factory, "setup",
    G_CALLBACK (z_gtk_drop_down_list_item_header_setup_common), nullptr);
  g_signal_connect (
    header_factory, "bind", G_CALLBACK (on_header_bind), nullptr);
  gtk_drop_down_set_header_factory (dropdown, header_factory);
  g_object_unref (header_factory);

  /* --- set normal factory --- */

  GtkListItemFactory * factory = gtk_signal_list_item_factory_new ();
  g_signal_connect (
    factory, "setup",
    G_CALLBACK (z_gtk_drop_down_factory_setup_common_ellipsized), nullptr);
  g_signal_connect (factory, "bind", G_CALLBACK (on_bind), nullptr);
  gtk_drop_down_set_factory (dropdown, factory);
  g_object_unref (factory);

  /* --- set list factory --- */

  factory = gtk_signal_list_item_factory_new ();
  g_signal_connect (
    factory, "setup", G_CALLBACK (z_gtk_drop_down_factory_setup_common),
    nullptr);
  g_signal_connect (factory, "bind", G_CALLBACK (on_bind), nullptr);
  gtk_drop_down_set_list_factory (dropdown, factory);
  g_object_unref (factory);

  /* --- set closure for search FIXME makes the dropdown not use factories --- */

#if 0
  GtkExpression * expression = gtk_cclosure_expression_new (
    G_TYPE_STRING, nullptr, 0, nullptr, G_CALLBACK (get_str), nullptr, nullptr);
  gtk_drop_down_set_expression (dropdown, expression);
  gtk_expression_unref (expression);

  gtk_drop_down_set_enable_search (dropdown, true);
#endif

  /* --- create models --- */

  GtkStringList * standard_sl = gtk_string_list_new (nullptr);
  if (midi)
    {
      gtk_string_list_append (standard_sl, _ ("All MIDI Inputs"));
      gtk_string_list_append (standard_sl, _ ("No Inputs"));
    }
  else
    {
      gtk_string_list_append (standard_sl, _ ("All Audio Inputs"));
      gtk_string_list_append (
        standard_sl, left ? _ ("No left input") : _ ("No right input"));
    }

  /* get all ext inputs */
  GListStore * ext_inputs_ls =
    g_list_store_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);
  for (
    auto &port :
    midi ? HW_IN_PROCESSOR->ext_midi_ports_ : HW_IN_PROCESSOR->ext_audio_ports_)
    {
      WrappedObjectWithChangeSignal * wobj =
        wrapped_object_with_change_signal_new (
          port.get (), WrappedObjectType::WRAPPED_OBJECT_TYPE_EXT_PORT);
      g_list_store_append (ext_inputs_ls, wobj);
    }

  GListStore * composite_ls = g_list_store_new (G_TYPE_LIST_MODEL);
  g_list_store_append (composite_ls, standard_sl);
  g_list_store_append (composite_ls, ext_inputs_ls);

  GtkFlattenListModel * flatten_model =
    gtk_flatten_list_model_new (G_LIST_MODEL (composite_ls));
  gtk_drop_down_set_model (dropdown, G_LIST_MODEL (flatten_model));

  /* --- preselect the current value --- */

  /* select the correct value */
  auto ch = track->channel_;
  z_debug ("selecting ext input...");
  if (midi)
    {
      if (ch->all_midi_ins_)
        gtk_drop_down_set_selected (dropdown, 0);
      else if (ch->ext_midi_ins_.empty ())
        gtk_drop_down_set_selected (dropdown, 1);
      else
        preselect_current_port (dropdown, ch->ext_midi_ins_[0].get ());
    }
  else if (left)
    {
      if (ch->all_stereo_l_ins_)
        gtk_drop_down_set_selected (dropdown, 0);
      else if (ch->ext_stereo_l_ins_.empty ())
        gtk_drop_down_set_selected (dropdown, 1);
      else
        preselect_current_port (dropdown, ch->ext_stereo_l_ins_[0].get ());
    }
  else
    {
      if (ch->all_stereo_r_ins_)
        gtk_drop_down_set_selected (dropdown, 0);
      else if (ch->ext_stereo_r_ins_.empty ())
        gtk_drop_down_set_selected (dropdown, 1);
      else
        preselect_current_port (dropdown, ch->ext_stereo_r_ins_[0].get ());
    }

  /* --- add signal --- */

  if (midi)
    {
      g_signal_connect (
        dropdown, "notify::selected-item", G_CALLBACK (on_midi_input_changed),
        track);
    }
  else if (left)
    {
      g_signal_connect (
        dropdown, "notify::selected-item",
        G_CALLBACK (on_stereo_l_input_changed), track);
    }
  else
    {
      g_signal_connect (
        dropdown, "notify::selected-item",
        G_CALLBACK (on_stereo_r_input_changed), track);
    }
}
