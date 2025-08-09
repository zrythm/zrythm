// SPDX-FileCopyrightText: © 2018-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "structure/tracks/channel.h"
#include "utils/views.h"

namespace zrythm::structure::tracks
{

ChannelMidiPassthroughProcessor::ChannelMidiPassthroughProcessor (
  dsp::ProcessorBase::ProcessorBaseDependencies dependencies,
  QObject *                                     parent)
    : QObject (parent), dsp::MidiPassthroughProcessor (dependencies)
{
  set_name (u8"MIDI PreFader");
}

ChannelAudioPassthroughProcessor::ChannelAudioPassthroughProcessor (
  dsp::ProcessorBase::ProcessorBaseDependencies dependencies,
  QObject *                                     parent)
    : QObject (parent), dsp::StereoPassthroughProcessor (dependencies)
{
  set_name (u8"Audio PreFader");
}

Channel::Channel (
  plugins::PluginRegistry                      &plugin_registry,
  dsp::ProcessorBase::ProcessorBaseDependencies processor_dependencies,
  dsp::PortType                                 signal_type,
  NameProvider                                  name_provider,
  bool                                          hard_limit_fader_output,
  Fader::ShouldBeMutedCallback                  should_be_muted_cb,
  QObject *                                     parent)
    : QObject (parent), dependencies_ (processor_dependencies),
      plugin_registry_ (plugin_registry),
      name_provider_ (std::move (name_provider)), signal_type_ (signal_type),
      hard_limit_fader_output_ (hard_limit_fader_output),
      should_be_muted_cb_ (std::move (should_be_muted_cb))
{
  fader_ = utils::make_qobject_unique<Fader> (
    dependencies (), signal_type_, hard_limit_fader_output_, true,
    name_provider_, should_be_muted_cb_, this);
  if (signal_type_ == PortType::Audio)
    {
      audio_prefader_ = utils::make_qobject_unique<
        ChannelAudioPassthroughProcessor> (dependencies (), this);
      audio_prefader_->set_name (u8"Audio Pre-Fader");
      audio_postfader_ = utils::make_qobject_unique<
        ChannelAudioPassthroughProcessor> (dependencies (), this);
      audio_postfader_->set_name (u8"Audio Post-Fader");
    }
  else if (signal_type_ == PortType::Midi)
    {
      midi_prefader_ = utils::make_qobject_unique<
        ChannelMidiPassthroughProcessor> (dependencies (), this);
      midi_prefader_->set_name (u8"MIDI Pre-Fader");
      midi_postfader_ = utils::make_qobject_unique<
        ChannelMidiPassthroughProcessor> (dependencies (), this);
      midi_postfader_->set_name (u8"MIDI Post-Fader");
    }

  /* init sends */
  prefader_sends_.emplace_back (
    utils::make_qobject_unique<ChannelSend> (
      dependencies (), signal_type_, 0, true, this));
  postfader_sends_.emplace_back (
    utils::make_qobject_unique<ChannelSend> (
      dependencies (), signal_type_, 0, false, this));
}

void
init_from (Channel &obj, const Channel &other, utils::ObjectCloneType clone_type)
{
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

plugins::PluginUuidReference
Channel::remove_plugin (plugins::Plugin::Uuid id)
{
  std::optional<plugins::PluginUuidReference> ret;
  if (instrument_.has_value () && instrument_->id () == id)
    {
      ret = instrument_;
      instrument_.reset ();
    }

  const auto id_projection =
    [] (const std::optional<plugins::PluginUuidReference> &ref) {
      return ref.has_value () ? ref->id () : plugins::Plugin::Uuid{};
    };
  const auto check_plugin_container =
    [id, id_projection, &ret] (auto &container) {
      auto it = std::ranges::find (container, id, id_projection);
      if (it != container.end ())
        {
          ret = it->value ();
          it->reset ();
        }
    };
  check_plugin_container (inserts_);
  check_plugin_container (midi_fx_);
  assert (ret.has_value ());

  std::visit (
    [&] (auto &&plugin) {
      z_debug ("Removing {} from {}", plugin->get_name (), name_provider_ ());
    },
    ret.value ().get_object ());

  return ret.value ();
}

std::optional<plugins::PluginUuidReference>
Channel::add_plugin (plugins::PluginUuidReference plugin_id, PluginSlot slot)
{
  std::optional<plugins::PluginUuidReference> ret;

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
      z_debug ("existing plugin exists at {}:{}", name_provider_ (), slot);
      ret = remove_plugin (existing_pl_id->id ());
    }

  auto plugin_var = plugin_id.get_object ();
  std::visit (
    [&] (auto &&plugin) {
      z_debug (
        "Inserting {} {} at {}:{}:{}", slot_type, plugin->get_name (),
        name_provider_ (), slot_type, slot);
    },
    plugin_var);

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

  return ret;
}

std::optional<plugins::PluginPtrVariant>
Channel::get_plugin_at_slot (PluginSlot slot) const
{
  auto slot_type =
    slot.has_slot_index ()
      ? slot.get_slot_with_index ().first
      : slot.get_slot_type_only ();
  auto slot_index =
    slot.has_slot_index () ? slot.get_slot_with_index ().second : -1;
  auto existing_pl_id = [&] () -> std::optional<plugins::PluginUuidReference> {
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
    const auto it = std::ranges::find (
      inserts_, plugin_id, &plugins::PluginUuidReference::id);
    if (it != inserts_.end ())
      {
        return PluginSlot{
          PluginSlotType::Insert,
          static_cast<PluginSlot::SlotNo> (std::distance (inserts_.begin (), it))
        };
      }
  }
  {
    const auto it = std::ranges::find (
      midi_fx_, plugin_id, &plugins::PluginUuidReference::id);
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

void
Channel::get_plugins (std::vector<Channel::Plugin *> &pls) const
{
  std::vector<plugins::PluginUuidReference> refs;
  for (const auto &insert : inserts_)
    {
      if (insert)
        {
          refs.push_back (*insert);
        }
    }
  for (const auto &midi_fx : midi_fx_)
    {
      if (midi_fx)
        {
          refs.push_back (*midi_fx);
        }
    }
  if (instrument_)
    {
      refs.push_back (*instrument_);
    }

  std::ranges::transform (refs, std::back_inserter (pls), [&] (const auto &ref) {
    return std::visit (
      [] (const auto &pl) -> plugins::Plugin * { return pl; },
      ref.get_object ());
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
    utils::views::enumerate (j.at (Channel::kPreFaderSendsKey)))
    {
      auto send = utils::make_qobject_unique<ChannelSend> (
        c.dependencies (), c.signal_type_, index, true);
      from_json (send_json, *send);
      c.prefader_sends_.at (index) = std::move (send);
    }
  for (
    const auto &[index, send_json] :
    utils::views::enumerate (j.at (Channel::kPostFaderSendsKey)))
    {
      auto send = utils::make_qobject_unique<ChannelSend> (
        c.dependencies (), c.signal_type_, index, false);
      from_json (send_json, *send);
      c.postfader_sends_.at (index) = std::move (send);
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
}

}
