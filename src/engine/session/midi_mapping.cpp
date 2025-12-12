// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "dsp/midi_event.h"
#include "engine/session/midi_mapping.h"
#include "gui/backend/backend/zrythm.h"
#include "structure/project/project.h"
#include "utils/midi.h"
#include "utils/rt_thread_id.h"

namespace zrythm::engine::session
{
MidiMapping::MidiMapping (
  dsp::ProcessorParameterRegistry &param_registry,
  QObject *                        parent)
    : QObject (parent), param_registry_ (param_registry)
{
}

void
init_from (
  MidiMapping           &obj,
  const MidiMapping     &other,
  utils::ObjectCloneType clone_type)
{
  obj.key_ = other.key_;
  obj.device_id_ = other.device_id_;
  obj.dest_id_ = other.dest_id_;
  obj.enabled_.store (other.enabled_.load ());
}

MidiMappings::MidiMappings (dsp::ProcessorParameterRegistry &param_registry)
    : param_registry_ (param_registry)
{
}

void
MidiMappings::bind_at (
  std::array<midi_byte_t, 3>           buf,
  std::optional<utils::Utf8String>     device_id,
  dsp::ProcessorParameterUuidReference dest_port,
  int                                  idx,
  bool                                 fire_events)
{
  auto mapping = std::make_unique<MidiMapping> (param_registry_);
  mapping->key_ = buf;
  mapping->device_id_ = device_id;
  mapping->dest_id_ = dest_port;
  mapping->enabled_.store (true);

  mappings_.insert (mappings_.begin () + idx, std::move (mapping));

  auto str = utils::midi::midi_ctrl_change_get_description (buf);
  z_info (
    "bounded MIDI mapping from {} to {}", str,
    dest_port.get_object_as<dsp::ProcessorParameter> ()->label ());

  if (fire_events && ZRYTHM_HAVE_UI)
    {
      // EVENTS_PUSH (EventType::ET_MIDI_BINDINGS_CHANGED, nullptr);
    }
}

void
MidiMappings::unbind (int idx, bool fire_events)
{
  z_return_if_fail (idx >= 0 && idx < static_cast<int> (mappings_.size ()));

  mappings_.erase (mappings_.begin () + idx);

  if (fire_events && ZRYTHM_HAVE_UI)
    {
      // EVENTS_PUSH (EventType::ET_MIDI_BINDINGS_CHANGED, nullptr);
    }
}

int
MidiMappings::get_mapping_index (const MidiMapping &mapping) const
{
  auto it = std::ranges::find_if (mappings_, [&mapping] (const auto &m) {
    return m.get () == &mapping;
  });
  return it != mappings_.end () ? std::distance (mappings_.begin (), it) : -1;
}

void
MidiMapping::apply (std::array<midi_byte_t, 3> buf)
{
  auto * dest = dest_id_->get_object_as<dsp::ProcessorParameter> ();
  /* if toggle, reverse value */
  if (dest->range ().type_ == dsp::ParameterRange::Type::Toggle)
    {
      dest->setBaseValue (
        dest->range ().is_toggled (dest->baseValue ()) ? 0.f : 1.f);
    }
  /* else if not toggle set the control value received */
  else
    {
      float normalized_val = static_cast<float> (buf[2]) / 127.f;
      dest->setBaseValue (normalized_val);
    }
    // TODO port these from MidiPort's to parameters
#if 0
  else if (dest_->id_->type_ == dsp::PortType::Midi)
    {
      /* FIXME these are called during processing they should be queued as UI
       * events instead */
      if (ENUM_BITSET_TEST (
            dest_->id_->flags_, PortIdentifier::Flags::TransportRoll))
        {
          // EVENTS_PUSH (EventType::ET_TRANSPORT_ROLL_REQUIRED, nullptr);
        }
      else if (
        ENUM_BITSET_TEST (
          dest_->id_->flags_, PortIdentifier::Flags::TransportStop))
        {
          // EVENTS_PUSH (EventType::ET_TRANSPORT_PAUSE_REQUIRED, nullptr);
        }
      else if (
        ENUM_BITSET_TEST (
          dest_->id_->flags_, PortIdentifier::Flags::TransportBackward))
        {
          // EVENTS_PUSH (EventType::ET_TRANSPORT_MOVE_BACKWARD_REQUIRED,
          // nullptr);
        }
      else if (
        ENUM_BITSET_TEST (
          dest_->id_->flags_, PortIdentifier::Flags::TransportForward))
        {
          // EVENTS_PUSH (EventType::ET_TRANSPORT_MOVE_FORWARD_REQUIRED, nullptr);
        }
      else if (
        ENUM_BITSET_TEST (
          dest_->id_->flags_, PortIdentifier::Flags::TransportLoopToggle))
        {
          // EVENTS_PUSH (EventType::ET_TRANSPORT_TOGGLE_LOOP_REQUIRED, nullptr);
        }
      else if (
        ENUM_BITSET_TEST (
          dest_->id_->flags_, PortIdentifier::Flags::TransportRecToggle))
        {
          /* EVENTS_PUSH (
            EventType::ET_TRANSPORT_TOGGLE_RECORDING_REQUIRED, nullptr); */
        }
    }
#endif
}

void
MidiMappings::apply_from_cc_events (const dsp::MidiEventVector &events)
{
  for (const auto &ev : events)
    {
      if (utils::midi::midi_is_controller (ev.raw_buffer_))
        {
          auto channel = utils::midi::midi_get_channel_1_to_16 (ev.raw_buffer_);
          auto controller =
            utils::midi::midi_get_controller_number (ev.raw_buffer_);
          auto &mapping = mappings_[(channel - 1) * 128 + controller];
          mapping->apply (ev.raw_buffer_);
        }
    }
}

void
MidiMappings::apply (const midi_byte_t * buf)
{
  for (auto &mapping : mappings_)
    {
      if (
        mapping->enabled_.load () && mapping->key_[0] == buf[0]
        && mapping->key_[1] == buf[1])
        {
          std::array<midi_byte_t, 3> arr = { buf[0], buf[1], buf[2] };
          mapping->apply (arr);
        }
    }
}

int
MidiMappings::get_for_port (
  const dsp::ProcessorParameter &dest_port,
  std::vector<MidiMapping *> *   arr) const
{
  int count = 0;
  for (const auto &mapping : mappings_)
    {
      if (mapping->dest_id_->id () == dest_port.get_uuid ())
        {
          if (arr != nullptr)
            {
              arr->push_back (mapping.get ());
            }
          count++;
        }
    }
  return count;
}

void
from_json (const nlohmann::json &j, MidiMappings &mappings)
{
  for (const auto &mapping_json : j.at (MidiMappings::kMappingsKey))
    {
      auto mapping = std::make_unique<MidiMapping> (mappings.param_registry_);
      from_json (mapping_json, *mapping);
      mappings.mappings_.push_back (std::move (mapping));
    }
}
}
