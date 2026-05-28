// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstdint>

#include "structure/arrangement/arranger_object.h"

#include <nlohmann/json_fwd.hpp>

namespace zrythm::structure::arrangement
{

class MidiControlEvent final : public ArrangerObject
{
  Q_OBJECT
  Q_PROPERTY (
    EventType controlType READ controlType WRITE setControlType NOTIFY
      controlTypeChanged)
  Q_PROPERTY (int channel READ channel WRITE setChannel NOTIFY channelChanged)
  Q_PROPERTY (
    int controller READ controller WRITE setController NOTIFY controllerChanged)
  Q_PROPERTY (int value READ value WRITE setValue NOTIFY valueChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  enum class EventType : std::uint8_t
  {
    ControlChange,
    PitchBend,
    ChannelPressure,
    PolyKeyPressure,
    ProgramChange,
  };
  Q_ENUM (EventType)

  MidiControlEvent (const dsp::TempoMap &tempo_map, QObject * parent = nullptr);
  Q_DISABLE_COPY_MOVE (MidiControlEvent)
  ~MidiControlEvent () override;

  static constexpr int kMaxChannel = 15;
  static constexpr int kMaxController = 127;
  static constexpr int kMaxValue7Bit = 127;
  static constexpr int kMaxValuePitchBend = 16383;

  EventType     controlType () const;
  void          setControlType (EventType type);
  Q_SIGNAL void controlTypeChanged (EventType);

  int           channel () const { return static_cast<int> (channel_); }
  void          setChannel (int ch);
  Q_SIGNAL void channelChanged (int);

  int           controller () const { return static_cast<int> (controller_); }
  void          setController (int ctrl);
  Q_SIGNAL void controllerChanged (int);

  int           value () const { return static_cast<int> (value_); }
  void          setValue (int val);
  Q_SIGNAL void valueChanged (int);

  EventType     controlEventType () const { return type_; }
  std::uint8_t  midiChannel () const { return channel_; }
  std::uint8_t  midiController () const { return controller_; }
  std::uint16_t midiValue () const { return value_; }

private:
  int maxValueForType () const;

  friend void init_from (
    MidiControlEvent       &obj,
    const MidiControlEvent &other,
    utils::ObjectCloneType  clone_type);

  static constexpr auto kTypeKey = "type"sv;
  static constexpr auto kChannelKey = "channel"sv;
  static constexpr auto kControllerKey = "controller"sv;
  static constexpr auto kValueKey = "value"sv;
  friend void           to_json (nlohmann::json &j, const MidiControlEvent &ev);
  friend void from_json (const nlohmann::json &j, MidiControlEvent &ev);

  EventType     type_{ EventType::ControlChange };
  std::uint8_t  channel_{ 0 };
  std::uint8_t  controller_{ 0 };
  std::uint16_t value_{ 0 };

  BOOST_DESCRIBE_CLASS (
    MidiControlEvent,
    (ArrangerObject),
    (),
    (),
    (type_, channel_, controller_, value_))
};

}
