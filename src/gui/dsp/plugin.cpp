// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "dsp/port_identifier.h"
#include "gui/backend/backend/actions/undo_manager.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings/settings.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/cached_plugin_descriptors.h"
#include "gui/backend/channel.h"
#include "gui/backend/plugin_manager.h"
#include "gui/backend/ui.h"
#include "gui/dsp/audio_port.h"
#include "gui/dsp/automatable_track.h"
#include "gui/dsp/automation_tracklist.h"
#include "gui/dsp/carla_native_plugin.h"
#include "gui/dsp/channel_track.h"
#include "gui/dsp/control_port.h"
#include "gui/dsp/cv_port.h"
#include "gui/dsp/engine.h"
#include "gui/dsp/midi_event.h"
#include "gui/dsp/midi_port.h"
#include "gui/dsp/modulator_track.h"
#include "gui/dsp/plugin.h"
#include "gui/dsp/track.h"
#include "gui/dsp/tracklist.h"
#include "gui/dsp/transport.h"
#include "utils/dsp.h"
#include "utils/exceptions.h"
#include "utils/gtest_wrapper.h"
#include "utils/io.h"
#include "utils/math.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "utils/rt_thread_id.h"
#include "utils/string.h"

#include <fmt/printf.h>

using namespace zrythm;
using namespace zrythm::gui::old_dsp::plugins;

Plugin::Plugin (PortRegistry &port_registry, const PluginSetting &setting)
    : port_registry_ (port_registry), setting_ (setting.clone_unique ())
{
  const auto &descr = setting_->descr_;

  z_debug (
    "creating plugin: {} ({})", descr->name_, ENUM_NAME (descr->protocol_));

  init ();
}

Plugin::Plugin (PortRegistry &port_registry, ZPluginCategory cat)
    : port_registry_ (port_registry)
{
  zrythm::gui::old_dsp::plugins::PluginDescriptor descr;
  descr.author_ = "Hoge";
  descr.name_ = "Dummy Plugin";
  descr.category_ = cat;
  descr.category_str_ = "Dummy Plugin Category";

  setting_ = std::make_unique<PluginSetting> (descr);

  init ();
}

Plugin::~Plugin ()
{
  if (activated_)
    {
      z_return_if_fail (!visible_);
    }
}

void
Plugin::set_stereo_outs_and_midi_in ()
{
  /* set the L/R outputs */
  auto audio_outs = get_output_port_span ().get_elements_by_type<AudioPort> ();
  int  num_audio_outs{};

  for (auto * out_port : audio_outs)
    {
      if (num_audio_outs == 0)
        {
          out_port->id_->flags_ |= PortIdentifier::Flags::StereoL;
          l_out_ = out_port;
        }
      else if (num_audio_outs == 1)
        {
          out_port->id_->flags_ |= PortIdentifier::Flags::StereoR;
          r_out_ = out_port;
        }
      num_audio_outs++;
    }

  /* if mono set it as both stereo out L and R */
  if (num_audio_outs == 1)
    {
      /* this code is only accessed by beta projects before the change to force
       * stereo a few commits after beta 1.1.11 */
      z_warning ("should not happen with carla");
      l_out_->id_->flags_ |= PortIdentifier::Flags::StereoR;
      r_out_ = l_out_;
    }

  const auto &descr = get_descriptor ();
  if (descr.num_audio_outs_ > 0)
    {
      z_return_if_fail (l_out_ && r_out_);
    }

  /* set MIDI input */
  for (auto * port : get_input_port_span ().get_elements_by_type<MidiPort> ())
    {
      if (ENUM_BITSET_TEST (
            port->id_->flags2_, PortIdentifier::Flags2::SupportsMidi))
        {
          midi_in_port_ = port;
          break;
        }
    }
  if (descr.is_instrument ())
    {
      z_return_if_fail (midi_in_port_);
    }
}

void
Plugin::set_enabled_and_gain ()
{
  z_return_if_fail (!in_ports_.empty ());

  /* set enabled/gain ports */
  for (auto * port : get_input_port_span ().get_elements_by_type<ControlPort> ())
    {
      const auto &id = port->id_;
      if (!(ENUM_BITSET_TEST (
            id->flags_, PortIdentifier::Flags::GenericPluginPort)))
        continue;

      if (ENUM_BITSET_TEST (id->flags_, PortIdentifier::Flags::PluginEnabled))
        {
          enabled_ = port;
        }
      if (ENUM_BITSET_TEST (id->flags_, PortIdentifier::Flags::PluginGain))
        {
          gain_ = port;
        }
    }
  z_return_if_fail (enabled_ && gain_);
}

bool
Plugin::is_in_active_project () const
{
  auto track_var = get_track ();
  if (!track_var)
    return false;

  return std::visit (
    [&] (auto &&track) { return track->is_in_active_project (); }, *track_var);
}

std::string
Plugin::get_full_designation_for_port (const dsp::PortIdentifier &id) const
{
  auto track = get_track ();
  z_return_val_if_fail (track, {});
  return fmt::format (
    "{}/{}/{}", TrackSpan::name_projection (*track), get_name (), id.label_);
}

void
Plugin::set_port_metadata_from_owner (dsp::PortIdentifier &id, PortRange &range)
  const
{
  id.plugin_id_ = get_uuid ();
  id.track_id_ = track_id_;
  id.owner_type_ = PortIdentifier::OwnerType::Plugin;

// FIXME: this was a horrible design decision
#if 0
  if (id.is_control ())
    {
      auto * control_port = dynamic_cast<ControlPort *> (this);
      if (control_port->at_)
        {
          control_port->at_->set_port_id (*id_);
        }
    }
#endif
}

void
Plugin::on_control_change_event (
  const dsp::PortIdentifier::PortUuid &port_uuid,
  const dsp::PortIdentifier           &id,
  float                                val)
{
  /* if plugin enabled port, also set plugin's own enabled port value and
   * vice versa */
  if (
    is_in_active_project ()
    && ENUM_BITSET_TEST (id.flags_, PortIdentifier::Flags::PluginEnabled))
    {
      if (ENUM_BITSET_TEST (id.flags_, PortIdentifier::Flags::GenericPluginPort))
        {
          if (
            own_enabled_port_
            && !utils::math::floats_equal (own_enabled_port_->control_, val))
            {
              z_debug (
                "generic enabled changed - changing plugin's own enabled");

              own_enabled_port_->set_control_value (val, false, true);
            }
        }
      else if (!utils::math::floats_equal (enabled_->control_, val))
        {
          z_debug ("plugin's own enabled changed - changing generic enabled");
          enabled_->set_control_value (val, false, true);
        }
    } /* endif plugin-enabled port */

#if HAVE_CARLA
  if (setting_->open_with_carla_ && carla_param_id_ >= 0)
    {
      auto carla =
        dynamic_cast<zrythm::gui::old_dsp::plugins::CarlaNativePlugin *> (pl);
      z_return_if_fail (carla);
      carla->set_param_value (static_cast<uint32_t> (carla_param_id_), val);
    }
#endif
  if (!state_changed_event_sent_.load (std::memory_order_acquire))
    {
      // EVENTS_PUSH (EventType::ET_PLUGIN_STATE_CHANGED, pl);
      state_changed_event_sent_.store (true, std::memory_order_release);
    }
}

bool
Plugin::should_bounce_to_master (utils::audio::BounceStep step) const
{
  // only pre-insert bounces for instrument plugins make sense
  const auto slot = get_slot ();
  if (
    !slot.has_value () || slot->has_slot_index ()
    || step != utils::audio::BounceStep::BeforeInserts)
    {
      return false;
    }

  return std::visit (
    [&] (auto &&track) { return track->bounce_to_master_; }, *get_track ());
}

bool
Plugin::is_auditioner () const
{
  return has_track ()
         && std::visit (
           [&] (auto &&track) { return track->is_auditioner (); }, *get_track ());
}

void
Plugin::init_loaded (AutomatableTrack * track)
{
  track_id_ = track->get_uuid ();

  std::vector<Port *> ports;
  append_ports (ports);
  z_return_if_fail (!ports.empty ());
  for (auto &port : ports)
    {
      port->owner_ = this;
    }

  set_enabled_and_gain ();

  if (is_in_active_project ())
    {
      bool was_enabled = this->is_enabled (false);
      try
        {
          instantiate ();
        }
      catch (const ZrythmException &err)
        {
          /* disable plugin, instantiation failed */
          instantiation_failed_ = true;
          throw ZrythmException (format_qstr (
            QObject::tr ("Instantiation failed for plugin '{}'. Disabling..."),
            get_name ()));
        }

      activate (true);
      set_enabled (was_enabled, false);
    }
}

void
Plugin::init ()
{
  z_debug ("{} ({})", get_name (), ENUM_NAME (get_protocol ()));

  {
    /* add enabled port */
    auto port_ref = port_registry_->create_object<ControlPort> (
      utils::qstring_to_std_string (QObject::tr ("Enabled")));
    auto * port = std::get<ControlPort *> (port_ref.get_object ());
    port->id_->sym_ = "enabled";
    port->id_->comment_ = utils::qstring_to_std_string (
      QObject::tr ("Enables or disables the plugin"));
    port->id_->port_group_ = "[Zrythm]";
    port->id_->flags_ |= PortIdentifier::Flags::PluginEnabled;
    port->id_->flags_ |= PortIdentifier::Flags::Toggle;
    port->id_->flags_ |= PortIdentifier::Flags::Automatable;
    port->id_->flags_ |= PortIdentifier::Flags::GenericPluginPort;
    port->range_ = { 0.f, 1.f, 0.f };
    port->deff_ = 1.f;
    port->control_ = 1.f;
    port->unsnapped_control_ = 1.f;
    port->carla_param_id_ = -1;
    add_in_port (port_ref);
    enabled_ = port;
  }

  {
    /* add gain port */
    auto port_ref = port_registry_->create_object<ControlPort> (
      utils::qstring_to_std_string (QObject::tr ("Gain")));
    auto * port = std::get<ControlPort *> (port_ref.get_object ());
    port->id_->sym_ = "gain";
    port->id_->comment_ =
      utils::qstring_to_std_string (QObject::tr ("Plugin gain"));
    port->id_->flags_ |= PortIdentifier::Flags::PluginGain;
    port->id_->flags_ |= PortIdentifier::Flags::Automatable;
    port->id_->flags_ |= PortIdentifier::Flags::GenericPluginPort;
    port->id_->port_group_ = "[Zrythm]";
    port->range_ = { 0.f, 8.f, 0.f };
    port->deff_ = 1.f;
    port->set_control_value (1.f, false, false);
    port->carla_param_id_ = -1;
    add_in_port (port_ref);
    gain_ = port;
  }

  selected_bank_.bank_idx_ = -1;
  selected_bank_.idx_ = -1;
  selected_preset_.bank_idx_ = -1;
  selected_preset_.idx_ = -1;

  set_ui_refresh_rate ();

  z_return_if_fail (gain_ && enabled_);

  /* select the init preset */
  selected_bank_.bank_idx_ = 0;
  selected_bank_.idx_ = -1;
  selected_bank_.plugin_id_ = get_uuid ();
  selected_preset_.bank_idx_ = 0;
  selected_preset_.idx_ = 0;
  selected_preset_.plugin_id_ = get_uuid ();

  if (!ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING)
    {
      /* save the new setting (may have changed during instantiation) */
      S_PLUGIN_SETTINGS->set (*setting_, true);
    }
}

Plugin::Bank *
Plugin::add_bank_if_not_exists (const std::string * uri, std::string_view name)
{
  for (auto &bank : banks_)
    {
      if (uri)
        {
          if (bank.uri_ == *uri)
            {
              return &bank;
            }
        }
      else
        {
          if (bank.name_ == name)
            {
              return &bank;
            }
        }
    }

  Bank bank;
  bank.id_.idx_ = -1;
  bank.id_.bank_idx_ = banks_.size ();
  bank.id_.plugin_id_ = get_uuid ();
  bank.name_ = name;
  if (uri)
    bank.uri_ = *uri;

  banks_.push_back (std::move (bank));
  return &banks_.back ();
}

void
Plugin::Bank::add_preset (Preset &&preset)
{
  preset.id_.idx_ = presets_.size ();
  preset.id_.bank_idx_ = id_.bank_idx_;
  preset.id_.plugin_id_ = id_.plugin_id_;

  presets_.push_back (std::move (preset));
}
void
Plugin::set_selected_bank_from_index (int idx)
{
  selected_bank_.bank_idx_ = idx;
  selected_preset_.bank_idx_ = idx;
  set_selected_preset_from_index (0);
}

void
Plugin::set_selected_preset_from_index (int idx)
{
  z_return_if_fail (instantiated_);

  selected_preset_.idx_ = idx;

  z_debug ("applying preset at index {}", idx);
  set_selected_preset_from_index_impl (idx);
}

void
Plugin::set_selected_preset_by_name (std::string_view name)
{
  z_return_if_fail (instantiated_);

  const auto &bank = banks_[selected_bank_.bank_idx_];
  for (size_t i = 0; i < bank.presets_.size (); ++i)
    {
      const auto &pset = bank.presets_[i];
      if (pset.name_ == name)
        {
          set_selected_preset_from_index (static_cast<int> (i));
          return;
        }
    }

  z_return_if_reached ();
}

void
Plugin::append_ports (std::vector<Port *> &ports)
{
  for (auto port : get_input_port_span ().as_base_type ())
    {
      ports.push_back (port);
    }
  for (auto port : get_output_port_span ().as_base_type ())
    {
      ports.push_back (port);
    }
}

std::string
Plugin::get_node_name () const
{
  auto track = get_track ();
  return fmt::format (
    "{}/{} (Plugin)",
    track ? TrackSpan::name_projection (*track) : "(No track)", get_name ());
}

void
Plugin::remove_ats_from_automation_tracklist (bool free_ats, bool fire_events)
{
  auto * track =
    TrackSpan::derived_type_transformation<AutomatableTrack> (*get_track ());
  auto &atl = track->get_automation_tracklist ();
  for (auto * at : atl.get_automation_tracks () | std::views::reverse)
    {
      auto &port = at->get_port ();
      if (port.id_->plugin_id_.has_value() && (
            port.id_->owner_type_ == PortIdentifier::OwnerType::Plugin
            || ENUM_BITSET_TEST ( port.id_->flags_,
              PortIdentifier::Flags::PluginControl)))
        {
          auto pl_var =
            PROJECT->find_plugin_by_id (port.id_->plugin_id_.value ());
          z_return_if_fail (pl_var.has_value ());
          std::visit (
            [&] (auto &&pl) {
              if (pl->get_slot () == get_slot ())
                {
                  atl.remove_at (*at, free_ats, fire_events);
                }
            },
            pl_var.value ());
        }
    }
}

bool
Plugin::validate () const
{
  if (is_in_active_project ())
    {
      /* assert instantiated and activated, or instantiation failed */
      z_return_val_if_fail (
        instantiation_failed_ || (instantiated_ && activated_), false);
    }

  return true;
}

struct PluginMoveData
{
  Plugin *                pl = nullptr;
  OptionalTrackPtrVariant track_var;
  dsp::PluginSlot         slot;
  bool                    fire_events = false;
};

#if 0
static void
plugin_move_data_free (void * _data, GClosure * closure)
{
  PluginMoveData * self = (PluginMoveData *) _data;
  object_delete_and_null (self);
}
#endif

static void
do_move (PluginMoveData * data)
{
  auto * pl = data->pl;
  auto   prev_slot = *pl->get_slot ();
  auto * prev_track = TrackSpan::derived_type_transformation<AutomatableTrack> (
    *pl->get_track ());
  auto * prev_ch = pl->get_channel ();
  z_return_if_fail (prev_ch);

  std::visit (
    [&] (auto &&data_track) {
      using TrackT = base_type<decltype (data_track)>;
      if constexpr (std::derived_from<TrackT, ChannelTrack>)
        {
          /* if existing plugin exists, delete it */
          auto existing_pl = data_track->get_plugin_at_slot (data->slot);
          if (existing_pl)
            {
              data_track->channel_->remove_plugin_from_channel (
                data->slot, false, true);
            }

          /* move plugin's automation from src to dest */
          pl->move_automation (*prev_track, *data_track, data->slot);

          /* remove plugin from its channel */
          PluginUuidReference plugin_ref{
            pl->get_uuid (), data_track->get_plugin_registry ()
          };
          auto plugin_id =
            prev_ch->remove_plugin_from_channel (prev_slot, true, false);

          /* add plugin to its new channel */
          data_track->channel_->add_plugin (
            plugin_ref, data->slot, false, true, false, true, true);

          if (data->fire_events)
            {
#if 0
      // EVENTS_PUSH (EventType::ET_CHANNEL_SLOTS_CHANGED, prev_ch);
      EVENTS_PUSH (
        EventType::ET_CHANNEL_SLOTS_CHANGED,
        data_channel_track->channel_.get ());
#endif
            }
        }
    },
    *data->track_var);
}

#if 0
static void
overwrite_plugin_response_cb (
  AdwMessageDialog * dialog,
  char *             response,
  gpointer           user_data)
{
  PluginMoveData * data = (PluginMoveData *) user_data;
  if (!string_is_equal (response, "overwrite"))
    {
      return;
    }

  do_move (data);
}
#endif

void
Plugin::move (
  AutomatableTrack * track,
  dsp::PluginSlot    slot,
  bool               confirm_overwrite,
  bool               fire_events)
{
  auto data = std::make_unique<PluginMoveData> ();
  data->pl = this;
  data->track_var = convert_to_variant<TrackPtrVariant> (track);
  data->slot = slot;
  data->fire_events = fire_events;

  std::visit (
    [&] (auto &&tr) {
      using TrackT = base_type<decltype (tr)>;
      if constexpr (std::derived_from<TrackT, AutomatableTrack>)
        {
          auto existing_pl = tr->get_plugin_at_slot (slot);
          if (existing_pl && confirm_overwrite && ZRYTHM_HAVE_UI)
            {
#if 0
      auto dialog =
        dialogs_get_overwrite_plugin_dialog (GTK_WINDOW (MAIN_WINDOW));
      gtk_window_present (GTK_WINDOW (dialog));
      g_signal_connect_data (
        dialog, "response", G_CALLBACK (overwrite_plugin_response_cb),
        data.release (), plugin_move_data_free, G_CONNECT_DEFAULT);
#endif
              return;
            }

          do_move (data.get ());
        }
    },
    *data->track_var);
}

std::optional<TrackPtrVariant>
Plugin::get_track () const
{
  if (!track_resolver_.has_value () || !track_id_.has_value ())
    return std::nullopt;
  return (*track_resolver_) (*track_id_);
}

auto
Plugin::get_slot () const -> std::optional<PluginSlot>
{
  if (!track_id_.has_value ())
    return std::nullopt;

  return std::visit (
    [&] (auto &&track) -> std::optional<PluginSlot> {
      using TrackT = base_type<decltype (track)>;
      if constexpr (
        std::derived_from<TrackT, ChannelTrack>
        || std::is_same_v<TrackT, ModulatorTrack>)
        {
          return track->get_plugin_slot (get_uuid ());
        }
      throw std::runtime_error ("Plugin::get_slot: invalid track type");
    },
    *get_track ());
}

gui::Channel *
Plugin::get_channel () const
{
  auto track =
    TrackSpan::derived_type_transformation<ChannelTrack> (*get_track ());
  auto ch = track->channel_;
  z_return_val_if_fail (ch, nullptr);

  return ch;
}

std::string
Plugin::get_full_port_group_designation (const std::string &port_group) const
{
  assert (has_track ());
  return fmt::format (
    "{}/{}/{}", TrackSpan::name_projection (*get_track ()), get_name (),
    port_group);
}

Port *
Plugin::get_port_in_group (const std::string &port_group, bool left) const
{
  auto flag =
    left ? PortIdentifier::Flags::StereoL : PortIdentifier::Flags::StereoR;
  for (auto port : get_input_port_span ().as_base_type ())
    {
      if (
        port->id_->port_group_ == port_group
        && ENUM_BITSET_TEST (port->id_->flags_, flag))
        {
          return port;
        }
    }
  for (auto port : get_output_port_span ().as_base_type ())
    {
      if (
        port->id_->port_group_ == port_group
        && ENUM_BITSET_TEST (port->id_->flags_, flag))
        {
          return port;
        }
    }

  return nullptr;
}

Port *
Plugin::get_port_in_same_group (const Port &port)
{
  if (port.id_->port_group_.empty ())
    {
      z_warning ("port {} has no port group", port.get_label ());
      return nullptr;
    }

  const auto &ports =
    port.id_->flow_ == dsp::PortFlow::Input
      ? get_input_port_span ()
      : get_output_port_span ();

  for (const auto &cur_port : ports.as_base_type ())
    {
      if (port.get_uuid () == cur_port->get_uuid ())
        {
          continue;
        }

      if (port.id_->port_group_ == cur_port->id_->port_group_ && ((ENUM_BITSET_TEST ( cur_port->id_->flags_, PortIdentifier::Flags::StereoL) && ENUM_BITSET_TEST ( port.id_->flags_, PortIdentifier::Flags::StereoR)) || (ENUM_BITSET_TEST ( cur_port->id_->flags_, PortIdentifier::Flags::StereoR) && ENUM_BITSET_TEST ( port.id_->flags_, PortIdentifier::Flags::StereoL))))
        {
          return cur_port;
        }
    }

  return nullptr;
}

std::string
Plugin::generate_window_title () const
{
  z_return_val_if_fail (is_in_active_project (), {});

  auto track_var = get_track ();

  return std::visit (
    [&] (auto &&track) -> std::string {
      const auto track_name = track ? track->get_name () : "";
      const auto plugin_name = get_name ();
      z_return_val_if_fail (!track_name.empty () && !plugin_name.empty (), {});

      std::string bridge_mode;
      if (
        setting_->bridge_mode_
        != zrythm::gui::old_dsp::plugins::CarlaBridgeMode::None)
        {
          bridge_mode =
            fmt::format (" - bridge: {}", ENUM_NAME (setting_->bridge_mode_));
        }

      const auto  slot = *get_slot ();
      std::string slot_str;
      const auto  slot_type = get_slot_type ();
      if (slot_type == zrythm::dsp::PluginSlotType::Instrument)
        {
          slot_str = "instrument";
        }
      else
        {
          const auto slot_no = slot.get_slot_with_index ().second;
          slot_str = fmt::format ("#{}", slot_no + 1);
        }

      return fmt::format (
        "{} ({} {}{}{})", plugin_name, track_name, slot,
        /* assume all plugins use carla for now */
        "",
        /*setting->open_with_carla_ ? " carla" : "",*/
        bridge_mode);
    },
    track_var.value ());
}

void
Plugin::activate (bool activate)
{
  if ((activated_ && activate) || (!activated_ && !activate))
    {
      /* nothing to do */
      z_debug ("plugin already activated/deactivated. nothing to do");
      return;
    }

  if (activate && !instantiated_)
    {
      z_error ("plugin {} not instantiated", get_name ());
      return;
    }

  if (!activate)
    {
      deactivating_ = true;
    }

  activate_impl (activate);

  activated_ = activate;
  deactivating_ = false;
}

void
Plugin::cleanup ()
{
  z_debug ("Cleaning up {}...", get_name ());

  if (!activated_ && instantiated_)
    {
      cleanup_impl ();
    }

  instantiated_ = false;
  z_debug ("done");
}

#if 0
void
Plugin::update_latency ()
{
  latency_ = get_single_playback_latency ();

  z_debug ("{} latency: {} samples", get_name (), latency_);
}
#endif

void
Plugin::set_port_index (PortUuidReference port_id)
{
  auto port_var = port_id.get_object ();
  std::visit (
    [&] (auto &&port) { port->id_->port_index_ = in_ports_.size (); }, port_var);
}

void
Plugin::add_in_port (const PortUuidReference &port_id)
{
  set_port_index (port_id);
  in_ports_.push_back (port_id);
}

void
Plugin::add_out_port (const PortUuidReference &port_id)
{
  set_port_index (port_id);
  out_ports_.push_back (port_id);
}

void
Plugin::move_automation (
  AutomatableTrack &prev_track,
  AutomatableTrack &track,
  dsp::PluginSlot   new_slot)
{
  z_debug (
    "moving plugin '{}' automation from {} to {} -> {}", get_name (),
    prev_track.get_name (), track.get_name (), new_slot);

  auto &prev_atl = prev_track.get_automation_tracklist ();
  auto &atl = track.get_automation_tracklist ();

  for (auto * at : prev_atl.get_automation_tracks ())
    {
      auto port_var = PROJECT->find_port_by_id (at->port_id_);
      if (!port_var)
        continue;

      z_return_if_fail (std::holds_alternative<ControlPort *> (*port_var));
      auto * port = std::get<ControlPort *> (*port_var);
      if (port->id_->owner_type_ == PortIdentifier::OwnerType::Plugin)
        {
          auto port_pl =
            PROJECT->find_plugin_by_id (port->id_->plugin_id_.value ());
          if (!port_pl.has_value ())
            continue;

          bool match =
            std::visit ([&] (auto &&p) { return p == this; }, port_pl.value ());
          if (!match)
            continue;
        }
      else
        continue;

      z_return_if_fail (port->at_ == at);

      /* delete from prev channel */
      auto   num_regions_before = at->get_children_vector ().size ();
      auto * removed_at = prev_atl.remove_at (*at, false, false);

      /* add to new channel */
      auto added_at = atl.add_automation_track (*removed_at);
      z_return_if_fail (
        added_at == atl.get_automation_track_at (added_at->index_)
        && added_at->get_children_vector ().size () == num_regions_before);
    }
}

void
Plugin::set_ui_refresh_rate ()
{
  // TODO ? if needed
#if 0
  if (ZRYTHM_TESTING || ZRYTHM_BENCHMARKING || ZRYTHM_GENERATING_PROJECT)
    {
      ui_update_hz_ = 30.f;
      ui_scale_factor_ = 1.f;
      goto return_refresh_rate_and_scale_factor;
    }

  /* if no preferred refresh rate is set,
   * use the monitor's refresh rate */
  if (g_settings_get_int (S_P_PLUGINS_UIS, "refresh-rate"))
    {
      /* Use user-specified UI update rate. */
      ui_update_hz_ =
        (float) g_settings_get_int (S_P_PLUGINS_UIS, "refresh-rate");
    }
  else
    {
      // ui_update_hz_ = (float) z_gtk_get_primary_monitor_refresh_rate ();
      z_debug ("refresh rate returned by GDK: %.01f", (double) ui_update_hz_);
    }

  /* if no preferred scale factor is set, use the monitor's scale factor */
  {
    float scale_factor_setting =
      (float) g_settings_get_double (S_P_PLUGINS_UIS, "scale-factor");
    if (scale_factor_setting >= 0.5f)
      {
        /* use user-specified scale factor */
        ui_scale_factor_ = scale_factor_setting;
      }
    else
      {
        /* set the scale factor */
        // ui_scale_factor_ = (float) z_gtk_get_primary_monitor_scale_factor ();
        z_debug (
          "scale factor returned by GDK: %.01f", (double) ui_scale_factor_);
      }
  }

  /* clamp the refresh rate to sensible limits */
  if (ui_update_hz_ < MIN_REFRESH_RATE || ui_update_hz_ > MAX_REFRESH_RATE)
    {
      z_warning (
        "Invalid refresh rate of %.01f received, "
        "clamping to reasonable bounds",
        (double) ui_update_hz_);
      ui_update_hz_ =
        std::clamp<float> (ui_update_hz_, MIN_REFRESH_RATE, MAX_REFRESH_RATE);
    }

  /* clamp the scale factor to sensible limits */
  if (ui_scale_factor_ < MIN_SCALE_FACTOR || ui_scale_factor_ > MAX_SCALE_FACTOR)
    {
      z_warning (
        "Invalid scale factor of %.01f received, "
        "clamping to reasonable bounds",
        (double) ui_scale_factor_);
      ui_scale_factor_ = std::clamp<float> (
        ui_scale_factor_, MIN_SCALE_FACTOR, MAX_SCALE_FACTOR);
    }

return_refresh_rate_and_scale_factor:
  z_debug ("refresh rate set to {:f}", (double) ui_update_hz_);
  z_debug ("scale factor set to {:f}", (double) ui_scale_factor_);
#endif
}

void
Plugin::generate_automation_tracks (AutomatableTrack &track)
{
  z_debug ("generating automation tracks for {}...", get_name ());

  auto &atl = track.get_automation_tracklist ();
  for (auto port : get_input_port_span ().get_elements_by_type<ControlPort> ())
    {
      if (
        port->id_->type_ != dsp::PortType::Control
        || !(ENUM_BITSET_TEST (
          port->id_->flags_, PortIdentifier::Flags::Automatable)))
        continue;

      auto * at = new AutomationTrack (
        track.get_port_registry (), track.get_object_registry (),
        [&track] () { return convert_to_variant<TrackPtrVariant> (&track); },
        port->get_uuid ());
      atl.add_automation_track (*at);
    }
}

/**
 * Gets the enable/disable port for this plugin.
 */
ControlPort *
Plugin::get_enabled_port ()
{
  for (auto port : get_input_port_span ().get_elements_by_type<ControlPort> ())
    {
      if (
        ENUM_BITSET_TEST (port->id_->flags_, PortIdentifier::Flags::PluginEnabled)
        && ENUM_BITSET_TEST (
          port->id_->flags_, PortIdentifier::Flags::GenericPluginPort))
        {
          return port;
        }
    }
  z_return_val_if_reached (nullptr);
}

void
Plugin::update_identifier ()
{
  assert (has_track ());

  /* set port identifier track poses */
  const auto new_track_uuid = get_track_id ();
  for (auto port : get_input_port_span ().as_base_type ())
    {
      port->change_track (new_track_uuid);
    }
  for (auto port : get_output_port_span ().as_base_type ())
    {
      port->change_track (new_track_uuid);
    }
}

void
Plugin::instantiate ()
{
  z_debug ("Instantiating plugin '{}'...", get_name ());

  set_enabled_and_gain ();

  set_ui_refresh_rate ();

  if (!PROJECT->loaded_)
    {
      z_return_if_fail (!state_dir_.empty ());
    }
  z_debug ("state dir: {}", state_dir_);

  instantiate_impl (!PROJECT->loaded_, !state_dir_.empty ());
  save_state (false, nullptr);

  z_return_if_fail (enabled_);
  enabled_->set_val_from_normalized (1.f, 0);

  /* set the L/R outputs */
  set_stereo_outs_and_midi_in ();

  /* update banks */
  populate_banks ();

  instantiated_ = true;
}

void
Plugin::prepare_process ()
{
  for (auto &port : audio_in_ports_)
    {
      port->clear_buffer (*AUDIO_ENGINE);
    }
  for (auto &port : cv_in_ports_)
    {
      port->clear_buffer (*AUDIO_ENGINE);
    }
  for (auto &port : midi_in_ports_)
    {
      port->clear_buffer (*AUDIO_ENGINE);
    }

  for (auto port_var : get_output_port_span ())
    {
      std::visit (
        [&] (auto &&port) { port->clear_buffer (*AUDIO_ENGINE); }, port_var);
    }
}

void
Plugin::process_block (const EngineProcessTimeInfo time_nfo)
{
  if (!is_enabled (true) && !own_enabled_port_)
    {
      process_passthrough (time_nfo);
      return;
    }

  if (!instantiated_ || !activated_)
    {
      return;
    }

  /* if has MIDI input port */
  if (get_descriptor ().num_midi_ins_ > 0)
    {
      /* if recording, write MIDI events to the region TODO */

      /* if there is a midi note in this buffer range TODO */
      /* add midi events to input port */
    }

  process_impl (time_nfo);

  /* if plugin has gain, apply it */
  if (!utils::math::floats_near (gain_->control_, 1.f, 0.001f))
    {
      for (
        auto port : get_output_port_span ().get_elements_by_type<AudioPort> ())
        {
          /* if close to 0 set it to the denormal prevention val */
          if (utils::math::floats_near (gain_->control_, 0.f, 0.00001f))
            {
              utils::float_ranges::fill (
                &port->buf_[time_nfo.local_offset_],
                DENORMAL_PREVENTION_VAL (AUDIO_ENGINE), time_nfo.nframes_);
            }
          /* otherwise just apply gain */
          else
            {
              utils::float_ranges::mul_k2 (
                &port->buf_[time_nfo.local_offset_], gain_->control_,
                time_nfo.nframes_);
            }
        }
    }
}

std::string
Plugin::print () const
{
  const auto track_name =
    is_in_active_project ()
      ? TrackSpan::name_projection (*get_track ())
      : "<no track>";
  const auto track_pos =
    is_in_active_project () ? TrackSpan::position_projection (*get_track ()) : -1;
  return fmt::format (
    "{} ({}):{} - {}", track_name, track_pos, get_slot (), get_name ());
}

void
Plugin::set_caches ()
{
  ctrl_in_ports_.clear ();
  audio_in_ports_.clear ();
  cv_in_ports_.clear ();
  midi_in_ports_.clear ();

  for (const auto &port_var : get_input_port_span ())
    {
      std::visit (
        [&] (auto &&port) {
          using PortT = base_type<decltype (port)>;
          if constexpr (std::is_same_v<PortT, ControlPort>)
            ctrl_in_ports_.push_back (port);
          else if constexpr (std::is_same_v<PortT, AudioPort>)
            audio_in_ports_.push_back (port);
          else if constexpr (std::is_same_v<PortT, CVPort>)
            cv_in_ports_.push_back (port);
          else if constexpr (std::is_same_v<PortT, MidiPort>)
            midi_in_ports_.push_back (port);
        },
        port_var);
    }
}

void
Plugin::open_ui ()
{
  z_return_if_fail (is_in_active_project ());

  z_debug ("opening plugin UI [{}]", print ());

  if (instantiation_failed_)
    {
      z_warning ("plugin {} instantiation failed, no UI to open", print ());
      return;
    }

#if 0
  /* if plugin already has a window (either generic or LV2 non-carla and
   * non-external UI) */
  if (GTK_IS_WINDOW (window_))
    {
      /* present it */
      z_debug ("presenting plugin [{}] window {}", print (), (void *) window_);
      gtk_window_present (GTK_WINDOW (window_));
    }
  else
    {
      bool generic_ui = setting_->force_generic_ui_;

      /* handle generic UIs, then carla custom, then LV2 custom */
      if (generic_ui)
        {
          z_debug ("creating and opening generic UI");
          plugin_gtk_create_window (this);
          plugin_gtk_open_generic_ui (this, true);
        }
      else
        {
          open_custom_ui (true);
        }
    }
#endif
}

#if 0
void
Plugin::select (bool select, bool exclusive)
{
  z_return_if_fail (is_in_active_project ());

  if (exclusive)
    {
      MIXER_SELECTIONS->clear (true);
    }

  auto track = get_track ();
  z_return_if_fail (track);

  if (select)
    {
      MIXER_SELECTIONS->add_slot (*track, id_.slot_, true);
    }
  else
    {
      MIXER_SELECTIONS->remove_slot (id_.slot_, true);
    }
}
#endif

void
Plugin::copy_state_dir (
  const Plugin       &src,
  bool                is_backup,
  const std::string * abs_state_dir)
{
  auto dir_to_use =
    abs_state_dir ? *abs_state_dir : get_abs_state_dir (is_backup, true);
  {
    auto files_in_dir = utils::io::get_files_in_dir (dir_to_use);
    z_return_if_fail (files_in_dir.isEmpty ());
  }

  auto src_dir_to_use = src.get_abs_state_dir (is_backup);
  z_return_if_fail (!src_dir_to_use.empty ());
  {
    auto files_in_src_dir = utils::io::get_files_in_dir (src_dir_to_use);
    z_return_if_fail (!files_in_src_dir.isEmpty ());
  }

  utils::io::copy_dir (dir_to_use, src_dir_to_use, true, true);
}

std::string
Plugin::get_abs_state_dir (const std::string &plugin_state_dir, bool is_backup)
{
  z_return_val_if_fail (!plugin_state_dir.empty (), "");
  auto parent_dir = PROJECT->get_path (ProjectPath::PluginStates, is_backup);
  return fmt::format ("{}/{}", parent_dir, plugin_state_dir);
}

std::string
Plugin::get_abs_state_dir (bool is_backup, bool create_if_not_exists)
{
  if (create_if_not_exists)
    {
      try
        {
          ensure_state_dir (is_backup);
        }
      catch (const ZrythmException &e)
        {
          e.handle ("Failed to create plugin state directory");
          return {};
        }
    }

  return get_abs_state_dir (state_dir_, is_backup);
}

void
Plugin::ensure_state_dir (bool is_backup)
{
  if (!state_dir_.empty ())
    {
      auto abs_state_dir = get_abs_state_dir (state_dir_, is_backup);
      utils::io::mkdir (abs_state_dir);
      return;
    }

  auto escaped_name = utils::io::get_legal_file_name (get_name ());
  auto parent_dir = PROJECT->get_path (ProjectPath::PluginStates, is_backup);
  z_return_if_fail (!parent_dir.empty ());
  utils::io::mkdir (parent_dir);
  auto tmp = fmt::format ("{}_XXXXXX", escaped_name);
  auto abs_state_dir_template = fs::path (parent_dir) / tmp;
  try
    {
      auto abs_state_dir =
        utils::io::make_tmp_dir_at_path (abs_state_dir_template);
      abs_state_dir->setAutoRemove (false);
      state_dir_ =
        utils::io::path_get_basename (
          utils::io::qstring_to_fs_path (abs_state_dir->path ()))
          .string ();
      z_debug ("set plugin state dir to {}", state_dir_);
    }
  catch (const ZrythmException &e)
    {
      throw ZrythmException (format_str (
        "Failed to make state dir using template {}",
        abs_state_dir_template.string ()));
    }
}

void
Plugin::copy_members_from (Plugin &other)
{
  z_debug ("[0/5] cloning plugin '{}'", other.print ());

  /* save the state of the original plugin */
  z_debug ("[1/5] saving state of source plugin (if instantiated)");
  if (other.instantiated_)
    {
      other.save_state (false, nullptr);
      z_debug ("saved source plugin state to {}", other.state_dir_);
    }

  /* create a new plugin with same descriptor */
  z_debug ("[2/5] creating new plugin with same setting");
  setting_ = other.setting_->clone_unique ();
  init ();

  /* copy ports */
  z_debug ("[3/5] copying ports from source plugin");
  enabled_ = nullptr;
  gain_ = nullptr;

  const auto clone_ports = [] (auto &ports, const auto &other_ports) {
    ports.clear ();
    ports.reserve (other_ports.size ());
    for (auto &port : other_ports)
      {
        ports.push_back (port.clone_new_identity ());
      }
  };
  clone_ports (in_ports_, other.in_ports_);
  clone_ports (out_ports_, other.out_ports_);
  set_enabled_and_gain ();

  /* copy the state directory */
  z_debug ("[4/5] copying state directory from source plugin");
  copy_state_dir (other, false, nullptr);

  z_debug ("[5/5] done");

  track_id_ = other.track_id_;
  visible_ = other.visible_;
  selected_ = other.selected_;
}

bool
Plugin::is_enabled (bool check_track) const
{
  if (!enabled_->is_toggled ())
    return false;

  if (check_track)
    {
      auto * track =
        TrackSpan::derived_type_transformation<ChannelTrack> (*get_track ());
      return track->is_enabled ();
    }
  else
    {
      return true;
    }
}

void
Plugin::set_enabled (bool enabled, bool fire_events)
{
  z_return_if_fail (instantiated_);

  enabled_->set_control_value (enabled ? 1.f : 0.f, false, fire_events);

  if (fire_events)
    {
      // EVENTS_PUSH (EventType::ET_PLUGIN_STATE_CHANGED, this);
    }
}

void
Plugin::process_passthrough (const EngineProcessTimeInfo time_nfo)
{
  size_t last_audio_idx = 0;
  size_t last_midi_idx = 0;
  auto   output_port_span = get_output_port_span ();
  for (const auto &in_port_var : get_input_port_span ())
    {
      std::visit (
        [&] (auto &&in_port) {
          using PortT = base_type<decltype (in_port)>;
          bool goto_next = false;
          if constexpr (std::is_same_v<PortT, AudioPort>)
            {
              for (size_t j = last_audio_idx; j < out_ports_.size (); j++)
                {
                  auto out_port_var = output_port_span[j];
                  if (std::holds_alternative<AudioPort *> (out_port_var))
                    {
                      /* copy */
                      auto out_port = std::get<AudioPort *> (out_port_var);
                      utils::float_ranges::copy (
                        &out_port->buf_[time_nfo.local_offset_],
                        &in_port->buf_[time_nfo.local_offset_],
                        time_nfo.nframes_);

                      last_audio_idx = j + 1;
                      goto_next = true;
                      break;
                    }
                  if (goto_next)
                    continue;
                }
            }
          else if constexpr (std::is_same_v<PortT, MidiPort>)
            {

              for (size_t j = last_midi_idx; j < out_ports_.size (); j++)
                {
                  auto out_port_var = output_port_span[j];
                  if (
                    std::holds_alternative<MidiPort *> (out_port_var)
                    && ENUM_BITSET_TEST (
                      std::get<MidiPort *> (out_port_var)->id_->flags2_,
                      PortIdentifier::Flags2::SupportsMidi))
                    {
                      auto midi_out_port = std::get<MidiPort *> (out_port_var);
                      /* copy */
                      midi_out_port->midi_events_.active_events_.append (
                        in_port->midi_events_.active_events_,
                        time_nfo.local_offset_, time_nfo.nframes_);

                      last_midi_idx = j + 1;
                      goto_next = true;
                      break;
                    }
                  if (goto_next)
                    continue;
                }
            }
        },
        in_port_var);
    }
}

void
Plugin::close_ui ()
{
  z_return_if_fail (ZRYTHM_HAVE_UI);
  z_return_if_fail (is_in_active_project ());

  if (instantiation_failed_)
    {
      z_warning ("plugin {} instantiation failed, no UI to close", get_name ());
      return;
    }

  z_return_if_fail (instantiated_);

  // plugin_gtk_close_ui (this);

  bool generic_ui = setting_->force_generic_ui_;
  if (!generic_ui)
    {
      open_custom_ui (false);
    }

  /* run events immediately otherwise freed plugin might be accessed by event
   * manager */
  // EVENT_MANAGER->process_now ();

  visible_ = false;
}

void
Plugin::connect_to_plugin (Plugin &dest)
{
  PortConnectionsManager * connections_mgr = PORT_CONNECTIONS_MGR;

  const auto num_src_audio_outs = std::ranges::distance (
    get_output_port_span ().get_elements_by_type<AudioPort> ());
  const auto num_dest_audio_ins = std::ranges::distance (
    dest.get_input_port_span ().get_elements_by_type<AudioPort> ());

  if (num_src_audio_outs == 1 && num_dest_audio_ins == 1)
    {
      for (
        auto out_port :
        get_output_port_span ().get_elements_by_type<AudioPort> ())
        {
          for (
            auto in_port :
            dest.get_input_port_span ().get_elements_by_type<AudioPort> ())
            {
              connections_mgr->ensure_connect_default (
                out_port->get_uuid (), in_port->get_uuid (), true);
              goto done1;
            }
        }
    }
  else if (num_src_audio_outs == 1 && num_dest_audio_ins > 1)
    {
      /* plugin is mono and next plugin is not mono, so connect the mono out to
       * each input */
      for (
        auto out_port :
        get_output_port_span ().get_elements_by_type<AudioPort> ())
        {
          for (
            auto in_port :
            dest.get_input_port_span ().get_elements_by_type<AudioPort> ())
            {
              connections_mgr->ensure_connect_default (
                out_port->get_uuid (), in_port->get_uuid (), true);
            }
          break;
        }
    }
  else if (num_src_audio_outs > 1 && num_dest_audio_ins == 1)
    {
      /* connect multi-output channel into mono by only connecting to the first
       * input channel found */
      for (
        auto in_port :
        dest.get_input_port_span ().get_elements_by_type<AudioPort> ())
        {
          for (
            auto out_port :
            get_output_port_span ().get_elements_by_type<AudioPort> ())
            {
              connections_mgr->ensure_connect_default (
                out_port->get_uuid (), in_port->get_uuid (), true);
              goto done1;
            }
          break;
        }
    }
  else if (num_src_audio_outs > 1 && num_dest_audio_ins > 1)
    {
      /* connect to as many audio outs this plugin has, or until we can't
       * connect anymore */
      auto num_ports_to_connect =
        std::min (num_src_audio_outs, num_dest_audio_ins);
      size_t                          last_index = 0;
      decltype (num_ports_to_connect) ports_connected{};
      for (
        auto out_port :
        get_output_port_span ().get_elements_by_type<AudioPort> ())
        {
          for (; last_index < dest.in_ports_.size (); last_index++)
            {
              auto in_port_var = dest.get_input_port_span ()[last_index];
              if (std::holds_alternative<AudioPort *> (in_port_var))
                {
                  connections_mgr->ensure_connect_default (
                    out_port->get_uuid (),
                    std::get<AudioPort *> (in_port_var)->get_uuid (), true);
                  last_index++;
                  ports_connected++;
                  break;
                }
            }
          if (ports_connected == num_ports_to_connect)
            break;
        }
    }

done1:

  /* connect prev midi outs to next midi ins */
  /* this connects only one midi out to all of the midi ins of the next plugin */
  for (auto out_port : get_output_port_span ().get_elements_by_type<MidiPort> ())
    {
      if (ENUM_BITSET_TEST (
            out_port->id_->flags2_, PortIdentifier::Flags2::SupportsMidi))
        {
          for (
            auto in_port :
            dest.get_input_port_span ().get_elements_by_type<MidiPort> ())
            {
              if (
                ENUM_BITSET_TEST (
                  in_port->id_->flags2_, PortIdentifier::Flags2::SupportsMidi))
                {
                  connections_mgr->ensure_connect_default (
                    out_port->get_uuid (), in_port->get_uuid (), true);
                }
            }
          break;
        }
    }
}

void
Plugin::connect_to_prefader (Channel &ch)
{
  z_return_if_fail (instantiated_ || instantiation_failed_);

  auto &track = ch.get_track ();
  auto  type = track.get_output_signal_type ();

  auto * mgr = PORT_CONNECTIONS_MGR; // FIXME global var
  if (type == dsp::PortType::Event)
    {
      for (
        auto out_port :
        get_output_port_span ().get_elements_by_type<MidiPort> ())
        {
          if (
            ENUM_BITSET_TEST (
              out_port->id_->flags2_, PortIdentifier::Flags2::SupportsMidi)
            && out_port->id_->flow_ == dsp::PortFlow::Output)
            {
              mgr->ensure_connect_default (
                out_port->get_uuid (), ch.midi_out_id_->id (), true);
            }
        }
    }
  else if (type == dsp::PortType::Audio)
    {
      if (l_out_ && r_out_)
        {
          mgr->ensure_connect_default (
            l_out_->get_uuid (), ch.prefader_->get_stereo_in_left_id (), true);
          mgr->ensure_connect_default (
            r_out_->get_uuid (), ch.prefader_->get_stereo_in_right_id (), true);
        }
    }
}

void
Plugin::disconnect_from_prefader (Channel &ch)
{
  auto      &track = ch.get_track ();
  const auto type = track.get_output_signal_type ();

  auto * port_connections_mgr = PORT_CONNECTIONS_MGR;
  for (const auto &out_port_var : get_output_port_span ())
    {
      std::visit (
        [&] (auto &&out_port) {
          using PortT = base_type<decltype (out_port)>;
          if (type == dsp::PortType::Audio && std::is_same_v<PortT, AudioPort>)
            {
              if (
                port_connections_mgr->are_ports_connected (
                  out_port->get_uuid (), ch.prefader_->get_stereo_in_left_id ()))
                {
                  port_connections_mgr->ensure_disconnect (
                    out_port->get_uuid (),
                    ch.prefader_->get_stereo_in_left_id ());
                }
              if (
                port_connections_mgr->are_ports_connected (
                  out_port->get_uuid (), ch.prefader_->get_stereo_in_right_id ()))
                {
                  port_connections_mgr->ensure_disconnect (
                    out_port->get_uuid (),
                    ch.prefader_->get_stereo_in_right_id ());
                }
            }
          else if (
            type == dsp::PortType::Event && std::is_same_v<PortT, MidiPort>
            && ENUM_BITSET_TEST (
              out_port->id_->flags2_, PortIdentifier::Flags2::SupportsMidi))
            {
              if (port_connections_mgr->are_ports_connected (
                    out_port->get_uuid (), ch.prefader_->get_midi_in_id ()))
                {
                  port_connections_mgr->ensure_disconnect (
                    out_port->get_uuid (), ch.prefader_->get_midi_in_id ());
                }
            }
        },
        out_port_var);
    }
}

void
Plugin::disconnect_from_plugin (Plugin &dest)
{
  auto * mgr = PORT_CONNECTIONS_MGR;

  auto num_src_audio_outs = std::ranges::distance (
    get_output_port_span ().get_elements_by_type<AudioPort> ());
  auto num_dest_audio_ins = std::ranges::distance (
    dest.get_input_port_span ().get_elements_by_type<AudioPort> ());

  if (num_src_audio_outs == 1 && num_dest_audio_ins == 1)
    {
      for (
        auto out_port :
        get_output_port_span ().get_elements_by_type<AudioPort> ())
        {
          for (
            auto in_port :
            dest.get_input_port_span ().get_elements_by_type<AudioPort> ())
            {
              mgr->ensure_disconnect (
                out_port->get_uuid (), in_port->get_uuid ());
              goto done2;
            }
        }
    }
  else if (num_src_audio_outs == 1 && num_dest_audio_ins > 1)
    {
      /* plugin is mono and next plugin is not mono, so disconnect the mono out
       * from each input */
      for (
        auto out_port :
        get_output_port_span ().get_elements_by_type<AudioPort> ())
        {
          for (
            auto in_port :
            dest.get_input_port_span ().get_elements_by_type<AudioPort> ())
            {
              mgr->ensure_disconnect (
                out_port->get_uuid (), in_port->get_uuid ());
            }
          break;
        }
    }
  else if (num_src_audio_outs > 1 && num_dest_audio_ins == 1)
    {
      /* disconnect multi-output channel from mono by disconnecting to the first
       * input channel found */
      for (
        auto in_port :
        dest.get_input_port_span ().get_elements_by_type<AudioPort> ())
        {
          for (
            auto out_port :
            get_output_port_span ().get_elements_by_type<AudioPort> ())
            {
              mgr->ensure_disconnect (
                out_port->get_uuid (), in_port->get_uuid ());
              goto done2;
            }
          break;
        }
    }
  else if (num_src_audio_outs > 1 && num_dest_audio_ins > 1)
    {
      /* connect to as many audio outs this plugin has, or until we can't
       * connect anymore */
      auto num_ports_to_connect =
        std::min (num_src_audio_outs, num_dest_audio_ins);
      size_t                          last_index = 0;
      decltype (num_ports_to_connect) ports_disconnected{};
      for (
        auto out_port :
        get_output_port_span ().get_elements_by_type<AudioPort> ())
        {
          for (; last_index < dest.in_ports_.size (); ++last_index)
            {
              auto in_port_var = dest.get_input_port_span ()[last_index];
              if (std::holds_alternative<AudioPort *> (in_port_var))
                {
                  mgr->ensure_disconnect (
                    out_port->get_uuid (),
                    std::get<AudioPort *> (in_port_var)->get_uuid ());
                  last_index++;
                  ports_disconnected++;
                  break;
                }
            }
          if ((int) ports_disconnected == num_ports_to_connect)
            break;
        }
    }

done2:

  /* disconnect MIDI connections */
  for (auto out_port : get_output_port_span ().get_elements_by_type<MidiPort> ())
    {
      if (ENUM_BITSET_TEST (
            out_port->id_->flags2_, PortIdentifier::Flags2::SupportsMidi))
        {
          for (
            auto in_port :
            dest.get_input_port_span ().get_elements_by_type<MidiPort> ())
            {
              if (
                ENUM_BITSET_TEST (
                  in_port->id_->flags2_, PortIdentifier::Flags2::SupportsMidi))
                {
                  mgr->ensure_disconnect (
                    out_port->get_uuid (), in_port->get_uuid ());
                }
            }
        }
    }
}

void
Plugin::disconnect ()
{
  z_info ("disconnecting plugin {}...", get_name ());

  deleting_ = true;

  if (is_in_active_project ())
    {
      if (visible_ && ZRYTHM_HAVE_UI)
        close_ui ();

      /* disconnect all ports */
      for (auto port : get_input_port_span ().as_base_type ())
        port->disconnect_all ();
      for (auto port : get_output_port_span ().as_base_type ())
        port->disconnect_all ();
      z_debug (
        "disconnected all ports of {} in ports: {} out ports: {}", get_name (),
        in_ports_.size (), out_ports_.size ());

      close ();
    }
  else
    {
      z_debug ("{} is not a project plugin, skipping disconnect", get_name ());

      visible_ = false;
    }

  z_debug ("finished disconnecting plugin {}", get_name ());
}

void
Plugin::delete_state_files ()
{
  z_debug ("deleting state files for plugin {} ({})", get_name (), state_dir_);

  z_return_if_fail (
    !state_dir_.empty () && std::filesystem::path (state_dir_).is_absolute ());

  try
    {
      fs::remove_all (state_dir_);
    }
  catch (const fs::filesystem_error &e)
    {
      z_warning ("error deleting state dir: {}", e.what ());
    }
}

void
Plugin::expose_ports (AudioEngine &engine, bool expose, bool inputs, bool outputs)
{
  auto set_expose = [expose, &engine] (auto &port) {
    bool is_exposed = port->is_exposed_to_backend ();
    if (expose != is_exposed)
      port->set_expose_to_backend (engine, expose);
  };

  if (inputs)
    {
      for (auto port : get_input_port_span ().as_base_type ())
        set_expose (port);
    }
  if (outputs)
    {
      for (auto port : get_output_port_span ().as_base_type ())
        set_expose (port);
    }
}

std::optional<PortPtrVariant>
Plugin::get_port_by_symbol (const std::string &sym)
{
  z_return_val_if_fail (
    get_protocol () == Protocol::ProtocolType::LV2, std::nullopt);

  auto find_port = [&sym] (const auto &ports) {
    auto it = std::ranges::find_if (ports, [&sym] (const auto &port_var) {
      return std::visit (
        [&] (auto &&port) { return port->id_->sym_ == sym; }, port_var);
    });
    return it != ports.end () ? std::make_optional (*it) : std::nullopt;
  };

  auto it = find_port (get_input_port_span ());
  if (it)
    return it;

  it = find_port (get_output_port_span ());
  if (it)
    return it;

  z_warning ("failed to find port with symbol {}", sym);
  return std::nullopt;
}
