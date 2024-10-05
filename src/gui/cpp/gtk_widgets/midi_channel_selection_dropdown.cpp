// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/cpp/gtk_widgets/midi_channel_selection_dropdown.h"

#include "common/dsp/channel_track.h"
#include "common/utils/gtk.h"
#include "common/utils/string.h"

static char *
get_str (void * data)
{
  if (GTK_IS_STRING_OBJECT (data))
    {
      GtkStringObject * str_obj = GTK_STRING_OBJECT (data);
      return g_strdup (gtk_string_object_get_string (str_obj));
    }
  z_return_val_if_reached (nullptr);
}

static void
on_midi_channel_changed (
  GtkDropDown *  dropdown,
  GParamSpec *   pspec,
  ChannelTrack * track)
{
  guint idx = gtk_drop_down_get_selected (dropdown);

  auto ch = track->channel_;
  if (idx == 0)
    {
      if (ch->all_midi_channels_)
        return;
      ch->all_midi_ins_ = true;
    }
  else
    {
      /* clear */
      ch->all_midi_channels_ = false;
      for (unsigned int i = 0; i < 16; i++)
        {
          ch->midi_channels_[i] = false;
        }

      if (idx > 1)
        {
          for (unsigned int i = 0; i < 16; i++)
            {
              if (idx == i + 2)
                {
                  ch->midi_channels_[i] = true;
                  break;
                }
            }
        }
    }

  ch->reconnect_ext_input_ports ();
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

  const char * str = gtk_string_object_get_string (GTK_STRING_OBJECT (item));
  if (string_is_equal (str, _ ("All Channels")))
    {
      gtk_label_set_markup (GTK_LABEL (child), _ ("Macros"));
    }
  else
    {
      gtk_label_set_markup (GTK_LABEL (child), _ ("Channels"));
    }
}

void
midi_channel_selection_dropdown_widget_refresh (
  GtkDropDown *  dropdown,
  ChannelTrack * track)
{
  /* --- disconnect existing signals --- */

  auto * cur_track =
    (ChannelTrack *) g_object_get_data (G_OBJECT (dropdown), "cur-track");
  if (cur_track)
    {
      g_signal_handlers_disconnect_by_data (dropdown, cur_track);
    }
  g_object_set_data (G_OBJECT (dropdown), "cur-track", track);

  /* --- create models --- */

  GtkStringList * standard_sl = gtk_string_list_new (nullptr);
  gtk_string_list_append (standard_sl, _ ("All Channels"));
  gtk_string_list_append (standard_sl, _ ("No Channels"));

  /* get all ext inputs */
  GtkStringList * midi_channels_sl = gtk_string_list_new (nullptr);
  for (int i = 0; i < 16; i++)
    {
      char lbl[128];
      snprintf (lbl, 128, _ ("Channel %d"), i + 1);
      gtk_string_list_append (midi_channels_sl, lbl);
    }

  GListStore * composite_ls = g_list_store_new (G_TYPE_LIST_MODEL);
  g_list_store_append (composite_ls, standard_sl);
  g_list_store_append (composite_ls, midi_channels_sl);

  GtkFlattenListModel * flatten_model =
    gtk_flatten_list_model_new (G_LIST_MODEL (composite_ls));

  GtkExpression * expression = gtk_cclosure_expression_new (
    G_TYPE_STRING, nullptr, 0, nullptr, G_CALLBACK (get_str), nullptr, nullptr);
  gtk_drop_down_set_expression (dropdown, expression);
  gtk_expression_unref (expression);

  gtk_drop_down_set_model (dropdown, G_LIST_MODEL (flatten_model));

  /* --- set header factory --- */

  GtkListItemFactory * header_factory = gtk_signal_list_item_factory_new ();
  g_signal_connect (
    header_factory, "setup",
    G_CALLBACK (z_gtk_drop_down_list_item_header_setup_common), nullptr);
  g_signal_connect (
    header_factory, "bind", G_CALLBACK (on_header_bind), nullptr);
  gtk_drop_down_set_header_factory (dropdown, header_factory);
  g_object_unref (header_factory);

  /* --- preselect the current value --- */

  /* select the correct value */
  auto ch = track->channel_;
  z_debug ("selecting MIDI channel...");
  if (ch->all_midi_channels_)
    gtk_drop_down_set_selected (dropdown, 0);
  else
    {
      bool found = false;
      for (unsigned int i = 0; i < 16; i++)
        {
          if (ch->midi_channels_[i])
            {
              gtk_drop_down_set_selected (dropdown, i + 2);
              found = true;
              break;
            }
        }
      if (!found)
        gtk_drop_down_set_selected (dropdown, 1);
    }

  /* --- add signal --- */

  g_signal_connect (
    dropdown, "notify::selected-item", G_CALLBACK (on_midi_channel_changed),
    track);
}
