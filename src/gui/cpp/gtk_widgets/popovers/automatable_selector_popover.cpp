// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/cpp/backend/event.h"
#include "gui/cpp/backend/event_manager.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/wrapped_object_with_change_signal.h"
#include "gui/cpp/backend/zrythm.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"
#include "gui/cpp/gtk_widgets/item_factory.h"
#include "gui/cpp/gtk_widgets/popovers/automatable_selector_popover.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

#include "common/dsp/automation_track.h"
#include "common/dsp/channel_track.h"
#include "common/dsp/engine.h"
#include "common/plugins/plugin.h"
#include "common/utils/flags.h"
#include "common/utils/resources.h"
#include "common/utils/rt_thread_id.h"
#include "common/utils/string.h"

G_DEFINE_TYPE (
  AutomatableSelectorPopoverWidget,
  automatable_selector_popover_widget,
  GTK_TYPE_POPOVER)

static void
on_closed (AutomatableSelectorPopoverWidget * self, gpointer user_data)
{
  /* if the selected automatable changed */
  auto at_port = Port::find_from_identifier<ControlPort> (self->owner->port_id_);
  if (self->selected_port && at_port != self->selected_port)
    {
      /* set the previous automation track invisible */
      auto atl = self->owner->get_automation_tracklist ();
      atl->set_at_visible (*self->owner, false);

      z_debug ("selected port: {}", self->selected_port->get_label ());

      /* swap indices */
      auto selected_at = atl->get_at_from_port (*self->selected_port);
      z_return_if_fail (selected_at);
      atl->set_at_index (*self->owner, selected_at->index_, false);

      selected_at->created_ = true;
      atl->set_at_visible (*selected_at, true);
      EVENTS_PUSH (EventType::ET_AUTOMATION_TRACK_ADDED, selected_at);
    }
  else
    {
      z_info ("same automatable selected, doing nothing");
    }

  gtk_widget_unparent (GTK_WIDGET (self));
}

/**
 * Updates the info label based on the currently
 * selected Automatable.
 */
static int
update_info_label (AutomatableSelectorPopoverWidget * self)
{
  if (self->selected_port)
    {
      Port * port = self->selected_port;

      auto label = fmt::format (
        "{}\nMin: {}\nMax: {}", port->get_label (), port->minf_, port->maxf_);

      gtk_label_set_text (self->info, label.c_str ());
    }
  else
    {
      gtk_label_set_text (self->info, _ ("No control selected"));
    }

  return G_SOURCE_REMOVE;
}

/**
 * Selects the selected automatable on the UI.
 */
static void
select_automatable (AutomatableSelectorPopoverWidget * self)
{
  update_info_label (self);
}

static GtkFilter *
get_ports_filter (AutomatableSelectorPopoverWidget * self)
{
  GtkSelectionModel * sel_model = gtk_list_view_get_model (self->port_listview);
  GtkFilterListModel * filter_model = GTK_FILTER_LIST_MODEL (
    gtk_single_selection_get_model (GTK_SINGLE_SELECTION (sel_model)));
  GtkFilter * filter = gtk_filter_list_model_get_filter (filter_model);
  return filter;
}

static void
on_search_changed (
  GtkSearchEntry *                   search_entry,
  AutomatableSelectorPopoverWidget * self)
{
  GtkFilter * filter = get_ports_filter (self);
  gtk_filter_changed (filter, GTK_FILTER_CHANGE_DIFFERENT);
}

/**
 * Visible function for ports.
 */
static gboolean
ports_filter_func (GObject * item, AutomatableSelectorPopoverWidget * self)
{
  WrappedObjectWithChangeSignal * wrapped_obj =
    Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (item);
  Port * port = wrapped_object_with_change_signal_get_port (wrapped_obj);
  const Plugin * port_pl = port->get_plugin (false);
  const Track *  port_tr = port->get_track (false);

  GtkMultiSelection * multi_sel =
    GTK_MULTI_SELECTION (gtk_list_view_get_model (self->type_listview));
  GtkFlattenListModel * flatten_model =
    GTK_FLATTEN_LIST_MODEL (gtk_multi_selection_get_model (multi_sel));
  (void) flatten_model;
  GtkBitset * bitset =
    gtk_selection_model_get_selection (GTK_SELECTION_MODEL (multi_sel));
  guint64 num_selected = gtk_bitset_get_size (bitset);
  bool    match = num_selected == 0;
  for (guint64 i = 0; i < num_selected; i++)
    {
      guint    idx = gtk_bitset_get_nth (bitset, i);
      gpointer ptr = g_list_model_get_item (G_LIST_MODEL (multi_sel), idx);
      if (Z_IS_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (ptr))
        {
          WrappedObjectWithChangeSignal * wobj =
            Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (ptr);
          Plugin * pl = wrapped_object_with_change_signal_get_plugin (wobj);
          if (pl == port_pl)
            {
              match = true;
              break;
            }
        }
      else if (GTK_IS_STRING_OBJECT (ptr))
        {
          GtkStringObject * sobj = GTK_STRING_OBJECT (ptr);
          const char *      str = gtk_string_object_get_string (sobj);
          if (string_is_equal (_ ("Tempo"), str))
            {
              if (port_tr->is_tempo ())
                {
                  match = true;
                  break;
                }
            }
          else if (string_is_equal (_ ("Macros"), str))
            {
              if (port_tr->is_modulator ())
                {
                  match = true;
                  break;
                }
            }
          else if (string_is_equal (_ ("Channel"), str))
            {
              if (
                port->id_.owner_type_ == PortIdentifier::OwnerType::Channel
                || port->id_.owner_type_ == PortIdentifier::OwnerType::Fader)
                {
                  match = true;
                  break;
                }
            }
          else if (g_str_has_prefix (str, "MIDI Ch"))
            {
              int midi_ch;
              int found_nums = sscanf (str, "MIDI Ch%d", &midi_ch);
              z_return_val_if_fail (found_nums == 1, false);
              int port_midi_ch = port->id_.get_midi_channel ();
              if (port_midi_ch == midi_ch)
                {
                  match = true;
                  break;
                }
            }
        }
    }

  if (match)
    {
      auto         str = port->get_full_designation ();
      const char * search_term =
        gtk_editable_get_text (GTK_EDITABLE (self->port_search_entry));
      char ** search_terms = g_strsplit (search_term, " ", -1);
      if (search_terms)
        {
          size_t       i = 0;
          const char * term;
          while ((term = search_terms[i++]) != nullptr)
            {
              match =
                string_contains_substr_case_insensitive (str.c_str (), term);
              if (!match)
                break;
            }
          g_strfreev (search_terms);
        }
    }

  return match;
}

static void
setup_ports_listview (
  GtkListView *                      list_view,
  AutomatableSelectorPopoverWidget * self)
{
  GListStore * store = g_list_store_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);

  auto  track = self->owner->get_track ();
  auto &atl = track->get_automation_tracklist ();
  for (auto &at : atl.ats_)
    {
      auto port = Port::find_from_identifier<ControlPort> (at->port_id_);

      if (!port)
        {
          z_warning ("no port found");
          continue;
        }

      /* if this automation track is not already in a visible lane */
      if (!at->created_ || !at->visible_ || at.get () == self->owner)
        {
          WrappedObjectWithChangeSignal * wobj =
            wrapped_object_with_change_signal_new (
              port, WrappedObjectType::WRAPPED_OBJECT_TYPE_PORT);
          g_list_store_append (store, wobj);
        }
    }

  GtkFilter *          filter = GTK_FILTER (gtk_custom_filter_new (
    (GtkCustomFilterFunc) ports_filter_func, self, nullptr));
  GtkFilterListModel * filter_model =
    gtk_filter_list_model_new (G_LIST_MODEL (store), filter);
  GtkSingleSelection * single_sel =
    gtk_single_selection_new (G_LIST_MODEL (filter_model));
  gtk_list_view_set_model (list_view, GTK_SELECTION_MODEL (single_sel));

  self->port_factory =
    std::make_unique<ItemFactory> (ItemFactory::Type::Text, false, "");
  self->port_factory->ellipsize_label_ = false;
  gtk_list_view_set_factory (list_view, self->port_factory->list_item_factory_);
}

static void
setup_type_cb (
  GtkSignalListItemFactory *         factory,
  GtkListItem *                      list_item,
  AutomatableSelectorPopoverWidget * self)
{
  GtkLabel * lbl = GTK_LABEL (gtk_label_new (""));
  gtk_list_item_set_child (list_item, GTK_WIDGET (lbl));
}

static void
bind_type_cb (
  GtkSignalListItemFactory *         factory,
  GtkListItem *                      list_item,
  AutomatableSelectorPopoverWidget * self)
{
  GObject *  obj = G_OBJECT (gtk_list_item_get_item (list_item));
  GtkLabel * lbl = GTK_LABEL (gtk_list_item_get_child (list_item));
  if (Z_IS_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (obj))
    {
      WrappedObjectWithChangeSignal * wobj =
        Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (obj);
      Plugin * pl = wrapped_object_with_change_signal_get_plugin (wobj);
      gtk_label_set_text (lbl, pl->get_name ().c_str ());
    }
  else if (GTK_IS_STRING_OBJECT (obj))
    {
      GtkStringObject * sobj = GTK_STRING_OBJECT (obj);
      gtk_label_set_text (lbl, gtk_string_object_get_string (sobj));
    }
}

static void
setup_type_header_cb (
  GtkSignalListItemFactory *         factory,
  GtkListHeader *                    list_header,
  AutomatableSelectorPopoverWidget * self)
{
  GtkLabel * lbl = GTK_LABEL (gtk_label_new (""));
  gtk_list_header_set_child (list_header, GTK_WIDGET (lbl));
}

static void
bind_type_header_cb (
  GtkSignalListItemFactory *         factory,
  GtkListHeader *                    list_header,
  AutomatableSelectorPopoverWidget * self)
{
  GObject *  obj = G_OBJECT (gtk_list_header_get_item (list_header));
  GtkLabel * lbl = GTK_LABEL (gtk_list_header_get_child (list_header));
  if (Z_IS_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (obj))
    {
      WrappedObjectWithChangeSignal * wobj =
        Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (obj);
      Plugin * pl = wrapped_object_with_change_signal_get_plugin (wobj);
      switch (pl->id_.slot_type_)
        {
        case PluginSlotType::Insert:
          gtk_label_set_text (lbl, _ ("Inserts"));
          break;
        case PluginSlotType::MidiFx:
          gtk_label_set_text (lbl, _ ("MIDI FX"));
          break;
        case PluginSlotType::Instrument:
          gtk_label_set_text (lbl, _ ("Instrument"));
          break;
        case PluginSlotType::Modulator:
          gtk_label_set_text (lbl, _ ("Modulators"));
          break;
        default:
          gtk_label_set_text (lbl, "Invalid");
          z_return_if_reached ();
          break;
        }
    }
  else if (GTK_IS_STRING_OBJECT (obj))
    {
      const char * str = gtk_string_object_get_string (GTK_STRING_OBJECT (obj));
      if (g_str_has_prefix (str, "MIDI Ch"))
        {
          gtk_label_set_text (lbl, _ ("MIDI"));
        }
      else
        {
          gtk_label_set_text (lbl, _ ("Track"));
        }
    }
}

static void
setup_types_listview (
  GtkListView *                      listview,
  AutomatableSelectorPopoverWidget * self)
{
  auto * track = self->owner->get_track ();

  GListStore * composite_ls = g_list_store_new (G_TYPE_LIST_MODEL);

  GtkStringList * basic_types_sl = gtk_string_list_new (nullptr);
  if (track->is_tempo ())
    {
      gtk_string_list_append (basic_types_sl, _ ("Tempo"));
    }
  else if (track->is_modulator ())
    {
      gtk_string_list_append (basic_types_sl, _ ("Macros"));
    }
  g_list_store_append (composite_ls, basic_types_sl);

  if (track->has_piano_roll ())
    {
      GtkStringList * sl = gtk_string_list_new (nullptr);
      for (int i = 0; i < 16; i++)
        {
          char name[300];
          sprintf (name, "MIDI Ch%d", i + 1);
          gtk_string_list_append (sl, name);
        }
      g_list_store_append (composite_ls, sl);
    }

  if (track->has_channel ())
    {
      gtk_string_list_append (basic_types_sl, _ ("Channel"));

      auto ch_track = dynamic_cast<ChannelTrack *> (track);
      auto ch = ch_track->channel_;
      if (ch_track->channel_->instrument_)
        {
          GListStore * ls =
            g_list_store_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);
          std::visit (
            [&] (auto &&derived_pl) {
              WrappedObjectWithChangeSignal * wobj =
                wrapped_object_with_change_signal_new (
                  derived_pl, WrappedObjectType::WRAPPED_OBJECT_TYPE_PLUGIN);
              g_list_store_append (ls, wobj);
            },
            convert_to_variant<PluginPtrVariant> (ch->instrument_.get ()));
          g_list_store_append (composite_ls, ls);
        }

      GListStore * midi_fx_ls =
        g_list_store_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);
      GListStore * inserts_ls =
        g_list_store_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);
      for (auto &plugin : ch->midi_fx_)
        {
          if (plugin)
            {
              std::visit (
                [&] (auto &&derived_pl) {
                  WrappedObjectWithChangeSignal * wobj =
                    wrapped_object_with_change_signal_new (
                      derived_pl, WrappedObjectType::WRAPPED_OBJECT_TYPE_PLUGIN);
                  g_list_store_append (midi_fx_ls, wobj);
                },
                convert_to_variant<PluginPtrVariant> (plugin.get ()));
            }
        }
      for (auto &plugin : ch->inserts_)
        {
          if (plugin)
            {
              std::visit (
                [&] (auto &&derived_pl) {
                  WrappedObjectWithChangeSignal * wobj =
                    wrapped_object_with_change_signal_new (
                      derived_pl, WrappedObjectType::WRAPPED_OBJECT_TYPE_PLUGIN);
                  g_list_store_append (inserts_ls, wobj);
                },
                convert_to_variant<PluginPtrVariant> (plugin.get ()));
            }
        }
      g_list_store_append (composite_ls, midi_fx_ls);
      g_list_store_append (composite_ls, inserts_ls);
    }
  GListStore * modulators_ls =
    g_list_store_new (WRAPPED_OBJECT_WITH_CHANGE_SIGNAL_TYPE);
  if (auto mod_track = dynamic_cast<ModulatorTrack *> (track))
    {
      for (auto &plugin : mod_track->modulators_)
        {
          if (plugin)
            {
              std::visit (
                [&] (auto &&derived_pl) {
                  WrappedObjectWithChangeSignal * wobj =
                    wrapped_object_with_change_signal_new (
                      derived_pl, WrappedObjectType::WRAPPED_OBJECT_TYPE_PLUGIN);
                  g_list_store_append (modulators_ls, wobj);
                },
                convert_to_variant<PluginPtrVariant> (plugin.get ()));
            }
        }
    }
  g_list_store_append (composite_ls, modulators_ls);

  GtkFlattenListModel * flatten_model =
    gtk_flatten_list_model_new (G_LIST_MODEL (composite_ls));
  GtkMultiSelection * multi_sel =
    gtk_multi_selection_new (G_LIST_MODEL (flatten_model));
  gtk_list_view_set_model (listview, GTK_SELECTION_MODEL (multi_sel));
  GtkListItemFactory * factory = gtk_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", G_CALLBACK (setup_type_cb), self);
  g_signal_connect (factory, "bind", G_CALLBACK (bind_type_cb), self);
  gtk_list_view_set_factory (listview, factory);
  GtkListItemFactory * header_factory = gtk_signal_list_item_factory_new ();
  g_signal_connect (
    header_factory, "setup", G_CALLBACK (setup_type_header_cb), self);
  g_signal_connect (
    header_factory, "bind", G_CALLBACK (bind_type_header_cb), self);
  gtk_list_view_set_header_factory (listview, header_factory);
}

static void
on_port_selection_changed (
  GtkSelectionModel *                selection_model,
  GParamSpec *                       pspec,
  AutomatableSelectorPopoverWidget * self)
{
  WrappedObjectWithChangeSignal * wobj = Z_WRAPPED_OBJECT_WITH_CHANGE_SIGNAL (
    gtk_single_selection_get_selected_item (
      GTK_SINGLE_SELECTION (selection_model)));
  if (wobj)
    {
      self->selected_port = std::get<ControlPort *> (wobj->obj);
      update_info_label (self);
    }
}

static void
on_type_selection_changed (
  GtkSelectionModel *                selection_model,
  guint                              position,
  guint                              n_items,
  AutomatableSelectorPopoverWidget * self)
{
  GtkFilter * filter = get_ports_filter (self);
  gtk_filter_changed (filter, GTK_FILTER_CHANGE_DIFFERENT);
}

/**
 * Creates a digital meter with the given type (bpm or position).
 */
AutomatableSelectorPopoverWidget *
automatable_selector_popover_widget_new (AutomationTrack * owner)
{
  AutomatableSelectorPopoverWidget * self =
    Z_AUTOMATABLE_SELECTOR_POPOVER_WIDGET (
      g_object_new (AUTOMATABLE_SELECTOR_POPOVER_WIDGET_TYPE, nullptr));

  self->owner = owner;

  /* set selected automatable */
  auto * port = Port::find_from_identifier<ControlPort> (owner->port_id_);
  z_return_val_if_fail (port, nullptr);
  self->selected_port = port;

  /* create model/treeview for types */
  setup_types_listview (self->type_listview, self);

  /* create model/treeview for ports */
  setup_ports_listview (self->port_listview, self);

  /* select the automatable */
  select_automatable (self);

  g_signal_connect (
    G_OBJECT (gtk_list_view_get_model (self->type_listview)),
    "selection-changed", G_CALLBACK (on_type_selection_changed), self);
  g_signal_connect (
    G_OBJECT (gtk_list_view_get_model (self->port_listview)),
    "notify::selected-item", G_CALLBACK (on_port_selection_changed), self);

  return self;
}

static void
automatable_selector_popover_widget_finalize (GObject * object)
{
  AutomatableSelectorPopoverWidget * self =
    Z_AUTOMATABLE_SELECTOR_POPOVER_WIDGET (object);

  std::destroy_at (&self->port_factory);

  G_OBJECT_CLASS (automatable_selector_popover_widget_parent_class)
    ->finalize (object);
}

static void
automatable_selector_popover_widget_class_init (
  AutomatableSelectorPopoverWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "automatable_selector.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child ( \
    klass, AutomatableSelectorPopoverWidget, x)

  BIND_CHILD (type_listview);
  BIND_CHILD (port_search_entry);
  BIND_CHILD (port_listview);
  BIND_CHILD (info);
  gtk_widget_class_bind_template_callback (klass, on_closed);

#undef BIND_CHILD

  GObjectClass * gobject_class = G_OBJECT_CLASS (_klass);
  gobject_class->finalize = automatable_selector_popover_widget_finalize;
}

static void
automatable_selector_popover_widget_init (
  AutomatableSelectorPopoverWidget * self)
{
  std::construct_at (&self->port_factory);

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_search_entry_set_key_capture_widget (
    self->port_search_entry, GTK_WIDGET (self->port_listview));
  g_signal_connect (
    G_OBJECT (self->port_search_entry), "search-changed",
    G_CALLBACK (on_search_changed), self);
}
