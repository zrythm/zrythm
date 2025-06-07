// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/port_identifier.h"
#include "gui/backend/backend/actions/undoable_action.h"
#include "utils/icloneable.h"
#include "utils/types.h"

namespace zrythm::gui::actions
{

/**
 * MIDI mapping action.
 */
class MidiMappingAction final : public QObject, public UndoableAction
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_UNDOABLE_ACTION_QML_PROPERTIES (MidiMappingAction)

public:
  enum class Type
  {
    Bind,
    Unbind,
    Enable,
    Disable,
  };

public:
  MidiMappingAction (QObject * parent = nullptr);

  /**
   * @brief Create a new action for enabling/disabling a MIDI mapping.
   *
   * @param enable
   */
  MidiMappingAction (int idx_to_enable_or_disable, bool enable);

  /**
   * @brief Construct a new action for unbinding a MIDI mapping.
   */
  MidiMappingAction (int idx_to_unbind);

  /**
   * @brief Construct a new action for binding a MIDI mapping.
   *
   * @param buf
   * @param device_id
   * @param dest_port
   */
  MidiMappingAction (
    std::array<midi_byte_t, 3>       buf,
    std::optional<utils::Utf8String> device_id,
    const Port                      &dest_port);

  QString to_string () const override;

  friend void init_from (
    MidiMappingAction       &obj,
    const MidiMappingAction &other,
    utils::ObjectCloneType   clone_type);

private:
  void init_loaded_impl () override { }
  void perform_impl () override;
  void undo_impl () override;

  void bind_or_unbind (bool bind);

public:
  /** Index of mapping, if enable/disable. */
  int idx_ = -1;

  /** Action type. */
  Type type_ = (Type) 0;

  std::optional<dsp::PortIdentifier::PortUuid> dest_port_id_;

  std::optional<utils::Utf8String> dev_id_;

  std::array<midi_byte_t, 3> buf_{};
};

}; // namespace zrythm::gui::actions
