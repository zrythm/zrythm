// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <ranges>

#include "dsp/midi_event.h"
#include "dsp/port.h"
#include "dsp/port_connections_manager.h"
#include "dsp/position.h"
#include "engine/session/router.h"
#include "gui/backend/backend/actions/mixer_selections_action.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/settings/settings.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/plugin.h"
#include "plugins/plugin_slot.h"
#include "structure/tracks/automation_track.h"
#include "structure/tracks/automation_tracklist.h"
#include "structure/tracks/channel.h"
#include "structure/tracks/group_target_track.h"
#include "structure/tracks/track.h"
#include "structure/tracks/track_processor.h"
#include "structure/tracks/tracklist.h"
#include "utils/logger.h"
#include "utils/objects.h"
#include "utils/rt_thread_id.h"
#include "utils/string.h"
#include "utils/views.h"

namespace zrythm::structure::tracks
{

MidiPreFader::MidiPreFader (
  dsp::PortRegistry               &port_registry,
  dsp::ProcessorParameterRegistry &param_registry,
  QObject *                        parent)
    : QObject (parent),
      dsp::MidiPassthroughProcessor (port_registry, param_registry)
{
  set_name (u8"MIDI PreFader");
}

AudioPreFader::AudioPreFader (
  dsp::PortRegistry               &port_registry,
  dsp::ProcessorParameterRegistry &param_registry,
  QObject *                        parent)
    : QObject (parent),
      dsp::StereoPassthroughProcessor (port_registry, param_registry)
{
  set_name (u8"Audio PreFader");
}

Channel::Channel (
  TrackRegistry                   &track_registry,
  PluginRegistry                  &plugin_registry,
  dsp::PortRegistry               &port_registry,
  dsp::ProcessorParameterRegistry &param_registry,
  OptionalRef<ChannelTrack>        track)
    : track_registry_ (track_registry), port_registry_ (port_registry),
      param_registry_ (param_registry), plugin_registry_ (plugin_registry)
{
  if (track.has_value ())
    {
      track_uuid_ = track->get_uuid ();
      track_ = std::addressof (track.value ());

      /* create ports */
      switch (track_->get_output_signal_type ())
        {
        case PortType::Audio:
          {
            stereo_out_left_id_ =
              get_port_registry ().create_object<dsp::AudioPort> (
                u8"Stereo out L", dsp::PortFlow::Output);
            stereo_out_right_id_ =
              get_port_registry ().create_object<dsp::AudioPort> (
                u8"Stereo out R", dsp::PortFlow::Output);
            get_stereo_out_ports ().first.set_symbol (u8"stereo_out_l");
            get_stereo_out_ports ().second.set_symbol (u8"stereo_out_r");
            get_stereo_out_ports ().first.set_full_designation_provider (this);
            get_stereo_out_ports ().second.set_full_designation_provider (this);
          }
          break;
        case PortType::Event:
          {
            midi_out_id_ = get_port_registry ().create_object<dsp::MidiPort> (
              utils::Utf8String::from_qstring (QObject::tr ("MIDI out")),
              dsp::PortFlow::Output);
            get_midi_out_port ().set_symbol (u8"midi_out");
            get_midi_out_port ().set_full_designation_provider (this);
          }
          break;
        default:
          break;
        }
    }
}

void
init_from (Channel &obj, const Channel &other, utils::ObjectCloneType clone_type)
{
  obj.output_track_uuid_ = other.output_track_uuid_;
  obj.width_ = other.width_;

  obj.ext_midi_ins_ = other.ext_midi_ins_;
  obj.ext_stereo_l_ins_ = other.ext_stereo_l_ins_;
  obj.ext_stereo_r_ins_ = other.ext_stereo_r_ins_;
  obj.midi_channels_ = other.midi_channels_;

  if (clone_type == utils::ObjectCloneType::Snapshot)
    {
      obj.midi_fx_ = other.midi_fx_;
      obj.inserts_ = other.inserts_;
      obj.instrument_ = other.instrument_;
      // TODO
      // utils::clone_unique_ptr_container (obj.sends_, other.sends_);
      // obj.fader_ = utils::clone_qobject (*other.fader_, &obj, clone_type);
      // obj.prefader_ = utils::clone_qobject (*other.prefader_, &obj,
      // clone_type);
      obj.midi_out_id_ = other.midi_out_id_;
      obj.stereo_out_left_id_ = other.stereo_out_left_id_;
      obj.stereo_out_right_id_ = other.stereo_out_right_id_;
      obj.track_uuid_ = other.track_uuid_;
    }
  else if (clone_type == utils::ObjectCloneType::NewIdentity)
    {
// TODO
#if 0
      const auto clone_from_registry = [] (auto &vec, const auto &other_vec) {
        for (const auto &[index, other_el] : utils::views::enumerate (other_vec))
          {
            if (other_el)
              {
                vec[index] = (other_el->clone_new_identity ());
              }
          }
        };

        clone_from_registry (obj.midi_fx_, other.midi_fx_);
        clone_from_registry (obj.inserts_, other.inserts_);
        if (other.instrument_.has_value ())
        {
          obj.instrument_ = other.instrument_->clone_new_identity ();
        }

        // Rest TODO
        throw std::runtime_error ("not implemented");
#endif
    }
}

std::optional<Channel::PluginPtrVariant>
Channel::get_plugin_from_id (PluginUuid id) const
{
  return plugin_registry_.find_by_id (id);
}

void
Channel::set_track_ptr (ChannelTrack &track)
{
  track_ = &track;

  // prefader_->track_ = track_;
  fader_->track_ = track_;

  std::vector<Channel::Plugin *> pls;
  get_plugins (pls);
  for (auto * pl : pls)
    {
      pl->set_track (track.get_uuid ());
    }
}

utils::Utf8String
Channel::get_full_designation_for_port (const dsp::Port &port) const
{
  const auto &tr = get_track ();
  return utils::Utf8String::from_utf8_encoded_string (
    fmt::format ("{}/{}", tr.get_name (), port.get_label ()));
}

#if 0
bool
Channel::should_bounce_to_master (utils::audio::BounceStep step) const
{
  // only post-fader bounces make sense for channel outputs
  if (step != utils::audio::BounceStep::PostFader)
    return false;

  const auto &track = get_track ();
  return !track.is_master () && track.bounce_to_master_;
}
#endif

void
Channel::prepare_process (nframes_t nframes)
{
  const auto out_type = track_->get_output_signal_type ();

  /* clear buffers */
  track_->processor_->clear_buffers (nframes);
  // prefader_->clear_buffers (nframes); // TODO
  fader_->clear_buffers (nframes);

  if (out_type == PortType::Audio)
    {
      get_stereo_out_ports ().first.clear_buffer (nframes);
      get_stereo_out_ports ().second.clear_buffer (nframes);
    }
  else if (out_type == PortType::Event)
    {
      get_midi_out_port ().clear_buffer (nframes);
    }

  auto process_plugin = [&] (auto &&pl_id) {
    if (pl_id.has_value ())
      {
        std::visit (
          [&] (auto &&plugin) { plugin->prepare_process (nframes); },
          pl_id->get_object ());
      }
  };

  std::ranges::for_each (inserts_, process_plugin);
  std::ranges::for_each (midi_fx_, process_plugin);
  process_plugin (instrument_);

  for (auto &send : sends_)
    {
      send->prepare_process (nframes);
    }

  if (track_->get_input_signal_type () == PortType::Event)
    {
      /* copy the cached MIDI events to the MIDI events in the MIDI in port */
      track_->processor_->get_midi_in_port ().midi_events_.dequeue (0, nframes);
    }
}

void
Channel::init_loaded ()
{
  z_debug ("initing channel");

  track_ = std::visit (
    [&] (auto &&track) { return dynamic_cast<ChannelTrack *> (track); },
    get_track_registry ().find_by_id_or_throw (track_uuid_.value ()));
  if (track_ == nullptr)
    {
      throw ZrythmException ("track not found");
    }

  /* fader */
  fader_->track_ = track_;

  fader_->init_loaded (
    port_registry_, param_registry_, track_, nullptr, nullptr);

  auto init_plugin = [&] (auto &plugin_id, PluginSlot slot) {
    if (plugin_id.has_value ())
      {
        auto plugin_var = plugin_id->get_object ();
        std::visit (
          [&] (auto &&plugin) {
            plugin->set_track (track_->get_uuid ());
            plugin->init_loaded ();
          },
          plugin_var);
      }
  };

  /* init plugins */
  for (int i = 0; i < (int) STRIP_SIZE; i++)
    {
      init_plugin (inserts_.at (i), PluginSlot (PluginSlotType::Insert, i));
      init_plugin (midi_fx_.at (i), PluginSlot (PluginSlotType::MidiFx, i));
    }
  init_plugin (instrument_, PluginSlot (PluginSlotType::Instrument));

  /* init sends */
  for (auto &send : sends_)
    {
      send->init_loaded (track_);
    }
}

#if 0
void
Channel::reconnect_ext_input_ports (engine::device_io::AudioEngine &engine)
{
  std::visit (
    [&] (auto &&track) {
      using TrackT = base_type<decltype (track)>;

      /* skip if auditioner track */
      if (track_->is_auditioner ())
        return;

      if (track_->disconnecting_)
        {
          z_error (
            "attempted to reconnect ext input ports on track {} while it is disconnecting",
            track_->get_name ());
          return;
        }

      // FIXME
      z_warning ("unimplemented!!!!!!!!!!!!!!!!!!!!!! FIXME!!!!");
      return;

      z_debug ("reconnecting ext inputs for {}", track_->get_name ());

      if constexpr (
        std::derived_from<TrackT, PianoRollTrack>
        || std::is_same_v<TrackT, ChordTrack>)
        {
          auto &midi_in = track->processor_->get_midi_in_port ();

          /* disconnect all connections to hardware */
          disconnect_port_hardware_inputs (midi_in);

          if (!ext_midi_ins_.has_value())
            {
              // TODO
#  if 0
              for (
                const auto &port_id :
                engine.hw_in_processor_->selected_midi_ports_)
                {
                  auto source = engine.hw_in_processor_->find_port (port_id);
                  if (!source)
                    {
                      z_warning ("port for {} not found", port_id);
                      continue;
                    }
                  PORT_CONNECTIONS_MGR->add_connection (
                    source->get_uuid (), midi_in.get_uuid (), 1.f, true, true);
                }
#  endif
            }
          else
            {
              for (const auto &ext_port : ext_midi_ins_)
                {
                  auto * source =
                    engine.hw_in_processor_->find_port (ext_port->get_id ());
                  if (source == nullptr)
                    {
                      z_warning ("port for {} not found", ext_port->get_id ());
                      continue;
                    }
                  PORT_CONNECTIONS_MGR->add_connection (
                    source->get_uuid (), midi_in.get_uuid (), 1.f, true, true);
                }
            }
        }
      else if constexpr (std::is_same_v<TrackT, AudioTrack>)
        {
          /* disconnect all connections to hardware */
          iterate_tuple (
            [&] (auto &port) { disconnect_port_hardware_inputs (port); },
            track_->processor_->get_stereo_in_ports ());

          auto &l = track_->processor_->get_stereo_in_ports ().first;
          auto &r = track_->processor_->get_stereo_in_ports ().second;
          if (all_stereo_l_ins_)
            {
              for (
                const auto &port_id :
                engine.hw_in_processor_->selected_audio_ports_)
                {
                  auto * source = engine.hw_in_processor_->find_port (port_id);
                  if (source == nullptr)
                    {
                      z_warning ("port for {} not found", port_id);
                      continue;
                    }
                  PORT_CONNECTIONS_MGR->add_connection (
                    source->get_uuid (), l.get_uuid (), 1.f, true, true);
                }
            }
          else
            {
              z_debug ("{} L HW ins", ext_stereo_l_ins_.size ());
              for (const auto &ext_port : ext_stereo_l_ins_)
                {
                  auto * source =
                    engine.hw_in_processor_->find_port (ext_port->get_id ());
                  if (source == nullptr)
                    {
                      z_warning ("port for {} not found", ext_port->get_id ());
                      continue;
                    }
                  PORT_CONNECTIONS_MGR->add_connection (
                    source->get_uuid (), l.get_uuid (), 1.f, true, true);
                }
            }

          if (all_stereo_r_ins_)
            {
              for (
                const auto &port_id :
                engine.hw_in_processor_->selected_audio_ports_)
                {
                  auto * source = engine.hw_in_processor_->find_port (port_id);
                  if (source == nullptr)
                    {
                      z_warning ("port for {} not found", port_id);
                      continue;
                    }
                  PORT_CONNECTIONS_MGR->add_connection (
                    source->get_uuid (), r.get_uuid (), 1.f, true, true);
                }
            }
          else
            {
              z_debug ("{} R HW ins", ext_stereo_r_ins_.size ());
              for (const auto &ext_port : ext_stereo_r_ins_)
                {
                  auto * source =
                    engine.hw_in_processor_->find_port (ext_port->get_id ());
                  if (source == nullptr)
                    {
                      z_warning ("port for {} not found", ext_port->get_id ());
                      continue;
                    }
                  PORT_CONNECTIONS_MGR->add_connection (
                    source->get_uuid (), r.get_uuid (), 1.f, true, true);
                }
            }
        }

      engine.router_->recalc_graph (false);
    },
    convert_to_variant<ChannelTrackPtrVariant> (this));
}
#endif

void
Channel::reset_fader (bool fire_events)
{
// TODO
#if 0
  fader_->set_amp_with_action (fader_->get_amp_port ().control_, 1.0f, true);

  if (fire_events)
    {
      // EVENTS_PUSH (EventType::ET_CHANNEL_FADER_VAL_CHANGED, this);
    }
#endif
}

void
Channel::paste_plugins_to_slot (PluginSpan plugins, PluginSlot slot)
{
  try
    {
      UNDO_MANAGER->perform (new gui::actions::MixerSelectionsPasteAction (
        plugins, *PORT_CONNECTIONS_MGR, &get_track (), slot));
    }
  catch (const ZrythmException &e)
    {
      e.handle (QObject::tr ("Failed to paste plugins"));
    }
}

GroupTargetTrack *
Channel::get_output_track () const
{
  if (!has_output ())
    return nullptr;

  z_return_val_if_fail (track_, nullptr);
  auto * tracklist = track_->get_tracklist ();
  auto   output_track_var = tracklist->get_track (output_track_uuid_.value ());
  z_return_val_if_fail (output_track_var, nullptr);

  return std::visit (
    [&] (auto &&output_track) -> GroupTargetTrack * {
      using TrackT = base_type<decltype (output_track)>;
      if constexpr (std::derived_from<TrackT, GroupTargetTrack>)
        {
          z_return_val_if_fail (track_ != output_track, nullptr);
          return output_track;
        }
      z_return_val_if_reached (nullptr);
    },
    *output_track_var);
}

void
Channel::init ()
{
  // init ports
  switch (track_->get_output_signal_type ())
    {
    case PortType::Audio:
      {
        auto stereo_outs = get_stereo_out_ports ();
        iterate_tuple (
          [&] (auto &&port) { port.set_full_designation_provider (this); },
          stereo_outs);
      }
      break;
    case PortType::Event:
      {
        auto &midi_out = get_midi_out_port ();
        midi_out.set_full_designation_provider (this);
      }
      break;
    default:
      break;
    }

  auto fader_type = track_->get_fader_type ();
  fader_ = utils::make_qobject_unique<Fader> (
    port_registry_, param_registry_, fader_type, track_, nullptr, nullptr);
  fader_->setParent (this);
  if (track_->get_output_signal_type () == PortType::Audio)
    {
      audio_prefader_ = utils::make_qobject_unique<AudioPreFader> (
        port_registry_, param_registry_, this);
    }
  else if (track_->get_output_signal_type () == PortType::Event)
    {
      midi_prefader_ = utils::make_qobject_unique<MidiPreFader> (
        port_registry_, param_registry_, this);
    }

  /* init sends */
  for (const auto &[i, send] : utils::views::enumerate (sends_))
    {
      send = std::make_unique<ChannelSend> (
        *track_, get_track_registry (), get_port_registry (), param_registry_,
        i);
    }
}

void
Channel::disconnect_port_hardware_inputs (dsp::Port &port)
{
// TODO
#if 0
  dsp::PortConnectionsManager::ConnectionsVector srcs;
  PORT_CONNECTIONS_MGR->get_sources_or_dests (&srcs, port.get_uuid (), true);
  for (const auto &conn : srcs)
    {
      auto src_port_var = PROJECT->find_port_by_id (conn->src_id_);
      z_return_if_fail (src_port_var.has_value ());
      std::visit (
        [&] (auto &&src_port) {
          if (
            src_port->id_->owner_type_
            == PortIdentifier::OwnerType::HardwareProcessor)
            PORT_CONNECTIONS_MGR->remove_connection (
              src_port->get_uuid (), port.get_uuid ());
        },
        src_port_var.value ());
    }
#endif
}

Channel::PluginPtrVariant
Channel::add_plugin (
  PluginUuidReference plugin_id,
  PluginSlot          slot,
  bool                confirm,
  bool                moving_plugin,
  bool                gen_automatables,
  bool                recalc_graph,
  bool                pub_events)
{
  bool prev_enabled = track_->enabled ();
  track_->set_enabled (false);

  auto slot_type =
    slot.has_slot_index ()
      ? slot.get_slot_with_index ().first
      : slot.get_slot_type_only ();
  auto slot_index =
    slot.has_slot_index () ? slot.get_slot_with_index ().second : -1;
  auto existing_pl_id =
    (slot_type == PluginSlotType::Instrument) ? instrument_
    : (slot_type == PluginSlotType::Insert)
      ? inserts_[slot_index]
      : midi_fx_[slot_index];

  /* free current plugin */
  if (existing_pl_id.has_value ())
    {
      z_debug ("existing plugin exists at {}:{}", track_->get_name (), slot);
      track_->remove_plugin (slot, moving_plugin, true);
    }

  auto plugin_var = plugin_id.get_object ();
  std::visit (
    [&] (auto &&plugin) {
      z_debug (
        "Inserting {} {} at {}:{}:{}", ENUM_NAME (slot_type),
        plugin->get_descriptor ().name_, track_->get_name (),
        ENUM_NAME (slot_type), slot);

      if (slot_type == PluginSlotType::Instrument)
        {
          instrument_ = plugin_id;
        }
      else if (slot_type == PluginSlotType::Insert)
        {
          inserts_[slot_index] = plugin_id;
        }
      else
        {
          midi_fx_[slot_index] = plugin_id;
        }

      plugin->set_track (track_->get_uuid ());

      track_->set_enabled (prev_enabled);

      if (gen_automatables)
        {
          track_->generate_automation_tracks_for_plugin (plugin->get_uuid ());
        }

      if (pub_events)
        {
          // EVENTS_PUSH (EventType::ET_PLUGIN_ADDED, plugin_ptr);
        }

      if (recalc_graph)
        {
          ROUTER->recalc_graph (false);
        }
    },
    plugin_var);
  return plugin_var;
}

std::optional<gui::old_dsp::plugins::PluginPtrVariant>
Channel::get_plugin_at_slot (PluginSlot slot) const
{
  auto slot_type =
    slot.has_slot_index ()
      ? slot.get_slot_with_index ().first
      : slot.get_slot_type_only ();
  auto slot_index =
    slot.has_slot_index () ? slot.get_slot_with_index ().second : -1;
  auto existing_pl_id = [&] () -> std::optional<PluginUuidReference> {
    switch (slot_type)
      {
      case PluginSlotType::Insert:
        return inserts_[slot_index];
      case PluginSlotType::MidiFx:
        return midi_fx_[slot_index];
      case PluginSlotType::Instrument:
        return instrument_;
      case PluginSlotType::Modulator:
      default:
        z_return_val_if_reached (std::nullopt);
      }
  }();
  if (!existing_pl_id.has_value ())
    {
      return std::nullopt;
    }
  return existing_pl_id->get_object ();
}

auto
Channel::get_plugin_slot (const PluginUuid &plugin_id) const -> PluginSlot
{
  if (instrument_ && plugin_id == instrument_->id ())
    {
      return PluginSlot{ PluginSlotType::Instrument };
    }
  {
    // note: for some reason `it` is not a pointer on msvc so `const auto * it`
    // won't work
    const auto it =
      std::ranges::find (inserts_, plugin_id, &PluginUuidReference::id);
    if (it != inserts_.end ())
      {
        return PluginSlot{
          PluginSlotType::Insert,
          static_cast<PluginSlot::SlotNo> (std::distance (inserts_.begin (), it))
        };
      }
  }
  {
    const auto it =
      std::ranges::find (midi_fx_, plugin_id, &PluginUuidReference::id);
    if (it != midi_fx_.end ())
      {
        return PluginSlot{
          PluginSlotType::MidiFx,
          static_cast<PluginSlot::SlotNo> (std::distance (midi_fx_.begin (), it))
        };
      }
  }

  throw std::runtime_error ("Plugin not found in channel");
}

struct PluginImportData
{
  Channel *                                 ch{};
  const Channel::Plugin *                   pl{};
  std::optional<PluginSpan>                 sel;
  const zrythm::plugins::PluginDescriptor * descr{};
  plugins::PluginSlot                       slot;
  bool                                      copy{};
  bool                                      ask_if_overwrite{};

  void do_import ()
  {
    bool plugin_valid = true;
    auto slot_type =
      this->slot.has_slot_index ()
        ? this->slot.get_slot_with_index ().first
        : this->slot.get_slot_type_only ();
    if (this->pl)
      {
        auto orig_track_var = this->pl->get_track ();

        /* if plugin at original position do nothing */
        auto do_nothing = std::visit (
          [&] (auto &&orig_track) {
            using TrackT = base_type<decltype (orig_track)>;
            if constexpr (std::derived_from<TrackT, ChannelTrack>)
              {
                if (
                  this->ch->track_ == orig_track
                  && this->slot
                       == this->ch->get_plugin_slot (this->pl->get_uuid ()))
                  return true;
              }
            return false;
          },
          orig_track_var.value ());
        if (do_nothing)
          return;

        if (
          Track::is_plugin_descriptor_valid_for_slot_type (
            *this->pl->setting_->get_descriptor (), slot_type,
            this->ch->track_->get_type ()))
          {
            try
              {
                if (this->copy)
                  {
                    UNDO_MANAGER->perform (
                      new gui::actions::MixerSelectionsCopyAction (
                        *this->sel, *PORT_CONNECTIONS_MGR, this->ch->track_,
                        this->slot));
                  }
                else
                  {
                    UNDO_MANAGER->perform (
                      new gui::actions::MixerSelectionsMoveAction (
                        *this->sel, *PORT_CONNECTIONS_MGR, this->ch->track_,
                        this->slot));
                  }
              }
            catch (const ZrythmException &e)
              {
                e.handle (QObject::tr ("Failed to move or copy plugins"));
                return;
              }
          }
        else
          {
            plugin_valid = false;
          }
      }
    else if (this->descr)
      {
        /* validate */
        if (Track::is_plugin_descriptor_valid_for_slot_type (
              *this->descr, slot_type, this->ch->track_->get_type ()))
          {
            auto setting =
              SETTINGS->plugin_settings_->create_configuration_for_descriptor (
                *this->descr);
            try
              {
                UNDO_MANAGER->perform (
                  new gui::actions::MixerSelectionsCreateAction (
                    *this->ch->track_, this->slot, *setting));
                SETTINGS->plugin_settings_
                  ->increment_num_instantiations_for_plugin (*this->descr);
              }
            catch (const ZrythmException &e)
              {
                e.handle (format_qstr (
                  QObject::tr ("Failed to create plugin {}"),
                  setting->get_name ()));
                return;
              }
          }
        else
          {
            plugin_valid = false;
          }
      }
    else
      {
        z_error ("No descriptor or plugin given");
        return;
      }

    if (!plugin_valid)
      {
        const auto &pl_descr =
          this->descr ? *this->descr : *this->pl->setting_->descr_;
        ZrythmException e (format_qstr (
          QObject::tr ("Channel::PluginBase {} cannot be added to this slot"),
          pl_descr.name_));
        e.handle (QObject::tr ("Failed to add plugin"));
      }
  }
};

#if 0
static void
overwrite_plugin_response_cb (
  AdwMessageDialog * dialog,
  char *             response,
  gpointer           user_data)
{
  auto * data = static_cast<PluginImportData *> (user_data);
  if (!string_is_equal (response, "overwrite"))
    {
      return;
    }

  do_import (data);
}
#endif

void
Channel::handle_plugin_import (
  const Channel::Plugin *   pl,
  std::optional<PluginSpan> plugins,
  const PluginDescriptor *  descr,
  PluginSlot                slot,
  bool                      copy,
  bool                      ask_if_overwrite)
{
// TODO
#if 0
  auto data = new PluginImportData ();
  data->ch = this;
  data->sel = sel;
  data->descr = descr;
  data->slot = slot;
  data->copy = copy;
  data->pl = pl && plugins && !plugins->empty() ? sel->get_first_plugin () : nullptr;

  z_info ("handling plugin import on channel {}...", track_->get_name ());

  if (ask_if_overwrite)
    {
      bool show_dialog = false;
      if (pl)
        {
          for (size_t i = 0; i < sel->slots_.size (); i++)
            {
              if (slot.has_slot_index ())
                {
                  auto slot_type = slot.get_slot_with_index ().first;
                  auto slot_index = slot.get_slot_with_index ().second;
                  if (get_plugin_at_slot (
                        plugins::PluginSlot (slot_type, slot_index + i)))
                    {
                      show_dialog = true;
                      break;
                    }
                }
              else
                {
                  if (get_plugin_at_slot (slot))
                    {
                      show_dialog = true;
                      break;
                    }
                }
            }
        }
      else
        {
          if (get_plugin_at_slot (slot))
            {
              show_dialog = true;
            }
        }

      if (show_dialog)
        {
#  if 0
          auto dialog =
            dialogs_get_overwrite_plugin_dialog (GTK_WINDOW (MAIN_WINDOW));
          gtk_window_present (GTK_WINDOW (dialog));
          g_signal_connect_data (
            dialog, "response", G_CALLBACK (overwrite_plugin_response_cb), data,
            (GClosureNotify) plugin_import_data_free, G_CONNECT_DEFAULT);
#  endif
          return;
        }
    }

  data->do_import ();
  delete data;
#endif
}

void
Channel::select_all (PluginSlotType type, bool select)
{
  TRACKLIST->get_track_span ().deselect_all_plugins ();
  if (!select)
    return;

  switch (type)
    {
    case PluginSlotType::Insert:
      for (auto &insert : inserts_)
        {
          if (insert)
            {
              auto * pl = Plugin::from_variant (insert->get_object ());
              pl->set_selected (true);
            }
        }
      break;
    case PluginSlotType::MidiFx:
      for (auto &midi_fx : midi_fx_)
        {
          if (midi_fx)
            {
              auto * pl = Plugin::from_variant (midi_fx->get_object ());
              pl->set_selected (true);
            }
        }
      break;
    default:
      z_warning ("not implemented");
      break;
    }
}

void
Channel::set_caches ()
{
  std::vector<Channel::Plugin *> pls;
  Channel::get_plugins (pls);

  for (auto pl : pls)
    {
      pl->set_caches ();
    }
}

void
Channel::get_plugins (std::vector<Channel::Plugin *> &pls)
{
  std::vector<PluginUuid> uuids;
  for (auto &insert : inserts_)
    {
      if (insert)
        {
          uuids.push_back (insert->id ());
        }
    }
  for (auto &midi_fx : midi_fx_)
    {
      if (midi_fx)
        {
          uuids.push_back (midi_fx->id ());
        }
    }
  if (instrument_)
    {
      uuids.push_back (instrument_->id ());
    }

  std::ranges::transform (uuids, std::back_inserter (pls), [&] (const auto &uuid) {
    return Plugin::from_variant (*get_plugin_from_id (uuid));
  });
}

void
from_json (const nlohmann::json &j, Channel &c)
{
  // TODO
  // j.at (Channel::kMidiFxKey).get_to (c.midi_fx_);
  // j.at (Channel::kInsertsKey).get_to (c.inserts_);
  for (
    const auto &[index, send_json] :
    utils::views::enumerate (j.at (Channel::kSendsKey)))
    {
      if (send_json.is_null ())
        continue;
      auto send = std::make_unique<ChannelSend> (
        *c.track_, c.track_registry_, c.port_registry_, c.param_registry_);
      from_json (send_json, *send);
      c.sends_.at (index) = std::move (send);
    }
  if (j.contains (Channel::kInstrumentKey))
    {
      c.instrument_ = { c.plugin_registry_ };
      j.at (Channel::kInstrumentKey).get_to (*c.instrument_);
    }
  // TODO: prefaders & fader
  // c.prefader_ = utils::make_qobject_unique<Fader> (&c);
  // j.at (Channel::kPrefaderKey).get_to (*c.prefader_);
  // c.fader_ = utils::make_qobject_unique<Fader> (&c);
  j.at (Channel::kFaderKey).get_to (*c.fader_);
  if (j.contains (Channel::kMidiOutKey))
    {
      c.midi_out_id_ = { c.port_registry_ };
      j.at (Channel::kMidiOutKey).get_to (*c.midi_out_id_);
    }
  if (j.contains (Channel::kStereoOutLKey))
    {
      c.stereo_out_left_id_ = { c.port_registry_ };
      j.at (Channel::kStereoOutLKey).get_to (*c.stereo_out_left_id_);
    }
  if (j.contains (Channel::kStereoOutRKey))
    {
      c.stereo_out_right_id_ = { c.port_registry_ };
      j.at (Channel::kStereoOutRKey).get_to (*c.stereo_out_right_id_);
    }
  j.at (Channel::kOutputIdKey).get_to (c.output_track_uuid_);
  j.at (Channel::kTrackIdKey).get_to (c.track_uuid_);
  if (j.contains (Channel::kExtMidiInputsKey))
    {
      j.at (Channel::kExtMidiInputsKey).get_to (c.ext_midi_ins_);
    }
  if (j.contains (Channel::kMidiChannelsKey))
    {
      j.at (Channel::kMidiChannelsKey).get_to (c.midi_channels_);
    }
  if (j.contains (Channel::kExtStereoLInputsKey))
    {
      j.at (Channel::kExtStereoLInputsKey).get_to (c.ext_stereo_l_ins_);
    }
  if (j.contains (Channel::kExtStereoRInputsKey))
    {
      j.at (Channel::kExtStereoRInputsKey).get_to (c.ext_stereo_r_ins_);
    }
  j.at (Channel::kWidthKey).get_to (c.width_);
}

};
