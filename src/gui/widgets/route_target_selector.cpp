// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/tracklist_selections.h"
#include "dsp/channel.h"
#include "dsp/master_track.h"
#include "dsp/track.h"
#include "dsp/tracklist.h"
#include "gui/backend/wrapped_object_with_change_signal.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/route_target_selector.h"
#include "project.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

G_DEFINE_TYPE (
  RouteTargetSelectorWidget,
  route_target_selector_widget,
  ADW_TYPE_BIN)

static void
on_route_target_changed (
  GtkDropDown *               dropdown,
  GParamSpec *                pspec,
  RouteTargetSelectorWidget * self)
{
  if (!self->track)
    return;

  Track * old_direct_out = channel_get_output_track (self->track->channel);
  Track * new_direct_out = old_direct_out;
  if (gtk_drop_down_get_selected (dropdown) == 0)
    {
      new_direct_out = NULL;
    }
  else
    {
      WrappedObjectWithChangeSignal * wobj = Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (
        gtk_drop_down_get_selected_item (dropdown));
      new_direct_out = (Track *) wobj->obj;
    }

  if (new_direct_out != old_direct_out)
    {
      Track *               own_track_clone = track_clone (self->track, NULL);
      TracklistSelections * sel = tracklist_selections_new (false);
      tracklist_selections_add_track (sel, own_track_clone, F_NO_PUBLISH_EVENTS);
      GError * err = NULL;
      bool     success = tracklist_selections_action_perform_set_direct_out (
        sel, PORT_CONNECTIONS_MGR, new_direct_out, &err);
      if (!success)
        {
          HANDLE_ERROR (
            err, _ ("Failed to change direct out to %s"),
            new_direct_out ? new_direct_out->name : _ ("None"));
        }
    }
}

static void
on_header_bind (
  GtkSignalListItemFactory * factory,
  GtkListHeader *            header,
  gpointer                   user_data)
{
  GtkLabel * label = GTK_LABEL (gtk_list_header_get_child (header));
  GObject *  item = G_OBJECT (gtk_list_header_get_item (header));

  if (GTK_IS_STRING_OBJECT (item))
    {
      gtk_label_set_markup (label, _ ("None"));
    }
  else
    {
      WrappedObjectWithChangeSignal * wobj =
        Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (item);
      Track * track = (Track *) wobj->obj;
      switch (track->type)
        {
        case TrackType::TRACK_TYPE_MASTER:
          gtk_label_set_markup (label, _ ("Master"));
          break;
        case TrackType::TRACK_TYPE_AUDIO_GROUP:
        case TrackType::TRACK_TYPE_MIDI_GROUP:
          gtk_label_set_markup (label, _ ("Groups"));
          break;
        case TrackType::TRACK_TYPE_INSTRUMENT:
          gtk_label_set_markup (label, _ ("Instruments"));
          break;
        default:
          g_return_if_reached ();
        }
    }
}

static char *
get_str (void * item, gpointer user_data)
{
  RouteTargetSelectorWidget * self = Z_ROUTE_TARGET_SELECTOR_WIDGET (user_data);
  if (self->track && self->track->type == TrackType::TRACK_TYPE_MASTER)
    {
      return g_strdup_printf ("<tt><i>%s</i></tt>", _ ("Engine"));
    }
  else if (GTK_IS_STRING_OBJECT (item))
    {
      return g_strdup_printf ("<tt><i>%s</i></tt>", _ ("None"));
    }
  else
    {
      WrappedObjectWithChangeSignal * wobj =
        Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (item);
      Track * track = (Track *) wobj->obj;
      return g_strdup (track->name);
    }
}

static void
on_bind (
  GtkSignalListItemFactory *  factory,
  GtkListItem *               list_item,
  RouteTargetSelectorWidget * self)
{
  GtkLabel * label = GTK_LABEL (gtk_list_item_get_child (list_item));
  GObject *  item = G_OBJECT (gtk_list_item_get_item (list_item));

  char * str = get_str (item, self);
  gtk_label_set_markup (label, str);
  g_free (str);
}

static gboolean
underlying_track_is_equal (gpointer a, gpointer b)
{
  WrappedObjectWithChangeSignal * aobj = Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (a);
  WrappedObjectWithChangeSignal * bobj = Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (b);
  return (aobj->obj == bobj->obj);
}

void
route_target_selector_widget_refresh (
  RouteTargetSelectorWidget * self,
  Track *                     track)
{
  GtkDropDown * dropdown = self->dropdown;

  /* --- disconnect existing signals --- */

  g_signal_handlers_disconnect_by_data (dropdown, self);

  /* --- remember track --- */

  self->track = track;

  /* --- set header factory --- */

  GtkListItemFactory * header_factory = gtk_signal_list_item_factory_new ();
  g_signal_connect (
    header_factory, "setup",
    G_CALLBACK (z_gtk_drop_down_list_item_header_setup_common), NULL);
  g_signal_connect (header_factory, "bind", G_CALLBACK (on_header_bind), self);
  gtk_drop_down_set_header_factory (dropdown, header_factory);
  g_object_unref (header_factory);

  /* --- set normal factory --- */

  GtkListItemFactory * factory = gtk_signal_list_item_factory_new ();
  g_signal_connect (
    factory, "setup",
    G_CALLBACK (z_gtk_drop_down_factory_setup_common_ellipsized), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (on_bind), self);
  gtk_drop_down_set_factory (dropdown, factory);
  g_object_unref (factory);

  /* --- set list factory --- */

  factory = gtk_signal_list_item_factory_new ();
  g_signal_connect (
    factory, "setup", G_CALLBACK (z_gtk_drop_down_factory_setup_common), NULL);
  g_signal_connect (factory, "bind", G_CALLBACK (on_bind), self);
  gtk_drop_down_set_list_factory (dropdown, factory);
  g_object_unref (factory);

  /* --- set closure for search FIXME makes the dropdown not use factories --- */

#if 0
  GtkExpression * expression = gtk_cclosure_expression_new (
    G_TYPE_STRING, NULL, 0, NULL, G_CALLBACK (get_str), self, NULL);
  gtk_drop_down_set_expression (dropdown, expression);
  gtk_expression_unref (expression);

  gtk_drop_down_set_enable_search (dropdown, true);
#endif

  /* --- create models --- */

  /* none */
  GtkStringList * none_sl = gtk_string_list_new (NULL);
  gtk_string_list_append (none_sl, _ ("None"));

  /* master */
  GListStore * master_ls =
    g_list_store_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);
  if (track && track->out_signal_type == ZPortType::Z_PORT_TYPE_AUDIO)
    {
      WrappedObjectWithChangeSignal * wobj =
        wrapped_object_with_change_signal_new (
          P_MASTER_TRACK, WrappedObjectType::WRAPPED_OBJECT_TYPE_TRACK);
      g_list_store_append (master_ls, wobj);
    }

  /* groups */
  GListStore * groups_ls =
    g_list_store_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);
  if (track)
    {
      for (int i = 0; i < TRACKLIST->num_tracks; i++)
        {
          Track * cur_track = TRACKLIST->tracks[i];
          if (cur_track != track && cur_track->in_signal_type == track->out_signal_type && (cur_track->type == TrackType::TRACK_TYPE_AUDIO_GROUP || cur_track->type == TrackType::TRACK_TYPE_MIDI_GROUP))
            {
              WrappedObjectWithChangeSignal * wobj =
                wrapped_object_with_change_signal_new (
                  cur_track, WrappedObjectType::WRAPPED_OBJECT_TYPE_TRACK);
              g_list_store_append (groups_ls, wobj);
            }
        }
    }

  /* instrument */
  GListStore * instruments_ls =
    g_list_store_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);
  if (track && track->out_signal_type == ZPortType::Z_PORT_TYPE_EVENT)
    {
      for (int i = 0; i < TRACKLIST->num_tracks; i++)
        {
          Track * cur_track = TRACKLIST->tracks[i];
          if (cur_track->type == TrackType::TRACK_TYPE_INSTRUMENT)
            {
              WrappedObjectWithChangeSignal * wobj =
                wrapped_object_with_change_signal_new (
                  cur_track, WrappedObjectType::WRAPPED_OBJECT_TYPE_TRACK);
              g_list_store_append (instruments_ls, wobj);
            }
        }
    }

  GListStore * composite_ls = g_list_store_new (G_TYPE_LIST_MODEL);
  g_list_store_append (composite_ls, none_sl);
  g_list_store_append (composite_ls, master_ls);
  g_list_store_append (composite_ls, groups_ls);
  g_list_store_append (composite_ls, instruments_ls);

  GtkFlattenListModel * flatten_model =
    gtk_flatten_list_model_new (G_LIST_MODEL (composite_ls));

  gtk_drop_down_set_model (dropdown, G_LIST_MODEL (flatten_model));

  /* --- preselect the current value --- */

  if (!track)
    {
      gtk_drop_down_set_selected (dropdown, 0);
    }
  else if (track && track->type == TrackType::TRACK_TYPE_MASTER)
    {
      gtk_drop_down_set_selected (dropdown, 0);
    }
  else
    {
      Track * direct_out = channel_get_output_track (track->channel);
      if (direct_out)
        {
          WrappedObjectWithChangeSignal * direct_out_wobj =
            wrapped_object_with_change_signal_new (
              direct_out, WrappedObjectType::WRAPPED_OBJECT_TYPE_TRACK);
          guint pos;
          bool  found = g_list_store_find_with_equal_func (
            master_ls, direct_out_wobj, (GEqualFunc) underlying_track_is_equal,
            &pos);
          pos += 1;
          if (!found)
            {
              found = g_list_store_find_with_equal_func (
                groups_ls, direct_out_wobj,
                (GEqualFunc) underlying_track_is_equal, &pos);
              pos += 2;
              if (!found)
                {
                  found = g_list_store_find_with_equal_func (
                    instruments_ls, direct_out_wobj,
                    (GEqualFunc) underlying_track_is_equal, &pos);
                  pos += 2 + g_list_model_get_n_items (G_LIST_MODEL (groups_ls));
                  g_return_if_fail (found);
                }
            }
          gtk_drop_down_set_selected (dropdown, pos);
        }
      else
        {
          gtk_drop_down_set_selected (dropdown, 0);
        }
    }

  /* --- add signal --- */

  g_signal_connect (
    dropdown, "notify::selected-item", G_CALLBACK (on_route_target_changed),
    self);

  /* --- set tooltips & sensitivity --- */

  gtk_widget_set_sensitive (GTK_WIDGET (dropdown), true);
  /* if unroutable */
  if (!track)
    {
      gtk_widget_set_tooltip_text (
        GTK_WIDGET (dropdown), _ ("No Routing Available"));
    }
  /* if routed by default and cannot be changed */
  else if (track && track->type == TrackType::TRACK_TYPE_MASTER)
    {
      gtk_widget_set_sensitive (GTK_WIDGET (dropdown), false);
      gtk_widget_set_tooltip_text (
        GTK_WIDGET (dropdown), _ ("Routed to Engine"));
    }
  /* if routable */
  else
    {
      Track * direct_out = channel_get_output_track (track->channel);
      if (direct_out)
        {
          char * str = g_strdup_printf (_ ("Routed to %s"), direct_out->name);
          gtk_widget_set_tooltip_text (GTK_WIDGET (dropdown), str);
          g_free (str);
        }
      else
        {
          gtk_widget_set_tooltip_text (GTK_WIDGET (dropdown), _ ("Unrouted"));
        }
    }
}

static void
finalize (RouteTargetSelectorWidget * self)
{
  G_OBJECT_CLASS (route_target_selector_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

static void
route_target_selector_widget_class_init (RouteTargetSelectorWidgetClass * _klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (wklass, "route-target-selector");

  GObjectClass * oklass = G_OBJECT_CLASS (_klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;
}

static void
route_target_selector_widget_init (RouteTargetSelectorWidget * self)
{
  self->dropdown = GTK_DROP_DOWN (gtk_drop_down_new (NULL, NULL));
  adw_bin_set_child (ADW_BIN (self), GTK_WIDGET (self->dropdown));
}
