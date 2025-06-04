// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "dsp/midi_event.h"
#include "engine/session/midi_mapping.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/control_port.h"
#include "utils/midi.h"
#include "utils/rt_thread_id.h"

namespace zrythm::engine::session
{
MidiMapping::MidiMapping (QObject * parent) : QObject (parent) { }

void
MidiMappings::init_loaded ()
{
  for (auto &mapping : mappings_)
    {
      auto dest_var = PROJECT->find_port_by_id (*mapping->dest_id_);
      z_return_if_fail (dest_var);
      std::visit ([&] (auto &&dest) { mapping->dest_ = dest; }, *dest_var);
    }
}

void
MidiMapping::init_after_cloning (
  const MidiMapping &other,
  ObjectCloneType    clone_type)
{
  key_ = other.key_;
  device_id_ = other.device_id_;
  dest_id_ = other.dest_id_;
  enabled_.store (other.enabled_.load ());
}

void
MidiMappings::bind_at (
  std::array<midi_byte_t, 3>       buf,
  std::optional<utils::Utf8String> device_id,
  Port                            &dest_port,
  int                              idx,
  bool                             fire_events)
{
  auto mapping = std::make_unique<MidiMapping> ();
  mapping->key_ = buf;
  mapping->device_id_ = device_id;
  mapping->dest_id_ = dest_port.get_uuid ();
  mapping->dest_ = &dest_port;
  mapping->enabled_.store (true);

  mappings_.insert (mappings_.begin () + idx, std::move (mapping));

  auto str = utils::midi::midi_ctrl_change_get_description (buf);
  if (
    (ENUM_BITSET_TEST (
      dest_port.id_->flags_, dsp::PortIdentifier::Flags::MidiAutomatable))
    == 0)
    {
      z_info ("bounded MIDI mapping from {} to {}", str, dest_port.get_label ());
    }

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
  z_return_if_fail (dest_);

  if (dest_->id_->type_ == dsp::PortType::Control)
    {
      auto * dest = dynamic_cast<ControlPort *> (dest_);
      /* if toggle, reverse value */
      if (ENUM_BITSET_TEST (dest->id_->flags_, PortIdentifier::Flags::Toggle))
        {
          dest->set_toggled (!dest->is_toggled (), true);
        }
      /* else if not toggle set the control value received */
      else
        {
          float normalized_val = static_cast<float> (buf[2]) / 127.f;
          dest->set_control_value (normalized_val, true, true);
        }
    }
  else if (dest_->id_->type_ == dsp::PortType::Event)
    {
      /* FIXME these are called during processing they should be queued as UI
       * events instead */
      if (ENUM_BITSET_TEST (
            dest_->id_->flags2_, PortIdentifier::Flags2::TransportRoll))
        {
          // EVENTS_PUSH (EventType::ET_TRANSPORT_ROLL_REQUIRED, nullptr);
        }
      else if (
        ENUM_BITSET_TEST (
          dest_->id_->flags2_, PortIdentifier::Flags2::TransportStop))
        {
          // EVENTS_PUSH (EventType::ET_TRANSPORT_PAUSE_REQUIRED, nullptr);
        }
      else if (
        ENUM_BITSET_TEST (
          dest_->id_->flags2_, PortIdentifier::Flags2::TransportBackward))
        {
          // EVENTS_PUSH (EventType::ET_TRANSPORT_MOVE_BACKWARD_REQUIRED,
          // nullptr);
        }
      else if (
        ENUM_BITSET_TEST (
          dest_->id_->flags2_, PortIdentifier::Flags2::TransportForward))
        {
          // EVENTS_PUSH (EventType::ET_TRANSPORT_MOVE_FORWARD_REQUIRED, nullptr);
        }
      else if (
        ENUM_BITSET_TEST (
          dest_->id_->flags2_, PortIdentifier::Flags2::TransportLoopToggle))
        {
          // EVENTS_PUSH (EventType::ET_TRANSPORT_TOGGLE_LOOP_REQUIRED, nullptr);
        }
      else if (
        ENUM_BITSET_TEST (
          dest_->id_->flags2_, PortIdentifier::Flags2::TransportRecToggle))
        {
          /* EVENTS_PUSH (
            EventType::ET_TRANSPORT_TOGGLE_RECORDING_REQUIRED, nullptr); */
        }
    }
}

void
MidiMappings::apply_from_cc_events (dsp::MidiEventVector &events)
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
  const Port                  &dest_port,
  std::vector<MidiMapping *> * arr) const
{
  int count = 0;
  for (const auto &mapping : mappings_)
    {
      if (mapping->dest_ == &dest_port)
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
  j.at (MidiMappings::kMappingsKey).get_to (mappings.mappings_);
}
}
