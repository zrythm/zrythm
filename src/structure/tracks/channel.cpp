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
  dsp::Fader::ShouldBeMutedCallback             should_be_muted_cb,
  QObject *                                     parent)
    : QObject (parent), dependencies_ (processor_dependencies),
      plugin_registry_ (plugin_registry),
      name_provider_ (std::move (name_provider)), signal_type_ (signal_type),
      hard_limit_fader_output_ (hard_limit_fader_output),
      midi_fx_ (
        utils::make_qobject_unique<plugins::PluginList> (plugin_registry, this)),
      inserts_ (
        utils::make_qobject_unique<plugins::PluginList> (plugin_registry, this)),
      fader_ (
        utils::make_qobject_unique<dsp::Fader> (
          dependencies (),
          signal_type_,
          hard_limit_fader_output_,
          true,
          name_provider_,
          should_be_muted_cb,
          this))
{

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
      // TODO
      // obj.midi_fx_ = other.midi_fx_;
      // obj.inserts_ = other.inserts_;
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
      ret = instrument_.value ();
      instrument_.reset ();
      Q_EMIT instrumentChanged (nullptr);
    }

  const auto check_plugin_list = [id, &ret] (auto &container) {
    auto it = std::ranges::find (
      container->plugins (), id, &plugins::PluginUuidReference::id);
    if (it != container->plugins ().end ())
      {
        ret = container->remove_plugin (it->id ());
      }
  };
  check_plugin_list (inserts_);
  check_plugin_list (midi_fx_);
  assert (ret.has_value ());

  std::visit (
    [&] (auto &&plugin) {
      z_debug ("Removing {} from {}", plugin->get_name (), name_provider_ ());
    },
    ret.value ().get_object ());

  return ret.value ();
}

void
Channel::get_plugins (std::vector<plugins::PluginPtrVariant> &plugins) const
{
  std::vector<plugins::PluginUuidReference> refs;
  std::ranges::copy (midi_fx_->plugins (), std::back_inserter (refs));
  std::ranges::copy (inserts_->plugins (), std::back_inserter (refs));
  if (instrument_)
    {
      refs.push_back (*instrument_);
    }

  std::ranges::transform (
    refs, std::back_inserter (plugins),
    &plugins::PluginUuidReference::get_object);
}

void
from_json (const nlohmann::json &j, Channel &c)
{
  j.at (Channel::kMidiFxKey).get_to (*c.midi_fx_);
  j.at (Channel::kInsertsKey).get_to (*c.inserts_);
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
