// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include "common/dsp/audio_port.h"
#include "common/dsp/automatable_track.h"
#include "common/dsp/automation_tracklist.h"
#include "common/dsp/channel.h"
#include "common/dsp/channel_track.h"
#include "common/dsp/control_port.h"
#include "common/dsp/cv_port.h"
#include "common/dsp/engine.h"
#include "common/dsp/midi_event.h"
#include "common/dsp/midi_port.h"
#include "common/dsp/modulator_track.h"
#include "common/dsp/port_identifier.h"
#include "common/dsp/track.h"
#include "common/dsp/tracklist.h"
#include "common/dsp/transport.h"
#include "common/plugins/cached_plugin_descriptors.h"
#include "common/plugins/carla_native_plugin.h"
#include "common/plugins/plugin.h"
#include "common/plugins/plugin_manager.h"
#include "common/utils/dsp.h"
#include "common/utils/exceptions.h"
#include "common/utils/file.h"
#include "common/utils/flags.h"
#include "common/utils/gtest_wrapper.h"
#include "common/utils/io.h"
#include "common/utils/math.h"
#include "common/utils/mem.h"
#include "common/utils/objects.h"
#include "common/utils/rt_thread_id.h"
#include "common/utils/string.h"
#include "common/utils/ui.h"
#include "gui/backend/backend/actions/undo_manager.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings/g_settings_manager.h"
#include "gui/backend/backend/settings/settings.h"

#include <fmt/printf.h>
#define _GNU_SOURCE 1 /* To pick up REG_RIP */

#include <glib/gi18n.h>

using namespace zrythm::plugins;

Plugin::~Plugin ()
{
  if (activated_)
    {
      z_return_if_fail (!visible_);
    }
}

std::unique_ptr<zrythm::plugins::Plugin>
Plugin::create_with_setting (
  const PluginSetting            &setting,
  unsigned int                    track_name_hash,
  zrythm::plugins::PluginSlotType slot_type,
  int                             slot)
{
  return std::make_unique<CarlaNativePlugin> (
    setting, track_name_hash, slot_type, slot);
}

void
Plugin::set_stereo_outs_and_midi_in ()
{
  /* set the L/R outputs */
  int num_audio_outs = 0;
  for (auto &out_port : out_ports_)
    {
      if (out_port->is_audio ())
        {
          if (num_audio_outs == 0)
            {
              out_port->id_->flags_ |= PortIdentifier::Flags::StereoL;
              l_out_ = dynamic_cast<AudioPort *> (out_port.get ());
            }
          else if (num_audio_outs == 1)
            {
              out_port->id_->flags_ |= PortIdentifier::Flags::StereoR;
              r_out_ = dynamic_cast<AudioPort *> (out_port.get ());
            }
          num_audio_outs++;
        }
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
  for (auto &port : in_ports_)
    {
      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags2, port->id_->flags2_,
          PortIdentifier::Flags2::SupportsMidi))
        {
          midi_in_port_ = dynamic_cast<MidiPort *> (port.get ());
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
  for (auto &port : in_ports_)
    {
      const auto &id = port->id_;
      if (
        !(port->is_control ()
          && ENUM_BITSET_TEST (
            PortIdentifier::Flags, id->flags_,
            PortIdentifier::Flags::GenericPluginPort)))
        continue;

      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, id->flags_,
          PortIdentifier::Flags::PluginEnabled))
        {
          enabled_ = dynamic_cast<ControlPort *> (port.get ());
        }
      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, id->flags_, PortIdentifier::Flags::PluginGain))
        {
          gain_ = dynamic_cast<ControlPort *> (port.get ());
        }
    }
  z_return_if_fail (enabled_ && gain_);
}

bool
Plugin::is_in_active_project () const
{
  return track_ && track_->is_in_active_project ();
}

bool
Plugin::is_auditioner () const
{
  return track_ && track_->is_auditioner ();
}

void
Plugin::init_loaded (AutomatableTrack * track, MixerSelections * ms)
{
  track_ = track;
  ms_ = ms;

  std::vector<Port *> ports;
  append_ports (ports);
  z_return_if_fail (!ports.empty ());
  for (auto &port : ports)
    {
      port->plugin_ = this;
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
          throw ZrythmException (format_str (
            _ ("Instantiation failed for plugin '{}'. Disabling..."),
            get_name ()));
        }

      activate (true);
      set_enabled (was_enabled, false);
    }
}

void
Plugin::init (
  unsigned int                    track_name_hash,
  zrythm::plugins::PluginSlotType slot_type,
  int                             slot)
{
  z_debug (
    "{} ({}) track name hash {} slot {}", get_name (),
    ENUM_NAME (get_protocol ()), track_name_hash, slot);

  z_return_if_fail (
    PluginIdentifier::validate_slot_type_slot_combo (slot_type, slot));

  id_.track_name_hash_ = track_name_hash;
  id_.slot_type_ = slot_type;
  id_.slot_ = slot;

  /* add enabled port */
  auto port = std::make_unique<ControlPort> (_ ("Enabled"));
  port->id_->sym_ = "enabled";
  port->id_->comment_ = _ ("Enables or disables the plugin");
  port->id_->port_group_ = "[Zrythm]";
  port->id_->flags_ |= PortIdentifier::Flags::PluginEnabled;
  port->id_->flags_ |= PortIdentifier::Flags::Toggle;
  port->id_->flags_ |= PortIdentifier::Flags::Automatable;
  port->id_->flags_ |= PortIdentifier::Flags::GenericPluginPort;
  port->minf_ = 0.f;
  port->maxf_ = 1.f;
  port->zerof_ = 0.f;
  port->deff_ = 1.f;
  port->control_ = 1.f;
  port->unsnapped_control_ = 1.f;
  port->carla_param_id_ = -1;
  enabled_ = dynamic_cast<ControlPort *> (add_in_port (std::move (port)));

  /* add gain port */
  port = std::make_unique<ControlPort> (_ ("Gain"));
  port->id_->sym_ = "gain";
  port->id_->comment_ = _ ("Plugin gain");
  port->id_->flags_ |= PortIdentifier::Flags::PluginGain;
  port->id_->flags_ |= PortIdentifier::Flags::Automatable;
  port->id_->flags_ |= PortIdentifier::Flags::GenericPluginPort;
  port->id_->port_group_ = "[Zrythm]";
  port->minf_ = 0.f;
  port->maxf_ = 8.f;
  port->zerof_ = 0.f;
  port->deff_ = 1.f;
  port->set_control_value (1.f, false, false);
  port->carla_param_id_ = -1;
  gain_ = dynamic_cast<ControlPort *> (add_in_port (std::move (port)));

  selected_bank_.bank_idx_ = -1;
  selected_bank_.idx_ = -1;
  selected_preset_.bank_idx_ = -1;
  selected_preset_.idx_ = -1;

  set_ui_refresh_rate ();

  z_return_if_fail (gain_ && enabled_);

  /* select the init preset */
  selected_bank_.bank_idx_ = 0;
  selected_bank_.idx_ = -1;
  selected_bank_.plugin_id_ = id_;
  selected_preset_.bank_idx_ = 0;
  selected_preset_.idx_ = 0;
  selected_preset_.plugin_id_ = id_;

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
  bank.id_.plugin_id_ = id_;
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

Plugin::Plugin (
  const PluginSetting            &setting,
  unsigned int                    track_name_hash,
  zrythm::plugins::PluginSlotType slot_type,
  int                             slot)
    : setting_ (setting.clone_unique ())
{
  const auto &descr = setting_->descr_;

  z_debug (
    "creating plugin: {} ({}) slot {}", descr->name_,
    ENUM_NAME (descr->protocol_), slot);

  init (track_name_hash, slot_type, slot);
}

Plugin::Plugin (ZPluginCategory cat, unsigned int track_name_hash, int slot)
{
  zrythm::plugins::PluginDescriptor descr;
  descr.author_ = "Hoge";
  descr.name_ = "Dummy Plugin";
  descr.category_ = cat;
  descr.category_str_ = "Dummy Plugin Category";

  setting_ = std::make_unique<PluginSetting> (descr);

  init (track_name_hash, zrythm::plugins::PluginSlotType::Insert, slot);
}

void
Plugin::append_ports (std::vector<Port *> &ports)
{
  for (auto &port : in_ports_)
    {
      z_return_if_fail (port);
      ports.push_back (port.get ());
    }
  for (auto &port : out_ports_)
    {
      z_return_if_fail (port);
      ports.push_back (port.get ());
    }
}

void
Plugin::remove_ats_from_automation_tracklist (bool free_ats, bool fire_events)
{
  auto track = get_track ();
  z_return_if_fail (track);
  auto &atl = track->get_automation_tracklist ();
  for (auto it = atl.ats_.rbegin (); it != atl.ats_.rend (); ++it)
    {
      auto &at = *it;
      if (
        at->port_id_->owner_type_ == PortIdentifier::OwnerType::Plugin
        || ENUM_BITSET_TEST (
          PortIdentifier::Flags, at->port_id_->flags_,
          PortIdentifier::Flags::PluginControl))
        {
          if (
            at->port_id_->plugin_id_.slot_ == id_.slot_
            && at->port_id_->plugin_id_.slot_type_ == id_.slot_type_)
            {
              atl.remove_at (*at, free_ats, fire_events);
            }
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
  Plugin *                        pl = nullptr;
  AutomatableTrack *              track = nullptr;
  zrythm::plugins::PluginSlotType slot_type{};
  int                             slot = 0;
  bool                            fire_events = false;
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
  auto pl = data->pl;
  auto prev_slot = pl->id_.slot_;
  auto prev_slot_type = pl->id_.slot_type_;
  auto prev_track = pl->get_track ();
  z_return_if_fail (prev_track);
  auto prev_ch = pl->get_channel ();
  z_return_if_fail (prev_ch);

  auto data_channel_track = dynamic_cast<ChannelTrack *> (data->track);

  /* if existing plugin exists, delete it */
  auto existing_pl =
    data->track->get_plugin_at_slot (data->slot_type, data->slot);
  if (existing_pl)
    {
      data_channel_track->channel_->remove_plugin (
        data->slot_type, data->slot, false, true, false, false);
    }

  /* move plugin's automation from src to dest */
  pl->move_automation (*prev_track, *data->track, data->slot_type, data->slot);

  /* remove plugin from its channel */
  auto plugin_ptr = prev_ch->remove_plugin (
    prev_slot_type, prev_slot, true, false, false, false);
  z_return_if_fail (plugin_ptr && plugin_ptr.get () == pl);

  /* add plugin to its new channel */
  data_channel_track->channel_->add_plugin (
    std::move (plugin_ptr), data->slot_type, data->slot, false, true, false,
    true, true);

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
  AutomatableTrack *              track,
  zrythm::plugins::PluginSlotType slot_type,
  int                             slot,
  bool                            confirm_overwrite,
  bool                            fire_events)
{
  auto data = std::make_unique<PluginMoveData> ();
  data->pl = this;
  data->track = track;
  data->slot_type = slot_type;
  data->slot = slot;
  data->fire_events = fire_events;

  auto existing_pl = track->get_plugin_at_slot (slot_type, slot);
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

void
Plugin::set_track_and_slot (
  unsigned int                    track_name_hash,
  zrythm::plugins::PluginSlotType slot_type,
  int                             slot)
{
  z_return_if_fail (
    PluginIdentifier::validate_slot_type_slot_combo (slot_type, slot));

  id_.track_name_hash_ = track_name_hash;
  id_.slot_ = slot;
  id_.slot_type_ = slot_type;

  auto track = get_track ();
  for (auto &port : in_ports_)
    {
      auto copy_id = port->id_;
      port->set_owner<zrythm::plugins::Plugin> (this);
      if (is_in_active_project ())
        {
          port->update_identifier (*copy_id, track, false);
        }
    }
  for (auto &port : out_ports_)
    {
      auto copy_id = port->id_;
      port->set_owner<zrythm::plugins::Plugin> (this);
      if (is_in_active_project ())
        {
          port->update_identifier (*copy_id, track, false);
        }
    }
}

AutomatableTrack *
Plugin::get_track () const
{
  z_return_val_if_fail (track_, nullptr);
  return track_;
}

Channel *
Plugin::get_channel () const
{
  auto track = dynamic_cast<ChannelTrack *> (get_track ());
  z_return_val_if_fail (track, nullptr);
  auto ch = track->channel_;
  z_return_val_if_fail (ch, nullptr);

  return ch;
}

Plugin *
Plugin::find (const PluginIdentifier &id)
{
  auto track = TRACKLIST->find_track_by_name_hash (id.track_name_hash_);
  z_return_val_if_fail (track, nullptr);

  Channel * ch = nullptr;
  if (
    !track->is_modulator ()
    || id.slot_type_ == zrythm::plugins::PluginSlotType::MidiFx
    || id.slot_type_ == zrythm::plugins::PluginSlotType::Instrument
    || id.slot_type_ == zrythm::plugins::PluginSlotType::Insert)
    {
      auto channel_track = dynamic_cast<ChannelTrack *> (track);
      ch = channel_track->channel_;
      z_return_val_if_fail (ch, nullptr);
    }
  Plugin * ret = nullptr;
  switch (id.slot_type_)
    {
    case zrythm::plugins::PluginSlotType::MidiFx:
      z_return_val_if_fail (ch, nullptr);
      ret = ch->midi_fx_[id.slot_].get ();
      break;
    case zrythm::plugins::PluginSlotType::Instrument:
      z_return_val_if_fail (ch, nullptr);
      ret = ch->instrument_.get ();
      break;
    case zrythm::plugins::PluginSlotType::Insert:
      z_return_val_if_fail (ch, nullptr);
      ret = ch->inserts_[id.slot_].get ();
      break;
    case zrythm::plugins::PluginSlotType::Modulator:
      {
        auto modulator_track = dynamic_cast<ModulatorTrack *> (track);
        z_return_val_if_fail (modulator_track, nullptr);
        ret = modulator_track->modulators_[id.slot_].get ();
      }
      break;
    default:
      z_return_val_if_reached (nullptr);
      break;
    }
  z_return_val_if_fail (ret, nullptr);

  return ret;
}

std::string
Plugin::get_full_port_group_designation (const std::string &port_group) const
{
  auto track = get_track ();
  z_return_val_if_fail (track, {});
  return fmt::format ("{}/{}/{}", track->name_, get_name (), port_group);
}

Port *
Plugin::get_port_in_group (const std::string &port_group, bool left) const
{
  auto flag =
    left ? PortIdentifier::Flags::StereoL : PortIdentifier::Flags::StereoR;
  for (const auto &port : in_ports_)
    {
      if (
        port->id_->port_group_ == port_group
        && ENUM_BITSET_TEST (PortIdentifier::Flags, port->id_->flags_, flag))
        {
          return port.get ();
        }
    }
  for (const auto &port : out_ports_)
    {
      if (
        port->id_->port_group_ == port_group
        && ENUM_BITSET_TEST (PortIdentifier::Flags, port->id_->flags_, flag))
        {
          return port.get ();
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
    port.id_->flow_ == PortFlow::Input ? in_ports_ : out_ports_;

  for (const auto &cur_port : ports)
    {
      if (&port == cur_port.get ())
        {
          continue;
        }

      if (port.id_->port_group_ == cur_port->id_->port_group_ && ((ENUM_BITSET_TEST (PortIdentifier::Flags, cur_port->id_->flags_, PortIdentifier::Flags::StereoL) && ENUM_BITSET_TEST (PortIdentifier::Flags, port.id_->flags_, PortIdentifier::Flags::StereoR)) || (ENUM_BITSET_TEST (PortIdentifier::Flags, cur_port->id_->flags_, PortIdentifier::Flags::StereoR) && ENUM_BITSET_TEST (PortIdentifier::Flags, port.id_->flags_, PortIdentifier::Flags::StereoL))))
        {
          return cur_port.get ();
        }
    }

  return nullptr;
}

std::string
Plugin::generate_window_title () const
{
  z_return_val_if_fail (is_in_active_project (), {});

  auto track = get_track ();

  const auto track_name = track ? track->name_ : "";
  const auto plugin_name = get_name ();
  z_return_val_if_fail (!track_name.empty () && !plugin_name.empty (), {});

  std::string bridge_mode;
  if (setting_->bridge_mode_ != zrythm::plugins::CarlaBridgeMode::None)
    {
      bridge_mode =
        fmt::format (" - bridge: {}", ENUM_NAME (setting_->bridge_mode_));
    }

  std::string slot;
  if (id_.slot_type_ == zrythm::plugins::PluginSlotType::Instrument)
    {
      slot = "instrument";
    }
  else
    {
      slot = fmt::format ("#{}", id_.slot_ + 1);
    }

  return fmt::format (
    "{} ({} {}{}{})", plugin_name, track_name, slot,
    /* assume all plugins use carla for now */
    "",
    /*setting->open_with_carla_ ? " carla" : "",*/
    bridge_mode);
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

void
Plugin::update_latency ()
{
  latency_ = get_latency ();

  z_debug ("{} latency: {} samples", get_name (), latency_);
}

Port *
Plugin::add_in_port (std::unique_ptr<Port> &&port)
{
  port->id_->port_index_ = in_ports_.size ();
  port->set_owner<zrythm::plugins::Plugin> (this);
  in_ports_.emplace_back (std::move (port));
  return in_ports_.back ().get ();
}

Port *
Plugin::add_out_port (std::unique_ptr<Port> &&port)
{
  port->id_->port_index_ = out_ports_.size ();
  port->set_owner<zrythm::plugins::Plugin> (this);
  out_ports_.push_back (std::move (port));
  return out_ports_.back ().get ();
}

void
Plugin::move_automation (
  AutomatableTrack               &prev_track,
  AutomatableTrack               &track,
  zrythm::plugins::PluginSlotType new_slot_type,
  int                             new_slot)
{
  z_debug (
    "moving plugin '%s' automation from "
    "{} to {} -> {}:{}",
    get_name (), prev_track.name_, track.name_, ENUM_NAME (new_slot_type),
    new_slot);

  auto &prev_atl = prev_track.get_automation_tracklist ();
  auto &atl = track.get_automation_tracklist ();

  auto name_hash = track.get_name_hash ();
  for (auto &at : prev_atl.ats_)
    {
      auto port = Port::find_from_identifier<ControlPort> (*at->port_id_);
      if (!port)
        continue;
      if (port->id_->owner_type_ == PortIdentifier::OwnerType::Plugin)
        {
          auto port_pl = port->get_plugin (true);
          if (port_pl != this)
            continue;
        }
      else
        continue;

      z_return_if_fail (port->at_ == at);

      /* delete from prev channel */
      auto num_regions_before = at->regions_.size ();
      auto * removed_at = prev_atl.remove_at (*at, false, false);

      /* add to new channel */
      auto added_at = atl.add_at (*removed_at);
      z_return_if_fail (
        added_at == atl.ats_[added_at->index_]
        && added_at->regions_.size () == num_regions_before);

      /* update the automation track port identifier */
      added_at->port_id_->plugin_id_.slot_ = new_slot;
      added_at->port_id_->plugin_id_.slot_type_ = new_slot_type;
      added_at->port_id_->plugin_id_.track_name_hash_ = name_hash;

      z_return_if_fail (
        added_at->port_id_->port_index_ == port->id_->port_index_);
    }
}

void
Plugin::set_ui_refresh_rate ()
{
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
}

void
Plugin::generate_automation_tracks (AutomatableTrack &track)
{
  z_debug ("generating automation tracks for {}...", get_name ());

  auto &atl = track.get_automation_tracklist ();
  for (auto port : in_ports_ | type_is<ControlPort> ())
    {
      if (
        port->id_->type_ != PortType::Control
        || !(ENUM_BITSET_TEST (
          PortIdentifier::Flags, port->id_->flags_,
          PortIdentifier::Flags::Automatable)))
        continue;

      auto * at = new AutomationTrack (*port);
      atl.add_at (*at);
    }
}

/**
 * Gets the enable/disable port for this plugin.
 */
Port *
Plugin::get_enabled_port ()
{
  for (auto &port : in_ports_)
    {
      if (
        ENUM_BITSET_TEST (
          PortIdentifier::Flags, port->id_->flags_,
          PortIdentifier::Flags::PluginEnabled)
        && ENUM_BITSET_TEST (
          PortIdentifier::Flags, port->id_->flags_,
          PortIdentifier::Flags::GenericPluginPort))
        {
          return port.get ();
        }
    }
  z_return_val_if_reached (nullptr);
}

void
Plugin::update_identifier ()
{
  z_return_if_fail (track_);

  /* set port identifier track poses */
  for (auto &port : in_ports_)
    {
      port->update_track_name_hash (*track_, id_.track_name_hash_);
      port->id_->plugin_id_ = id_;
    }
  for (auto &port : out_ports_)
    {
      port->update_track_name_hash (*track_, id_.track_name_hash_);
      port->id_->plugin_id_ = id_;
    }
}

void
Plugin::set_track_name_hash (unsigned int track_name_hash)
{
  id_.track_name_hash_ = track_name_hash;

  update_identifier ();
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

  for (auto &port : out_ports_)
    {
      port->clear_buffer (*AUDIO_ENGINE);
    }
}

void
Plugin::process (const EngineProcessTimeInfo time_nfo)
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
  if (!math_floats_equal_epsilon (gain_->control_, 1.f, 0.001f))
    {
      for (auto &port : out_ports_)
        {
          if (!port->is_audio ())
            continue;

          /* if close to 0 set it to the denormal prevention val */
          if (math_floats_equal_epsilon (gain_->control_, 0.f, 0.00001f))
            {
              dsp_fill (
                &port->buf_[time_nfo.local_offset_],
                DENORMAL_PREVENTION_VAL (AUDIO_ENGINE), time_nfo.nframes_);
            }
          /* otherwise just apply gain */
          else
            {
              dsp_mul_k2 (
                &port->buf_[time_nfo.local_offset_], gain_->control_,
                time_nfo.nframes_);
            }
        }
    }
}

std::string
Plugin::print () const
{
  auto track = is_in_active_project () ? track_ : nullptr;
  return fmt::format (
    "{} ({}):{}:{} - {}", track ? track->get_name () : "<no track>",
    track ? track->pos_ : -1, ENUM_NAME (id_.slot_type_), id_.slot_,
    get_name ());
}

void
Plugin::set_caches ()
{
  ctrl_in_ports_.clear ();
  audio_in_ports_.clear ();
  cv_in_ports_.clear ();
  midi_in_ports_.clear ();

  for (auto &port : in_ports_)
    {
      switch (port->id_->type_)
        {
        case PortType::Control:
          ctrl_in_ports_.push_back (dynamic_cast<ControlPort *> (port.get ()));
          break;
        case PortType::Audio:
          audio_in_ports_.push_back (dynamic_cast<AudioPort *> (port.get ()));
          break;
        case PortType::CV:
          cv_in_ports_.push_back (dynamic_cast<CVPort *> (port.get ()));
          break;
        case PortType::Event:
          midi_in_ports_.push_back (dynamic_cast<MidiPort *> (port.get ()));
          break;
        default:
          break;
        }
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

bool
Plugin::is_selected () const
{
  return MIXER_SELECTIONS->contains_plugin (*this);
}

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
      MIXER_SELECTIONS->add_slot (*track, id_.slot_type_, id_.slot_, true);
    }
  else
    {
      MIXER_SELECTIONS->remove_slot (id_.slot_, id_.slot_type_, true);
    }
}

void
Plugin::copy_state_dir (
  const Plugin       &src,
  bool                is_backup,
  const std::string * abs_state_dir)
{
  auto dir_to_use =
    abs_state_dir ? *abs_state_dir : get_abs_state_dir (is_backup, true);
  {
    auto files_in_dir = io_get_files_in_dir (dir_to_use);
    z_return_if_fail (files_in_dir.isEmpty ());
  }

  auto src_dir_to_use = src.get_abs_state_dir (is_backup);
  z_return_if_fail (!src_dir_to_use.empty ());
  {
    auto files_in_src_dir = io_get_files_in_dir (src_dir_to_use);
    z_return_if_fail (!files_in_src_dir.isEmpty ());
  }

  io_copy_dir (dir_to_use, src_dir_to_use, true, true);
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
      io_mkdir (abs_state_dir);
      return;
    }

  auto escaped_name = io_get_legal_file_name (get_name ());
  auto parent_dir = PROJECT->get_path (ProjectPath::PluginStates, is_backup);
  z_return_if_fail (!parent_dir.empty ());
  io_mkdir (parent_dir);
  auto tmp = fmt::format ("{}_XXXXXX", escaped_name);
  auto abs_state_dir_template = fs::path (parent_dir) / tmp;
  tmp = abs_state_dir_template.string ();
  char * abs_state_dir = g_mkdtemp (tmp.data ());
  if (!abs_state_dir)
    {
      throw ZrythmException (format_str (
        "Failed to make state dir using template {}: {}",
        abs_state_dir_template.string (), strerror (errno)));
    }
  state_dir_ = Glib::path_get_basename (abs_state_dir);
  z_debug ("set plugin state dir to {}", state_dir_);
}

void
Plugin::get_all (
  Project                                &prj,
  std::vector<zrythm::plugins::Plugin *> &arr,
  bool                                    check_undo_manager)
{
  const auto &tracks = prj.tracklist_->tracks_;
  std::ranges::for_each (tracks.begin (), tracks.end (), [&] (auto &&track) {
    std::visit ([&] (auto &&tr) { tr->get_plugins (arr); }, track);
  });

  if (check_undo_manager)
    {
      prj.undo_manager_->get_plugins (arr);
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
  init (other.id_.track_name_hash_, other.id_.slot_type_, other.id_.slot_);

  /* copy ports */
  z_debug ("[3/5] copying ports from source plugin");
  enabled_ = nullptr;
  gain_ = nullptr;
  in_ports_.clear ();
  out_ports_.clear ();
  in_ports_.reserve (other.in_ports_.size ());
  out_ports_.reserve (other.out_ports_.size ());
  for (auto &port : other.in_ports_)
    {
      std::visit (
        [&] (auto &&p) {
          auto new_port = p->clone_unique ();
          new_port->set_owner (this);
          in_ports_.push_back (std::move (new_port));
        },
        convert_to_variant<PortPtrVariant> (port.get ()));
    }
  for (auto &port : other.out_ports_)
    {
      std::visit (
        [&] (auto &&p) {
          auto new_port = p->clone_unique ();
          new_port->set_owner (this);
          out_ports_.push_back (std::move (new_port));
        },
        convert_to_variant<PortPtrVariant> (port.get ()));
    }
  set_enabled_and_gain ();

  /* copy the state directory */
  z_debug ("[4/5] copying state directory from source plugin");
  copy_state_dir (other, false, nullptr);

  z_debug ("[5/5] done");

  id_ = other.id_;
  visible_ = other.visible_;
}

bool
Plugin::is_enabled (bool check_track) const
{
  if (!enabled_->is_toggled ())
    return false;

  if (check_track)
    {
      auto track = get_track ();
      z_return_val_if_fail (track, false);
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
  for (auto &in_port : in_ports_)
    {
      bool goto_next = false;
      switch (in_port->id_->type_)
        {
        case PortType::Audio:
          for (size_t j = last_audio_idx; j < out_ports_.size (); j++)
            {
              auto &out_port = out_ports_[j];
              if (out_port->is_audio ())
                {
                  /* copy */
                  dsp_copy (
                    &out_port->buf_[time_nfo.local_offset_],
                    &in_port->buf_[time_nfo.local_offset_], time_nfo.nframes_);

                  last_audio_idx = j + 1;
                  goto_next = true;
                  break;
                }
              if (goto_next)
                continue;
            }
          break;
        case PortType::Event:
          for (size_t j = last_midi_idx; j < out_ports_.size (); j++)
            {
              auto &out_port = out_ports_[j];
              if (
                out_port->id_->type_ == PortType::Event
                && ENUM_BITSET_TEST (
                  PortIdentifier::Flags2, out_port->id_->flags2_,
                  PortIdentifier::Flags2::SupportsMidi))
                {
                  auto midi_in_port = dynamic_cast<MidiPort *> (in_port.get ());
                  auto midi_out_port =
                    dynamic_cast<MidiPort *> (out_port.get ());
                  /* copy */
                  midi_out_port->midi_events_.active_events_.append (
                    midi_in_port->midi_events_.active_events_,
                    time_nfo.local_offset_, time_nfo.nframes_);

                  last_midi_idx = j + 1;
                  goto_next = true;
                  break;
                }
              if (goto_next)
                continue;
            }
          break;
        default:
          break;
        }
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
  size_t num_src_audio_outs = 0;
  size_t num_dest_audio_ins = 0;

  PortConnectionsManager * connections_mgr = PORT_CONNECTIONS_MGR;

  for (const auto &out_port : out_ports_)
    {
      if (out_port->is_audio ())
        num_src_audio_outs++;
    }

  for (const auto &in_port : dest.in_ports_)
    {
      if (in_port->is_audio ())
        num_dest_audio_ins++;
    }

  if (num_src_audio_outs == 1 && num_dest_audio_ins == 1)
    {
      for (auto &out_port : out_ports_)
        {
          if (out_port->is_audio ())
            {
              for (auto &in_port : dest.in_ports_)
                {
                  if (in_port->is_audio ())
                    {
                      out_port->connect_to (*connections_mgr, *in_port, true);
                      goto done1;
                    }
                }
            }
        }
    }
  else if (num_src_audio_outs == 1 && num_dest_audio_ins > 1)
    {
      /* plugin is mono and next plugin is not mono, so connect the mono out to
       * each input */
      for (auto &out_port : out_ports_)
        {
          if (out_port->is_audio ())
            {
              for (auto &in_port : dest.in_ports_)
                {
                  if (in_port->is_audio ())
                    {
                      out_port->connect_to (*connections_mgr, *in_port, true);
                    }
                }
              break;
            }
        }
    }
  else if (num_src_audio_outs > 1 && num_dest_audio_ins == 1)
    {
      /* connect multi-output channel into mono by only connecting to the first
       * input channel found */
      for (auto &in_port : dest.in_ports_)
        {
          if (in_port->is_audio ())
            {
              for (auto &out_port : out_ports_)
                {
                  if (out_port->is_audio ())
                    {
                      out_port->connect_to (*connections_mgr, *in_port, true);
                      goto done1;
                    }
                }
              break;
            }
        }
    }
  else if (num_src_audio_outs > 1 && num_dest_audio_ins > 1)
    {
      /* connect to as many audio outs this plugin has, or until we can't
       * connect anymore */
      auto num_ports_to_connect = MIN (num_src_audio_outs, num_dest_audio_ins);
      size_t last_index = 0;
      size_t ports_connected = 0;
      for (auto &out_port : out_ports_)
        {
          if (out_port->is_audio ())
            {
              for (; last_index < dest.in_ports_.size (); last_index++)
                {
                  auto &in_port = *dest.in_ports_[last_index];
                  if (in_port.is_audio ())
                    {
                      out_port->connect_to (*connections_mgr, in_port, true);
                      last_index++;
                      ports_connected++;
                      break;
                    }
                }
              if (ports_connected == num_ports_to_connect)
                break;
            }
        }
    }

done1:

  /* connect prev midi outs to next midi ins */
  /* this connects only one midi out to all of the midi ins of the next plugin */
  for (auto &out_port : out_ports_)
    {
      if (
        out_port->id_->type_ == PortType::Event
        && ENUM_BITSET_TEST (
          PortIdentifier::Flags2, out_port->id_->flags2_,
          PortIdentifier::Flags2::SupportsMidi))
        {
          for (auto &in_port : dest.in_ports_)
            {
              if (
                in_port->id_->type_ == PortType::Event
                && ENUM_BITSET_TEST (
                  PortIdentifier::Flags2, in_port->id_->flags2_,
                  PortIdentifier::Flags2::SupportsMidi))
                {
                  out_port->connect_to (*connections_mgr, *in_port, true);
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

  auto track = ch.get_track ();
  auto type = track->out_signal_type_;

  if (type == PortType::Event)
    {
      for (auto &out_port : out_ports_)
        {
          if (
            out_port->id_->type_ == PortType::Event
            && ENUM_BITSET_TEST (
              PortIdentifier::Flags2, out_port->id_->flags2_,
              PortIdentifier::Flags2::SupportsMidi)
            && out_port->id_->flow_ == PortFlow::Output)
            {
              out_port->connect_to (*PORT_CONNECTIONS_MGR, *ch.midi_out_, true);
            }
        }
    }
  else if (type == PortType::Audio)
    {
      if (l_out_ && r_out_)
        {
          l_out_->connect_to (
            *PORT_CONNECTIONS_MGR, ch.prefader_->stereo_in_->get_l (), true);
          r_out_->connect_to (
            *PORT_CONNECTIONS_MGR, ch.prefader_->stereo_in_->get_r (), true);
        }
    }
}

void
Plugin::disconnect_from_prefader (Channel &ch)
{
  auto track = ch.get_track ();
  auto type = track->out_signal_type_;

  for (auto &out_port : out_ports_)
    {
      if (type == PortType::Audio && out_port->id_->type_ == PortType::Audio)
        {
          if (out_port->is_connected_to (ch.prefader_->stereo_in_->get_l ()))
            out_port->disconnect_from (
              *PORT_CONNECTIONS_MGR, ch.prefader_->stereo_in_->get_l ());
          if (out_port->is_connected_to (ch.prefader_->stereo_in_->get_r ()))
            out_port->disconnect_from (
              *PORT_CONNECTIONS_MGR, ch.prefader_->stereo_in_->get_r ());
        }
      else if (
        type == PortType::Event && out_port->id_->type_ == PortType::Event
        && ENUM_BITSET_TEST (
          PortIdentifier::Flags2, out_port->id_->flags2_,
          PortIdentifier::Flags2::SupportsMidi))
        {
          if (out_port->is_connected_to (*ch.prefader_->midi_in_))
            out_port->disconnect_from (
              *PORT_CONNECTIONS_MGR, *ch.prefader_->midi_in_);
        }
    }
}

void
Plugin::disconnect_from_plugin (Plugin &dest)
{
  int    num_src_audio_outs = 0;
  size_t num_dest_audio_ins = 0;

  for (auto &out_port : out_ports_)
    {
      if (out_port->is_audio ())
        num_src_audio_outs++;
    }

  for (auto &in_port : dest.in_ports_)
    {
      if (in_port->is_audio ())
        num_dest_audio_ins++;
    }

  if (num_src_audio_outs == 1 && num_dest_audio_ins == 1)
    {
      for (auto &out_port : out_ports_)
        {
          if (out_port->is_audio ())
            {
              for (auto &in_port : dest.in_ports_)
                {
                  if (in_port->is_audio ())
                    {
                      out_port->disconnect_from (
                        *PORT_CONNECTIONS_MGR, *in_port);
                      goto done2;
                    }
                }
            }
        }
    }
  else if (num_src_audio_outs == 1 && num_dest_audio_ins > 1)
    {
      /* plugin is mono and next plugin is not mono, so disconnect the mono out
       * from each input */
      for (auto &out_port : out_ports_)
        {
          if (out_port->is_audio ())
            {
              for (auto &in_port : dest.in_ports_)
                {
                  if (in_port->is_audio ())
                    {
                      out_port->disconnect_from (
                        *PORT_CONNECTIONS_MGR, *in_port);
                    }
                }
              break;
            }
        }
    }
  else if (num_src_audio_outs > 1 && num_dest_audio_ins == 1)
    {
      /* disconnect multi-output channel from mono by disconnecting to the first
       * input channel found */
      for (auto &in_port : dest.in_ports_)
        {
          if (in_port->is_audio ())
            {
              for (auto &out_port : out_ports_)
                {
                  if (out_port->is_audio ())
                    {
                      out_port->disconnect_from (
                        *PORT_CONNECTIONS_MGR, *in_port);
                      goto done2;
                    }
                }
              break;
            }
        }
    }
  else if (num_src_audio_outs > 1 && num_dest_audio_ins > 1)
    {
      /* connect to as many audio outs this plugin has, or until we can't
       * connect anymore */
      auto num_ports_to_connect =
        std::min (num_src_audio_outs, (int) num_dest_audio_ins);
      size_t last_index = 0;
      size_t ports_disconnected = 0;
      for (auto &out_port : out_ports_)
        {
          if (out_port->is_audio ())
            {
              for (; last_index < dest.in_ports_.size (); ++last_index)
                {
                  auto &in_port = *dest.in_ports_[last_index];
                  if (in_port.is_audio ())
                    {
                      out_port->disconnect_from (*PORT_CONNECTIONS_MGR, in_port);
                      last_index++;
                      ports_disconnected++;
                      break;
                    }
                }
              if ((int) ports_disconnected == num_ports_to_connect)
                break;
            }
        }
    }

done2:

  /* disconnect MIDI connections */
  for (auto &out_port : out_ports_)
    {
      if (
        out_port->id_->type_ == PortType::Event
        && ENUM_BITSET_TEST (
          PortIdentifier::Flags2, out_port->id_->flags2_,
          PortIdentifier::Flags2::SupportsMidi))
        {
          for (auto &in_port : dest.in_ports_)
            {
              if (
                in_port->id_->type_ == PortType::Event
                && ENUM_BITSET_TEST (
                  PortIdentifier::Flags2, in_port->id_->flags2_,
                  PortIdentifier::Flags2::SupportsMidi))
                {
                  out_port->disconnect_from (*PORT_CONNECTIONS_MGR, *in_port);
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
      for (auto &port : in_ports_)
        port->disconnect_all ();
      for (auto &port : out_ports_)
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
      for (auto &port : in_ports_)
        set_expose (port);
    }
  if (outputs)
    {
      for (auto &port : out_ports_)
        set_expose (port);
    }
}

template <typename T>
T *
Plugin::get_port_by_symbol (const std::string &sym)
{
  z_return_val_if_fail (get_protocol () == Protocol::ProtocolType::LV2, nullptr);

  auto find_port = [&sym] (const auto &ports) {
    return std::find_if (ports.begin (), ports.end (), [&sym] (const auto &port) {
      return port->id_->sym_ == sym;
    });
  };

  auto it = find_port (in_ports_);
  if (it != in_ports_.end ())
    return dynamic_cast<T *> (it->get ());

  it = find_port (out_ports_);
  if (it != out_ports_.end ())
    return dynamic_cast<T *> (it->get ());

  z_warning ("failed to find port with symbol {}", sym);
  return nullptr;
}

std::unique_ptr<zrythm::plugins::Plugin>
Plugin::create_unique_from_hosting_type (PluginSetting::HostingType hosting_type)
{
  switch (hosting_type)
    {
    case PluginSetting::HostingType::Carla:
      return std::make_unique<CarlaNativePlugin> ();
    default:
      // TODO: implement other plugin hosting types
      z_return_val_if_reached (nullptr);
    }
}

template Port *
Plugin::get_port_by_symbol (const std::string &);
template ControlPort *
Plugin::get_port_by_symbol (const std::string &);
template AudioPort *
Plugin::get_port_by_symbol (const std::string &);
template CVPort *
Plugin::get_port_by_symbol (const std::string &);
template MidiPort *
Plugin::get_port_by_symbol (const std::string &);