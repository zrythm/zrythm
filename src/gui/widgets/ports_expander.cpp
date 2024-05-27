// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/engine.h"
#include "dsp/port.h"
#include "dsp/track.h"
#include "gui/backend/wrapped_object_with_change_signal.h"
#include "gui/widgets/editable_label.h"
#include "gui/widgets/inspector_port.h"
#include "gui/widgets/ports_expander.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/string.h"

#include <adwaita.h>
#include <glib/gi18n.h>

G_DEFINE_TYPE (
  PortsExpanderWidget,
  ports_expander_widget,
  TWO_COL_EXPANDER_BOX_WIDGET_TYPE)

/**
 * Refreshes each field.
 */
void
ports_expander_widget_refresh (PortsExpanderWidget * self)
{
  /* set visibility */
  /* FIXME the port counts used don't take into
   * account invisible (notOnGUI) ports */
  if (self->owner_type == PortIdentifier::OwnerType::PLUGIN)
    {
      if (self->type == ZPortType::Z_PORT_TYPE_CONTROL)
        {
          if (self->flow == ZPortFlow::Z_PORT_FLOW_INPUT)
            {
              gtk_widget_set_visible (
                GTK_WIDGET (self),
                self->plugin && self->plugin->setting->descr->num_ctrl_ins > 0);
            }
          else if (self->flow == ZPortFlow::Z_PORT_FLOW_OUTPUT)
            {
              gtk_widget_set_visible (
                GTK_WIDGET (self),
                self->plugin && self->plugin->setting->descr->num_ctrl_outs > 0);
            }
        }
      else if (self->type == ZPortType::Z_PORT_TYPE_AUDIO)
        {
          if (self->flow == ZPortFlow::Z_PORT_FLOW_INPUT)
            {
              gtk_widget_set_visible (
                GTK_WIDGET (self),
                self->plugin && self->plugin->setting->descr->num_audio_ins > 0);
            }
          else if (self->flow == ZPortFlow::Z_PORT_FLOW_OUTPUT)
            {
              gtk_widget_set_visible (
                GTK_WIDGET (self),
                self->plugin && self->plugin->setting->descr->num_audio_outs > 0);
            }
        }
      else if (self->type == ZPortType::Z_PORT_TYPE_EVENT)
        {
          if (self->flow == ZPortFlow::Z_PORT_FLOW_INPUT)
            {
              gtk_widget_set_visible (
                GTK_WIDGET (self),
                self->plugin && self->plugin->setting->descr->num_midi_ins > 0);
            }
          else if (self->flow == ZPortFlow::Z_PORT_FLOW_OUTPUT)
            {
              gtk_widget_set_visible (
                GTK_WIDGET (self),
                self->plugin && self->plugin->setting->descr->num_midi_outs > 0);
            }
        }
      else if (self->type == ZPortType::Z_PORT_TYPE_CV)
        {
          if (self->flow == ZPortFlow::Z_PORT_FLOW_INPUT)
            {
              gtk_widget_set_visible (
                GTK_WIDGET (self),
                self->plugin && self->plugin->setting->descr->num_cv_ins > 0);
            }
          else if (self->flow == ZPortFlow::Z_PORT_FLOW_OUTPUT)
            {
              gtk_widget_set_visible (
                GTK_WIDGET (self),
                self->plugin && self->plugin->setting->descr->num_cv_outs > 0);
            }
        }
    }
  else if (self->owner_type == PortIdentifier::OwnerType::TRACK)
    {
    }
}

static void
set_icon_from_port_type (PortsExpanderWidget * self, ZPortType type)
{
  if (type == ZPortType::Z_PORT_TYPE_AUDIO)
    expander_box_widget_set_icon_name (
      Z_EXPANDER_BOX_WIDGET (self), "signal-audio");
  else if (type == ZPortType::Z_PORT_TYPE_CV)
    expander_box_widget_set_icon_name (
      Z_EXPANDER_BOX_WIDGET (self), "signal-cv");
  else if (type == ZPortType::Z_PORT_TYPE_EVENT)
    expander_box_widget_set_icon_name (
      Z_EXPANDER_BOX_WIDGET (self), "audio-midi");
  else if (type == ZPortType::Z_PORT_TYPE_CONTROL)
    expander_box_widget_set_icon_name (
      Z_EXPANDER_BOX_WIDGET (self), "gnome-icon-library-wrench-wide-symbolic");
}

typedef struct PortGroup
{
  char *      group_name;
  GPtrArray * ports;
} PortGroup;

static void
free_port_group (void * data)
{
  PortGroup * self = (PortGroup *) data;
  g_free_and_null (self->group_name);
  object_free_w_func_and_null (g_ptr_array_unref, self->ports);
  object_zero_and_free (self);
}

static PortGroup *
port_group_new (const char * name)
{
  PortGroup * self = object_new (PortGroup);
  self->group_name = g_strdup (name);
  self->ports = g_ptr_array_sized_new (50);
  return self;
}

static void
item_factory_setup_cb (
  GtkSignalListItemFactory * factory,
  GtkListItem *              listitem,
  gpointer                   user_data)
{
  /*PortsExpanderWidget * self = (PortsExpanderWidget *) user_data;*/
  gtk_list_item_set_child (listitem, GTK_WIDGET (adw_bin_new ()));
}

static void
item_factory_bind_cb (
  GtkSignalListItemFactory * factory,
  GtkListItem *              listitem,
  gpointer                   user_data)
{
  WrappedObjectWithChangeSignal * obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (gtk_list_item_get_item (listitem));
  InspectorPortWidget * ip = inspector_port_widget_new ((Port *) obj->obj);
  AdwBin *              bin = ADW_BIN (gtk_list_item_get_child (listitem));
  adw_bin_set_child (bin, GTK_WIDGET (ip));
}

static void
item_factory_unbind_cb (
  GtkSignalListItemFactory * factory,
  GtkListItem *              listitem,
  gpointer                   user_data)
{
  AdwBin * bin = ADW_BIN (gtk_list_item_get_child (listitem));
  adw_bin_set_child (bin, NULL);
}

static void
item_factory_teardown_cb (
  GtkSignalListItemFactory * factory,
  GtkListItem *              listitem,
  gpointer                   user_data)
{
  gtk_list_item_set_child (listitem, NULL);
}

/**
 * Sets up the PortsExpanderWidget for a Plugin.
 */
void
ports_expander_widget_setup_plugin (
  PortsExpanderWidget * self,
  ZPortFlow             flow,
  ZPortType             type,
  Plugin *              pl)
{
  self->flow = flow;
  self->type = type;
  self->owner_type = PortIdentifier::OwnerType::PLUGIN;

  /* if same plugin, do nothing (already set up) */
  if (self->plugin == pl)
    return;

  /* set name and icon */
  char fullstr[200];
  strcpy (fullstr, "");

  if (
    type == ZPortType::Z_PORT_TYPE_CONTROL
    && flow == ZPortFlow::Z_PORT_FLOW_INPUT)
    strcpy (fullstr, _ ("Controls"));
  else if (
    type == ZPortType::Z_PORT_TYPE_CONTROL
    && flow == ZPortFlow::Z_PORT_FLOW_OUTPUT)
    strcpy (fullstr, _ ("Control Outs"));
  else if (
    type == ZPortType::Z_PORT_TYPE_AUDIO && flow == ZPortFlow::Z_PORT_FLOW_INPUT)
    strcpy (fullstr, _ ("Audio Ins"));
  else if (
    type == ZPortType::Z_PORT_TYPE_AUDIO
    && flow == ZPortFlow::Z_PORT_FLOW_OUTPUT)
    strcpy (fullstr, _ ("Audio Outs"));
  else if (
    type == ZPortType::Z_PORT_TYPE_EVENT && flow == ZPortFlow::Z_PORT_FLOW_INPUT)
    strcpy (fullstr, _ ("MIDI Ins"));
  else if (
    type == ZPortType::Z_PORT_TYPE_EVENT
    && flow == ZPortFlow::Z_PORT_FLOW_OUTPUT)
    strcpy (fullstr, _ ("MIDI Outs"));
  else if (
    type == ZPortType::Z_PORT_TYPE_CV && flow == ZPortFlow::Z_PORT_FLOW_INPUT)
    strcpy (fullstr, _ ("CV Ins"));
  else if (
    type == ZPortType::Z_PORT_TYPE_CV && flow == ZPortFlow::Z_PORT_FLOW_OUTPUT)
    strcpy (fullstr, _ ("CV Outs"));

  expander_box_widget_set_label (Z_EXPANDER_BOX_WIDGET (self), fullstr);

  set_icon_from_port_type (self, type);

  /* readd ports */
  two_col_expander_box_widget_remove_children (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self));

  /* set scrollbar options */
  two_col_expander_box_widget_set_scroll_policy (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self), GTK_POLICY_NEVER,
    GTK_POLICY_AUTOMATIC);
  two_col_expander_box_widget_set_min_max_size (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self), -1, -1, -1, 120);

  TwoColExpanderBoxWidgetPrivate * prv = two_col_expander_box_widget_get_private (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self));
  gtk_widget_set_vexpand_set (GTK_WIDGET (prv->content), 1);

  GArray * ports = g_array_new (false, true, sizeof (Port *));

  Port *           port;
  PortIdentifier * pi;
  if (
    pl && type == ZPortType::Z_PORT_TYPE_CONTROL
    && flow == ZPortFlow::Z_PORT_FLOW_INPUT)
    {
      for (int i = 0; i < pl->num_in_ports; i++)
        {
          port = pl->in_ports[i];
          pi = &port->id;
          if (
            pi->type != ZPortType::Z_PORT_TYPE_CONTROL
            || ENUM_BITSET_TEST (
              PortIdentifier::Flags, pi->flags,
              PortIdentifier::Flags::NOT_ON_GUI))
            continue;

          g_array_append_val (ports, port);
        }
    }
  else if (
    pl && type == ZPortType::Z_PORT_TYPE_CONTROL
    && flow == ZPortFlow::Z_PORT_FLOW_OUTPUT)
    {
      for (int i = 0; i < pl->num_out_ports; i++)
        {
          port = pl->out_ports[i];
          pi = &port->id;
          if (
            pi->type != ZPortType::Z_PORT_TYPE_CONTROL
            || ENUM_BITSET_TEST (
              PortIdentifier::Flags, pi->flags,
              PortIdentifier::Flags::NOT_ON_GUI))
            continue;

          g_array_append_val (ports, port);
        }
    }
  else if (
    pl && type == ZPortType::Z_PORT_TYPE_CV
    && flow == ZPortFlow::Z_PORT_FLOW_INPUT)
    {
      for (int i = 0; i < pl->num_in_ports; i++)
        {
          port = pl->in_ports[i];
          pi = &port->id;
          if (
            pi->type != ZPortType::Z_PORT_TYPE_CV
            || ENUM_BITSET_TEST (
              PortIdentifier::Flags, pi->flags,
              PortIdentifier::Flags::NOT_ON_GUI))
            continue;

          g_array_append_val (ports, port);
        }
    }
  else if (
    pl && type == ZPortType::Z_PORT_TYPE_CV
    && flow == ZPortFlow::Z_PORT_FLOW_OUTPUT)
    {
      for (int i = 0; i < pl->num_out_ports; i++)
        {
          port = pl->out_ports[i];
          pi = &port->id;
          if (
            pi->type != ZPortType::Z_PORT_TYPE_CV
            || ENUM_BITSET_TEST (
              PortIdentifier::Flags, pi->flags,
              PortIdentifier::Flags::NOT_ON_GUI))
            continue;

          g_array_append_val (ports, port);
        }
    }
  else if (
    pl && type == ZPortType::Z_PORT_TYPE_AUDIO
    && flow == ZPortFlow::Z_PORT_FLOW_INPUT)
    {
      for (int i = 0; i < pl->num_in_ports; i++)
        {
          port = pl->in_ports[i];
          pi = &port->id;
          if (
            pi->type != ZPortType::Z_PORT_TYPE_AUDIO
            || ENUM_BITSET_TEST (
              PortIdentifier::Flags, pi->flags,
              PortIdentifier::Flags::NOT_ON_GUI))
            continue;

          g_array_append_val (ports, port);
        }
    }
  else if (
    pl && type == ZPortType::Z_PORT_TYPE_AUDIO
    && flow == ZPortFlow::Z_PORT_FLOW_OUTPUT)
    {
      for (int i = 0; i < pl->num_out_ports; i++)
        {
          port = pl->out_ports[i];
          pi = &port->id;
          if (
            pi->type != ZPortType::Z_PORT_TYPE_AUDIO
            || ENUM_BITSET_TEST (
              PortIdentifier::Flags, pi->flags,
              PortIdentifier::Flags::NOT_ON_GUI))
            continue;

          g_array_append_val (ports, port);
        }
    }
  else if (
    pl && type == ZPortType::Z_PORT_TYPE_EVENT
    && flow == ZPortFlow::Z_PORT_FLOW_INPUT)
    {
      for (int i = 0; i < pl->num_in_ports; i++)
        {
          port = pl->in_ports[i];
          pi = &port->id;
          if (
            pi->type != ZPortType::Z_PORT_TYPE_EVENT
            || ENUM_BITSET_TEST (
              PortIdentifier::Flags, pi->flags,
              PortIdentifier::Flags::NOT_ON_GUI))
            continue;

          g_array_append_val (ports, port);
        }
    }
  else if (
    pl && type == ZPortType::Z_PORT_TYPE_EVENT
    && flow == ZPortFlow::Z_PORT_FLOW_OUTPUT)
    {
      for (int i = 0; i < pl->num_out_ports; i++)
        {
          port = pl->out_ports[i];
          pi = &port->id;
          if (
            pi->type != ZPortType::Z_PORT_TYPE_EVENT
            || ENUM_BITSET_TEST (
              PortIdentifier::Flags, pi->flags,
              PortIdentifier::Flags::NOT_ON_GUI))
            continue;

          g_array_append_val (ports, port);
        }
    }
  g_array_sort (ports, port_identifier_port_group_cmp);

  g_debug (
    "adding ports for plugin %s, type %s flow %s...",
    pl ? pl->setting->descr->name : "(none)", ENUM_NAME (type),
    ENUM_NAME (flow));

  /* temporary array */
  GPtrArray * port_groups = g_ptr_array_new_with_free_func (free_port_group);

  /* Add ports in group order */
  const char * last_group = NULL;
  int          num_ports = (int) ports->len;
  for (int i = 0; i < num_ports; ++i)
    {
      port = g_array_index (ports, Port *, i);
      const char * group = port->id.port_group;

      PortGroup * port_group = NULL;

      /* Check group and add new heading if necessary */
      if (!string_is_equal (group, last_group))
        {
          const char * group_name = group ? group : _ ("Ungrouped");

          port_group = port_group_new (group_name);
          g_ptr_array_add (port_groups, port_group);
        }
      last_group = group;

      if (!port_group)
        {
          if (port_groups->len > 0)
            {
              port_group = static_cast<PortGroup *> (
                g_ptr_array_index (port_groups, port_groups->len - 1));
            }
          else
            {
              port_group = port_group_new (group);
              g_ptr_array_add (port_groups, port_group);
            }
        }

      g_ptr_array_add (port_group->ports, port);
    }

  /* foreach port group */
  for (size_t i = 0; i < port_groups->len; i++)
    {
      PortGroup * pg =
        static_cast<PortGroup *> (g_ptr_array_index (port_groups, i));

      /* add the header */
      if (pg->group_name)
        {
          GtkWidget * group_label = gtk_label_new (pg->group_name);
          gtk_widget_set_name (
            GTK_WIDGET (group_label),
            "ports-expander-inspector-port-group-label");
          gtk_label_set_xalign (GTK_LABEL (group_label), 0.f);
          gtk_widget_add_css_class (group_label, "small-and-bold");
          gtk_widget_set_margin_top (group_label, 3);
          gtk_widget_set_margin_bottom (group_label, 3);
          two_col_expander_box_widget_add_single (
            Z_TWO_COL_EXPANDER_BOX_WIDGET (self), GTK_WIDGET (group_label));
        }

      /* add the list view of ports */
      GListStore * store =
        g_list_store_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);
      for (size_t j = 0; j < pg->ports->len; j++)
        {
          Port * p = static_cast<Port *> (g_ptr_array_index (pg->ports, j));
          WrappedObjectWithChangeSignal * wrapped_obj =
            wrapped_object_with_change_signal_new (
              p, WrappedObjectType::WRAPPED_OBJECT_TYPE_PORT);
          g_list_store_append (store, wrapped_obj);
        }
      GtkNoSelection * no_selection =
        gtk_no_selection_new (G_LIST_MODEL (store));
      GtkListItemFactory * list_item_factory =
        gtk_signal_list_item_factory_new ();
      g_signal_connect (
        G_OBJECT (list_item_factory), "setup",
        G_CALLBACK (item_factory_setup_cb), self);
      g_signal_connect (
        G_OBJECT (list_item_factory), "bind", G_CALLBACK (item_factory_bind_cb),
        self);
      g_signal_connect (
        G_OBJECT (list_item_factory), "unbind",
        G_CALLBACK (item_factory_unbind_cb), self);
      g_signal_connect (
        G_OBJECT (list_item_factory), "teardown",
        G_CALLBACK (item_factory_teardown_cb), self);
      GtkListView * list_view = GTK_LIST_VIEW (gtk_list_view_new (
        GTK_SELECTION_MODEL (no_selection), list_item_factory));
      if (pg->ports->len > 100)
        {
          GtkScrolledWindow * scroll =
            GTK_SCROLLED_WINDOW (gtk_scrolled_window_new ());
          gtk_scrolled_window_set_child (scroll, GTK_WIDGET (list_view));
          gtk_scrolled_window_set_placement (scroll, GTK_CORNER_BOTTOM_RIGHT);
          gtk_scrolled_window_set_policy (
            scroll, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
          gtk_scrolled_window_set_min_content_height (scroll, 500);
          two_col_expander_box_widget_add_single (
            (TwoColExpanderBoxWidget *) self, (GtkWidget *) scroll);
        }
      else
        {
          two_col_expander_box_widget_add_single (
            (TwoColExpanderBoxWidget *) self, (GtkWidget *) list_view);
        }
    }

  g_ptr_array_unref (port_groups);

  g_debug ("added ports");

  self->plugin = pl;

  ports_expander_widget_refresh (self);
}

/**
 * Sets up the PortsExpanderWidget for Track ports.
 *
 * @param type The type of ports to include.
 */
void
ports_expander_widget_setup_track (
  PortsExpanderWidget *      self,
  Track *                    tr,
  PortsExpanderTrackPortType type)
{
  self->owner_type = PortIdentifier::OwnerType::TRACK;
  self->track = tr;

  /*PortType in_type =*/
  /*self->track->in_signal_type;*/
  ZPortType out_type;
  if (tr)
    out_type = self->track->out_signal_type;
  else
    out_type = ZPortType::Z_PORT_TYPE_AUDIO;

  switch (type)
    {
    case PortsExpanderTrackPortType::PE_TRACK_PORT_TYPE_CONTROLS:
      expander_box_widget_set_label (
        Z_EXPANDER_BOX_WIDGET (self), _ ("Controls"));
      self->flow = ZPortFlow::Z_PORT_FLOW_INPUT;
      self->type = ZPortType::Z_PORT_TYPE_CONTROL;
      break;
    case PortsExpanderTrackPortType::PE_TRACK_PORT_TYPE_SENDS:
      expander_box_widget_set_label (Z_EXPANDER_BOX_WIDGET (self), _ ("Sends"));
      self->flow = ZPortFlow::Z_PORT_FLOW_OUTPUT;
      self->type = out_type;
      break;
    case PortsExpanderTrackPortType::PE_TRACK_PORT_TYPE_STEREO_IN:
      expander_box_widget_set_label (
        Z_EXPANDER_BOX_WIDGET (self), _ ("Stereo In"));
      self->flow = ZPortFlow::Z_PORT_FLOW_INPUT;
      self->owner_type = PortIdentifier::OwnerType::TRACK;
      break;
    case PortsExpanderTrackPortType::PE_TRACK_PORT_TYPE_MIDI_IN:
      expander_box_widget_set_label (
        Z_EXPANDER_BOX_WIDGET (self), _ ("MIDI In"));
      self->owner_type = PortIdentifier::OwnerType::TRACK;
      self->flow = ZPortFlow::Z_PORT_FLOW_INPUT;
      self->type = ZPortType::Z_PORT_TYPE_EVENT;
      break;
    case PortsExpanderTrackPortType::PE_TRACK_PORT_TYPE_MIDI_OUT:
      expander_box_widget_set_label (
        Z_EXPANDER_BOX_WIDGET (self), _ ("MIDI Out"));
      self->owner_type = PortIdentifier::OwnerType::TRACK;
      self->flow = ZPortFlow::Z_PORT_FLOW_OUTPUT;
      self->type = ZPortType::Z_PORT_TYPE_EVENT;
      break;
    }

  set_icon_from_port_type (self, self->type);

  /* readd ports */
  two_col_expander_box_widget_remove_children (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self));

  TwoColExpanderBoxWidgetPrivate * prv = two_col_expander_box_widget_get_private (
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self));
  gtk_widget_set_vexpand_set (GTK_WIDGET (prv->content), 1);

#define ADD_SINGLE(x) \
  ip = inspector_port_widget_new (x); \
  two_col_expander_box_widget_add_single ( \
    Z_TWO_COL_EXPANDER_BOX_WIDGET (self), GTK_WIDGET (ip))

  if (tr)
    {
      InspectorPortWidget * ip;
      switch (type)
        {
        case PortsExpanderTrackPortType::PE_TRACK_PORT_TYPE_CONTROLS:
          ADD_SINGLE (tr->channel->fader->amp);
          ADD_SINGLE (tr->channel->fader->balance);
          ADD_SINGLE (tr->channel->fader->mute);
          break;
        case PortsExpanderTrackPortType::PE_TRACK_PORT_TYPE_SENDS:
          if (out_type == ZPortType::Z_PORT_TYPE_AUDIO)
            {
              ADD_SINGLE (tr->channel->prefader->stereo_out->l);
              ADD_SINGLE (tr->channel->prefader->stereo_out->r);
              ADD_SINGLE (tr->channel->fader->stereo_out->l);
              ADD_SINGLE (tr->channel->fader->stereo_out->r);
            }
          else if (out_type == ZPortType::Z_PORT_TYPE_EVENT)
            {
              ADD_SINGLE (tr->channel->prefader->midi_out);
              ADD_SINGLE (tr->channel->fader->midi_out);
            }
          break;
          break;
        case PortsExpanderTrackPortType::PE_TRACK_PORT_TYPE_STEREO_IN:
          ADD_SINGLE (tr->processor->stereo_in->l);
          ADD_SINGLE (tr->processor->stereo_in->r);
          break;
        case PortsExpanderTrackPortType::PE_TRACK_PORT_TYPE_MIDI_IN:
          ADD_SINGLE (tr->processor->midi_in);
          break;
        case PortsExpanderTrackPortType::PE_TRACK_PORT_TYPE_MIDI_OUT:
          ADD_SINGLE (tr->channel->midi_out);
          break;
        }
    }

#undef ADD_SINGLE
}

static void
ports_expander_widget_class_init (PortsExpanderWidgetClass * klass)
{
}

static void
ports_expander_widget_init (PortsExpanderWidget * self)
{
  /*two_col_expander_box_widget_add_single (*/
  /*Z_TWO_COL_EXPANDER_BOX_WIDGET (self),*/
  /*GTK_WIDGET (self->name));*/
}
