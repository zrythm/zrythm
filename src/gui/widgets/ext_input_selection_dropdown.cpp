// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/engine.h"
#include "dsp/hardware_processor.h"
#include "dsp/track.h"
#include "gui/backend/wrapped_object_with_change_signal.h"
#include "gui/widgets/ext_input_selection_dropdown.h"
#include "project.h"
#include "utils/gtk.h"

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
  g_return_val_if_reached (NULL);
}

static void
on_ext_input_changed (
  Track *       track,
  GtkDropDown * dropdown,
  const int     midi,
  const int     left)
{
  guint idx = gtk_drop_down_get_selected (dropdown);

  g_debug ("ext input: %u (midi? %d left? %d)", idx, midi, left);

  Channel * ch = track->channel;
  if (idx == 0)
    {
      if (midi)
        {
          if (ch->all_midi_ins)
            return;
          ch->all_midi_ins = 1;
        }
      else if (left)
        {
          if (ch->all_stereo_l_ins)
            return;
          ch->all_stereo_l_ins = 1;
        }
      else
        {
          if (ch->all_stereo_r_ins)
            return;
          ch->all_stereo_r_ins = 1;
        }
    }
  else if (idx == 1)
    {
      if (midi)
        {
          ch->all_midi_ins = 0;
          ext_ports_free (ch->ext_midi_ins, ch->num_ext_midi_ins);
          ch->num_ext_midi_ins = 0;
        }
      else if (left)
        {
          ch->all_stereo_l_ins = 0;
          ext_ports_free (ch->ext_stereo_l_ins, ch->num_ext_stereo_l_ins);
          ch->num_ext_stereo_l_ins = 0;
        }
      else
        {
          ch->all_stereo_r_ins = 0;
          ext_ports_free (ch->ext_stereo_r_ins, ch->num_ext_stereo_r_ins);
          ch->num_ext_stereo_r_ins = 0;
        }
    }
  else
    {
      if (midi)
        {
          ext_ports_free (ch->ext_midi_ins, ch->num_ext_midi_ins);
          ch->all_midi_ins = 0;
        }
      else if (left)
        {
          ext_ports_free (ch->ext_stereo_l_ins, ch->num_ext_stereo_l_ins);
          ch->all_stereo_l_ins = 0;
        }
      else
        {
          ext_ports_free (ch->ext_stereo_r_ins, ch->num_ext_stereo_r_ins);
          ch->all_stereo_r_ins = 0;
        }

      WrappedObjectWithChangeSignal * wobj = Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (
        gtk_drop_down_get_selected_item (dropdown));
      ExtPort * port = (ExtPort *) wobj->obj;
      if (midi)
        {
          ch->num_ext_midi_ins = 1;
          ch->ext_midi_ins[0] = ext_port_clone (port);
        }
      else if (left)
        {
          ch->num_ext_stereo_l_ins = 1;
          ch->ext_stereo_l_ins[0] = ext_port_clone (port);
        }
      else
        {
          ch->num_ext_stereo_r_ins = 1;
          ch->ext_stereo_r_ins[0] = ext_port_clone (port);
        }
    }

  ch->reconnect_ext_input_ports ();
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
      ExtPort * port = (ExtPort *) wobj->obj;
      if (ext_ports_equal (port, ch_port))
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
  GtkDropDown * dropdown,
  Track *       track,
  bool          left)
{
  bool midi = track->in_signal_type == ZPortType::Z_PORT_TYPE_EVENT;

  /* --- disconnect existing signals --- */

  Track * cur_track =
    (Track *) g_object_get_data (G_OBJECT (dropdown), "cur-track");
  if (cur_track)
    {
      g_signal_handlers_disconnect_by_data (dropdown, cur_track);
    }
  g_object_set_data (G_OBJECT (dropdown), "cur-track", track);

  /* --- set header factory --- */

  GtkListItemFactory * header_factory = gtk_signal_list_item_factory_new ();
  g_signal_connect (
    header_factory, "setup",
    G_CALLBACK (z_gtk_drop_down_list_item_header_setup_common), NULL);
  g_signal_connect (header_factory, "bind", G_CALLBACK (on_header_bind), NULL);
  gtk_drop_down_set_header_factory (dropdown, header_factory);
  g_object_unref (header_factory);

  /* --- set normal factory --- */

  GtkListItemFactory * factory = gtk_signal_list_item_factory_new ();
  g_signal_connect (
    factory, "setup",
    G_CALLBACK (z_gtk_drop_down_factory_setup_common_ellipsized), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (on_bind), NULL);
  gtk_drop_down_set_factory (dropdown, factory);
  g_object_unref (factory);

  /* --- set list factory --- */

  factory = gtk_signal_list_item_factory_new ();
  g_signal_connect (
    factory, "setup", G_CALLBACK (z_gtk_drop_down_factory_setup_common), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (on_bind), NULL);
  gtk_drop_down_set_list_factory (dropdown, factory);
  g_object_unref (factory);

  /* --- set closure for search FIXME makes the dropdown not use factories --- */

#if 0
  GtkExpression * expression = gtk_cclosure_expression_new (
    G_TYPE_STRING, NULL, 0, NULL, G_CALLBACK (get_str), NULL, NULL);
  gtk_drop_down_set_expression (dropdown, expression);
  gtk_expression_unref (expression);

  gtk_drop_down_set_enable_search (dropdown, true);
#endif

  /* --- create models --- */

  GtkStringList * standard_sl = gtk_string_list_new (NULL);
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
    int i = 0;
    i
    < (midi ? HW_IN_PROCESSOR->num_ext_midi_ports : HW_IN_PROCESSOR->num_ext_audio_ports);
    i++)
    {
      ExtPort * port =
        midi ? HW_IN_PROCESSOR->ext_midi_ports[i]
             : HW_IN_PROCESSOR->ext_audio_ports[i];

      WrappedObjectWithChangeSignal * wobj =
        wrapped_object_with_change_signal_new (
          port, WrappedObjectType::WRAPPED_OBJECT_TYPE_EXT_PORT);
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
  Channel * ch = track->channel;
  g_debug ("selecting ext input...");
  if (midi)
    {
      if (ch->all_midi_ins)
        gtk_drop_down_set_selected (dropdown, 0);
      else if (ch->num_ext_midi_ins == 0)
        gtk_drop_down_set_selected (dropdown, 1);
      else
        preselect_current_port (dropdown, ch->ext_midi_ins[0]);
    }
  else if (left)
    {
      if (ch->all_stereo_l_ins)
        gtk_drop_down_set_selected (dropdown, 0);
      else if (ch->num_ext_stereo_l_ins == 0)
        gtk_drop_down_set_selected (dropdown, 1);
      else
        preselect_current_port (dropdown, ch->ext_stereo_l_ins[0]);
    }
  else
    {
      if (ch->all_stereo_r_ins)
        gtk_drop_down_set_selected (dropdown, 0);
      else if (ch->num_ext_stereo_r_ins == 0)
        gtk_drop_down_set_selected (dropdown, 1);
      else
        preselect_current_port (dropdown, ch->ext_stereo_r_ins[0]);
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
