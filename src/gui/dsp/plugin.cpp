// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "dsp/midi_event.h"
#include "dsp/parameter.h"
#include "dsp/port_all.h"
#include "engine/device_io/engine.h"
#include "engine/session/transport.h"
#include "gui/backend/backend/actions/undo_manager.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings/settings.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/backend/plugin_manager.h"
#include "gui/backend/ui.h"
#include "gui/dsp/carla_native_plugin.h"
#include "gui/dsp/plugin.h"
#include "structure/tracks/automatable_track.h"
#include "structure/tracks/automation_tracklist.h"
#include "structure/tracks/channel.h"
#include "structure/tracks/channel_track.h"
#include "structure/tracks/modulator_track.h"
#include "structure/tracks/track.h"
#include "structure/tracks/tracklist.h"
#include "utils/dsp.h"
#include "utils/exceptions.h"
#include "utils/gtest_wrapper.h"
#include "utils/io.h"
#include "utils/math.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "utils/rt_thread_id.h"
#include "utils/string.h"

namespace zrythm::gui::old_dsp::plugins
{

Plugin::Plugin (
  dsp::PortRegistry               &port_registry,
  dsp::ProcessorParameterRegistry &param_registry,
  QObject                         &derived)
    : port_registry_ (port_registry), param_registry_ (param_registry),
      enabled_ (param_registry), gain_ (param_registry)
{
  const auto uuid_str = utils::Utf8String::from_qstring (
    type_safe::get (get_uuid ()).toString (QUuid::WithoutBraces));
  {
    /* add enabled port */
    enabled_ = param_registry_->create_object<dsp::ProcessorParameter> (
      port_registry, dsp::ProcessorParameter::UniqueId (uuid_str + u8"/enabled"),
      dsp::ParameterRange{
        dsp::ParameterRange::Type::Toggle, 0.f, 1.f, 0.f, 1.f },
      utils::Utf8String::from_qstring (QObject::tr ("Enabled")), &derived);
    get_enabled_param ()->set_description (
      utils::Utf8String::from_qstring (
        QObject::tr ("Enables or disables the plugin")));
  }

  {
    /* add gain port */
    gain_ = param_registry_->create_object<dsp::ProcessorParameter> (
      port_registry, dsp::ProcessorParameter::UniqueId (uuid_str + u8"/gain"),
      dsp::ParameterRange{
        dsp::ParameterRange::Type::GainAmplitude, 0.f, 8.f, 0.f, 1.f },
      utils::Utf8String::from_qstring (QObject::tr ("Gain")), &derived);
  }

  selected_bank_.bank_idx_ = -1;
  selected_bank_.idx_ = -1;
  selected_preset_.bank_idx_ = -1;
  selected_preset_.idx_ = -1;

  set_ui_refresh_rate ();

  /* select the init preset */
  selected_bank_.bank_idx_ = 0;
  selected_bank_.idx_ = -1;
  selected_bank_.plugin_id_ = get_uuid ();
  selected_preset_.bank_idx_ = 0;
  selected_preset_.idx_ = 0;
  selected_preset_.plugin_id_ = get_uuid ();
}

Plugin::~Plugin ()
{
  if (activated_)
    {
      z_return_if_fail (!visible_);
    }
}

void
Plugin::set_setting (const PluginConfiguration &setting)
{
  setting_ = utils::clone_unique (setting);
  const auto &descr = setting_->descr_;

  z_debug (
    "setting setting for plugin: {} ({})", descr->name_,
    ENUM_NAME (descr->protocol_));

  z_debug ("{} ({})", get_name (), ENUM_NAME (get_protocol ()));

  if (!ZRYTHM_TESTING && !ZRYTHM_BENCHMARKING)
    {
      /* save the new setting (may have changed during instantiation) */
      S_PLUGIN_SETTINGS->set (*setting_, true);
    }
}

void
Plugin::set_stereo_outs_and_midi_in ()
{
  /* set the L/R outputs */
  auto audio_outs =
    get_output_port_span ().get_elements_by_type<dsp::AudioPort> ();
  int num_audio_outs{};

  for (auto * out_port : audio_outs)
    {
      if (num_audio_outs == 0)
        {
          out_port->mark_as_stereo ();
          l_out_ = out_port;
        }
      else if (num_audio_outs == 1)
        {
          out_port->mark_as_stereo ();
          r_out_ = out_port;
        }
      num_audio_outs++;
    }

  const auto &descr = get_descriptor ();
  if (descr.num_audio_outs_ > 0)
    {
      z_return_if_fail (l_out_ && r_out_);
    }

  /* set MIDI input */
  for (
    auto * port : get_input_port_span ().get_elements_by_type<dsp::MidiPort> ())
    {
      midi_in_port_ = port;
      break;
    }
  if (descr.is_instrument ())
    {
      z_return_if_fail (midi_in_port_);
    }
}

utils::Utf8String
Plugin::get_full_designation_for_port (const dsp::Port &port) const
{
  auto track = get_track ();
  z_return_val_if_fail (track, {});
  return utils::Utf8String::from_utf8_encoded_string (
    fmt::format (
      "{}/{}/{}", structure::tracks::TrackSpan::name_projection (*track),
      get_name (), port.get_label ()));
}

#if 0
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
#endif

bool
Plugin::is_auditioner () const
{
  return has_track ()
         && std::visit (
           [&] (auto &&track) { return track->is_auditioner (); }, *get_track ());
}

void
Plugin::init_loaded ()
{
  std::vector<dsp::Port *> ports;
  append_ports (ports);
  z_return_if_fail (!ports.empty ());
  for (auto &port : ports)
    {
      port->set_full_designation_provider (this);
    }

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

Plugin::Bank *
Plugin::add_bank_if_not_exists (
  std::optional<utils::Utf8String> uri,
  const utils::Utf8String         &name)
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
Plugin::set_selected_preset_by_name (const utils::Utf8String &name)
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
Plugin::append_ports (std::vector<dsp::Port *> &ports)
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

utils::Utf8String
Plugin::get_node_name () const
{
  auto track = get_track ();
  return utils::Utf8String::from_utf8_encoded_string (
    fmt::format (
      "{}/{} (Plugin)",
      track ? structure::tracks::TrackSpan::name_projection (*track)
            : u8"(No track)",
      get_name ()));
}

void
Plugin::remove_ats_from_automation_tracklist (bool free_ats, bool fire_events)
{
// TODO
#if 0
  auto * track = structure::tracks::TrackSpan::derived_type_transformation<
    structure::tracks::AutomatableTrack> (*get_track ());
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
#endif
}

auto
Plugin::get_track () const -> std::optional<TrackPtrVariant>
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
        std::derived_from<TrackT, structure::tracks::ChannelTrack>
        || std::is_same_v<TrackT, structure::tracks::ModulatorTrack>)
        {
          return track->get_plugin_slot (get_uuid ());
        }
      throw std::runtime_error ("Plugin::get_slot: invalid track type");
    },
    *get_track ());
}

utils::Utf8String
Plugin::get_full_port_group_designation (
  const utils::Utf8String &port_group) const
{
  assert (has_track ());
  return utils::Utf8String::from_utf8_encoded_string (
    fmt::format (
      "{}/{}/{}", structure::tracks::TrackSpan::name_projection (*get_track ()),
      get_name (), port_group));
}

#if 0
Port *
Plugin::get_port_in_group (const utils::Utf8String &port_group, bool left) const
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
#endif

dsp::Port *
Plugin::get_port_in_same_group (const dsp::Port &port)
{
// TODO
#if 0
  if (!port->port_group_)
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
#endif
  return nullptr;
}

utils::Utf8String
Plugin::generate_window_title () const
{
  auto track_var = get_track ();

  return std::visit (
    [&] (auto &&track) -> utils::Utf8String {
      const auto track_name = track ? track->get_name () : u8"";
      const auto plugin_name = get_name ();
      z_return_val_if_fail (!track_name.empty () && !plugin_name.empty (), {});

      std::string bridge_mode;
      if (setting_->bridge_mode_ != zrythm::plugins::BridgeMode::None)
        {
          bridge_mode =
            fmt::format (" - bridge: {}", ENUM_NAME (setting_->bridge_mode_));
        }

      const auto  slot = *get_slot ();
      std::string slot_str;
      const auto  slot_type = get_slot_type ();
      if (slot_type == zrythm::plugins::PluginSlotType::Instrument)
        {
          slot_str = "instrument";
        }
      else
        {
          const auto slot_no = slot.get_slot_with_index ().second;
          slot_str = fmt::format ("#{}", slot_no + 1);
        }

      return utils::Utf8String::from_utf8_encoded_string (
        fmt::format (
          "{} ({} {}{}{})", plugin_name, track_name, slot,
          /* assume all plugins use carla for now */
          "",
          /*setting->open_with_carla_ ? " carla" : "",*/
          bridge_mode));
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
      z_debug ("refresh rate returned by GDK: {}", ui_update_hz_);
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
          "scale factor returned by GDK: {}",  ui_scale_factor_);
      }
  }

  /* clamp the refresh rate to sensible limits */
  if (ui_update_hz_ < MIN_REFRESH_RATE || ui_update_hz_ > MAX_REFRESH_RATE)
    {
      z_warning (
        "Invalid refresh rate of {} received, "
        "clamping to reasonable bounds",
        ui_update_hz_);
      ui_update_hz_ =
        std::clamp<float> (ui_update_hz_, MIN_REFRESH_RATE, MAX_REFRESH_RATE);
    }

  /* clamp the scale factor to sensible limits */
  if (ui_scale_factor_ < MIN_SCALE_FACTOR || ui_scale_factor_ > MAX_SCALE_FACTOR)
    {
      z_warning (
        "Invalid scale factor of {} received, "
        "clamping to reasonable bounds",
        ui_scale_factor_);
      ui_scale_factor_ = std::clamp<float> (
        ui_scale_factor_, MIN_SCALE_FACTOR, MAX_SCALE_FACTOR);
    }

return_refresh_rate_and_scale_factor:
  z_debug ("refresh rate set to {:f}", (double) ui_update_hz_);
  z_debug ("scale factor set to {:f}", (double) ui_scale_factor_);
#endif
}

dsp::ProcessorParameter *
Plugin::get_enabled_param () const
{
  return enabled_.get_object_as<dsp::ProcessorParameter> ();
}

void
Plugin::instantiate ()
{
  z_debug ("Instantiating plugin '{}'...", get_name ());

  set_ui_refresh_rate ();

  if (!PROJECT->loaded_)
    {
      z_return_if_fail (!state_dir_.empty ());
    }
  z_debug ("state dir: {}", state_dir_);

  instantiate_impl (!PROJECT->loaded_, !state_dir_.empty ());
  save_state (false, std::nullopt);

  get_enabled_param ()->setBaseValue (1.f);

  /* set the L/R outputs */
  set_stereo_outs_and_midi_in ();

  /* update banks */
  populate_banks ();

  instantiated_ = true;
}

void
Plugin::prepare_process (std::size_t block_length)
{
  for (auto &port : audio_in_ports_)
    {
      port->clear_buffer (block_length);
    }
  for (auto &port : cv_in_ports_)
    {
      port->clear_buffer (block_length);
    }
  for (auto &port : midi_in_ports_)
    {
      port->clear_buffer (block_length);
    }

  for (auto port_var : get_output_port_span ())
    {
      std::visit (
        [&] (auto &&port) { port->clear_buffer (block_length); }, port_var);
    }
}

void
Plugin::process_block (const EngineProcessTimeInfo time_nfo)
{
  if (!is_enabled (true))
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
  const auto gain_param = gain_.get_object_as<dsp::ProcessorParameter> ();
  const auto gain =
    gain_param->range ().convert_from_0_to_1 (gain_param->currentValue ());
  if (!utils::math::floats_near (gain, 1.f, 0.001f))
    {
      for (
        auto port :
        get_output_port_span ().get_elements_by_type<dsp::AudioPort> ())
        {
          /* if close to 0 set it to the denormal prevention val */
          if (utils::math::floats_near (gain, 0.f, 0.00001f))
            {
              utils::float_ranges::fill (
                &port->buf_[time_nfo.local_offset_],
                DENORMAL_PREVENTION_VAL (AUDIO_ENGINE), time_nfo.nframes_);
            }
          /* otherwise just apply gain */
          else
            {
              utils::float_ranges::mul_k2 (
                &port->buf_[time_nfo.local_offset_], gain, time_nfo.nframes_);
            }
        }
    }
}

void
Plugin::set_caches ()
{
  audio_in_ports_.clear ();
  cv_in_ports_.clear ();
  midi_in_ports_.clear ();

  for (const auto &port_var : get_input_port_span ())
    {
      std::visit (
        [&] (auto &&port) {
          using PortT = base_type<decltype (port)>;
          if constexpr (std::is_same_v<PortT, dsp::AudioPort>)
            audio_in_ports_.push_back (port);
          else if constexpr (std::is_same_v<PortT, dsp::CVPort>)
            cv_in_ports_.push_back (port);
          else if constexpr (std::is_same_v<PortT, dsp::MidiPort>)
            midi_in_ports_.push_back (port);
        },
        port_var);
    }
}

void
Plugin::open_ui ()
{
  z_debug ("opening plugin UI [{}]", get_name ());

  if (instantiation_failed_)
    {
      z_warning ("plugin {} instantiation failed, no UI to open", get_name ());
      return;
    }

#if 0
  /* if plugin already has a window (either generic or LV2 non-carla and
   * non-external UI) */
  if (GTK_IS_WINDOW (window_))
    {
      /* present it */
      z_debug ("presenting plugin [{}] window {}", get_name (), (void *) window_);
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
  const Plugin           &src,
  bool                    is_backup,
  std::optional<fs::path> abs_state_dir)
{
  auto dir_to_use =
    abs_state_dir ? *abs_state_dir : get_abs_state_dir (is_backup, true);
  {
    auto files_in_dir = utils::io::get_files_in_dir (dir_to_use);
    z_return_if_fail (files_in_dir.empty ());
  }

  auto src_dir_to_use = src.get_abs_state_dir (is_backup);
  z_return_if_fail (!src_dir_to_use.empty ());
  {
    auto files_in_src_dir = utils::io::get_files_in_dir (src_dir_to_use);
    z_return_if_fail (!files_in_src_dir.empty ());
  }

  utils::io::copy_dir (dir_to_use, src_dir_to_use, true, true);
}

fs::path
Plugin::get_abs_state_dir (const fs::path &plugin_state_dir, bool is_backup)
{
  z_return_val_if_fail (!plugin_state_dir.empty (), "");
  auto parent_dir = PROJECT->get_path (ProjectPath::PluginStates, is_backup);
  return fmt::format ("{}/{}", parent_dir, plugin_state_dir);
}

fs::path
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
      state_dir_ = utils::io::path_get_basename (
        utils::Utf8String::from_qstring (abs_state_dir->path ()));
      z_debug ("set plugin state dir to {}", state_dir_);
    }
  catch (const ZrythmException &e)
    {
      throw ZrythmException (format_str (
        "Failed to make state dir using template {}", abs_state_dir_template));
    }
}

void
Plugin::copy_members_from (Plugin &other)
{
  z_debug ("[0/5] cloning plugin '{}'", other.get_name ());

  /* save the state of the original plugin */
  z_debug ("[1/5] saving state of source plugin (if instantiated)");
  if (other.instantiated_)
    {
      other.save_state (false, std::nullopt);
      z_debug ("saved source plugin state to {}", other.state_dir_);
    }

  /* create a new plugin with same descriptor */
  z_debug ("[2/5] creating new plugin with same setting");
  setting_ = utils::clone_unique (*other.setting_);

  /* copy ports */
  z_debug ("[3/5] copying ports from source plugin");

  const auto clone_ports = [] (auto &ports, const auto &other_ports) {
// TODO
#if 0
    ports.clear ();
    ports.reserve (other_ports.size ());
    for (auto &port : other_ports)
      {
        ports.push_back (port.clone_new_identity ());
      }
#endif
  };
  clone_ports (in_ports_, other.in_ports_);
  clone_ports (out_ports_, other.out_ports_);

  /* copy the state directory */
  z_debug ("[4/5] copying state directory from source plugin");
  copy_state_dir (other, false, std::nullopt);

  z_debug ("[5/5] done");

  track_id_ = other.track_id_;
  visible_ = other.visible_;
  selected_ = other.selected_;
}

bool
Plugin::is_enabled (bool check_track) const
{
  if (!get_enabled_param ()->range ().is_toggled (
        get_enabled_param ()->currentValue ()))
    return false;

  if (check_track)
    {
      auto * track = structure::tracks::TrackSpan::derived_type_transformation<
        structure::tracks::ChannelTrack> (*get_track ());
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

  get_enabled_param ()->setBaseValue (enabled ? 1.f : 0.f);

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
          if constexpr (std::is_same_v<PortT, dsp::AudioPort>)
            {
              for (size_t j = last_audio_idx; j < out_ports_.size (); j++)
                {
                  auto out_port_var = output_port_span[j];
                  if (std::holds_alternative<dsp::AudioPort *> (out_port_var))
                    {
                      /* copy */
                      auto out_port = std::get<dsp::AudioPort *> (out_port_var);
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
          else if constexpr (std::is_same_v<PortT, dsp::MidiPort>)
            {

              for (size_t j = last_midi_idx; j < out_ports_.size (); j++)
                {
                  auto out_port_var = output_port_span[j];
                  if (std::holds_alternative<dsp::MidiPort *> (out_port_var))
                    {
                      auto midi_out_port =
                        std::get<dsp::MidiPort *> (out_port_var);
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
  auto * connections_mgr = PORT_CONNECTIONS_MGR;

  const auto num_src_audio_outs = std::ranges::distance (
    get_output_port_span ().get_elements_by_type<dsp::AudioPort> ());
  const auto num_dest_audio_ins = std::ranges::distance (
    dest.get_input_port_span ().get_elements_by_type<dsp::AudioPort> ());

  if (num_src_audio_outs == 1 && num_dest_audio_ins == 1)
    {
      for (
        auto out_port :
        get_output_port_span ().get_elements_by_type<dsp::AudioPort> ())
        {
          for (
            auto in_port :
            dest.get_input_port_span ().get_elements_by_type<dsp::AudioPort> ())
            {
              connections_mgr->add_default_connection (
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
        get_output_port_span ().get_elements_by_type<dsp::AudioPort> ())
        {
          for (
            auto in_port :
            dest.get_input_port_span ().get_elements_by_type<dsp::AudioPort> ())
            {
              connections_mgr->add_default_connection (
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
        dest.get_input_port_span ().get_elements_by_type<dsp::AudioPort> ())
        {
          for (
            auto out_port :
            get_output_port_span ().get_elements_by_type<dsp::AudioPort> ())
            {
              connections_mgr->add_default_connection (
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
        get_output_port_span ().get_elements_by_type<dsp::AudioPort> ())
        {
          for (; last_index < dest.in_ports_.size (); last_index++)
            {
              auto in_port_var = dest.get_input_port_span ()[last_index];
              if (std::holds_alternative<dsp::AudioPort *> (in_port_var))
                {
                  connections_mgr->add_default_connection (
                    out_port->get_uuid (),
                    std::get<dsp::AudioPort *> (in_port_var)->get_uuid (), true);
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
  for (
    auto out_port :
    get_output_port_span ().get_elements_by_type<dsp::MidiPort> ())
    {

      for (
        auto in_port :
        dest.get_input_port_span ().get_elements_by_type<dsp::MidiPort> ())
        {
          connections_mgr->add_default_connection (
            out_port->get_uuid (), in_port->get_uuid (), true);
        }
      break;
    }
}

void
Plugin::disconnect_from_plugin (Plugin &dest)
{
  auto * mgr = PORT_CONNECTIONS_MGR;

  auto num_src_audio_outs = std::ranges::distance (
    get_output_port_span ().get_elements_by_type<dsp::AudioPort> ());
  auto num_dest_audio_ins = std::ranges::distance (
    dest.get_input_port_span ().get_elements_by_type<dsp::AudioPort> ());

  if (num_src_audio_outs == 1 && num_dest_audio_ins == 1)
    {
      for (
        auto out_port :
        get_output_port_span ().get_elements_by_type<dsp::AudioPort> ())
        {
          for (
            auto in_port :
            dest.get_input_port_span ().get_elements_by_type<dsp::AudioPort> ())
            {
              mgr->remove_connection (
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
        get_output_port_span ().get_elements_by_type<dsp::AudioPort> ())
        {
          for (
            auto in_port :
            dest.get_input_port_span ().get_elements_by_type<dsp::AudioPort> ())
            {
              mgr->remove_connection (
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
        dest.get_input_port_span ().get_elements_by_type<dsp::AudioPort> ())
        {
          for (
            auto out_port :
            get_output_port_span ().get_elements_by_type<dsp::AudioPort> ())
            {
              mgr->remove_connection (
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
        get_output_port_span ().get_elements_by_type<dsp::AudioPort> ())
        {
          for (; last_index < dest.in_ports_.size (); ++last_index)
            {
              auto in_port_var = dest.get_input_port_span ()[last_index];
              if (std::holds_alternative<dsp::AudioPort *> (in_port_var))
                {
                  mgr->remove_connection (
                    out_port->get_uuid (),
                    std::get<dsp::AudioPort *> (in_port_var)->get_uuid ());
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
  for (
    auto out_port :
    get_output_port_span ().get_elements_by_type<dsp::MidiPort> ())
    {
      for (
        auto in_port :
        dest.get_input_port_span ().get_elements_by_type<dsp::MidiPort> ())
        {
          mgr->remove_connection (out_port->get_uuid (), in_port->get_uuid ());
        }
    }
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

std::optional<dsp::PortPtrVariant>
Plugin::get_port_by_symbol (const utils::Utf8String &sym)
{
  z_return_val_if_fail (
    get_protocol () == Protocol::ProtocolType::LV2, std::nullopt);

  auto find_port = [&sym] (const auto &ports) {
    auto it = std::ranges::find_if (ports, [&sym] (const auto &port_var) {
      return std::visit (
        [&] (auto &&port) { return port->get_symbol () == sym; }, port_var);
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

void
from_json (const nlohmann::json &j, Plugin &p)
{
  from_json (j, static_cast<Plugin::UuidIdentifiableObject &> (p));
  j.at (Plugin::kTrackIdKey).get_to (p.track_id_);
  j.at (Plugin::kSettingKey).get_to (p.setting_);
  for (const auto &port_json : j.at (Plugin::kInPortsKey))
    {
      dsp::PortUuidReference port_id_ref{ *p.port_registry_ };
      from_json (port_json, port_id_ref);
      p.in_ports_.push_back (std::move (port_id_ref));
    }
  for (const auto &port_json : j.at (Plugin::kOutPortsKey))
    {
      dsp::PortUuidReference port_id_ref{ *p.port_registry_ };
      from_json (port_json, port_id_ref);
      p.out_ports_.push_back (std::move (port_id_ref));
    }
  j.at (Plugin::kBanksKey).get_to (p.banks_);
  j.at (Plugin::kSelectedBankKey).get_to (p.selected_bank_);
  j.at (Plugin::kSelectedPresetKey).get_to (p.selected_preset_);
  j.at (Plugin::kVisibleKey).get_to (p.visible_);
  j.at (Plugin::kStateDirectoryKey).get_to (p.state_dir_);
}

} // namespace zrythm::gui::old_dsp::plugins
