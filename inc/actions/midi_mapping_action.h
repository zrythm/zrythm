// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UNDO_MIDI_MAPPING_ACTION_H__
#define __UNDO_MIDI_MAPPING_ACTION_H__

#include "actions/undoable_action.h"
#include "dsp/ext_port.h"
#include "dsp/port_identifier.h"
#include "utils/icloneable.h"
#include "utils/types.h"

/**
 * @addtogroup actions
 *
 * @{
 */

/**
 * MIDI mapping action.
 */
class MidiMappingAction final
    : public UndoableAction,
      public ICloneable<MidiMappingAction>,
      public ISerializable<MidiMappingAction>
{
public:
  enum class Type
  {
    Bind,
    Unbind,
    Enable,
    Disable,
  };

public:
  MidiMappingAction () : UndoableAction (UndoableAction::Type::MidiMapping) { }

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
   * @param device_port
   * @param dest_port
   */
  MidiMappingAction (
    const std::array<midi_byte_t, 3> buf,
    const ExtPort *                  device_port,
    const Port                      &dest_port);

  std::string to_string () const override;

  void init_after_cloning (const MidiMappingAction &other) override;

  DECLARE_DEFINE_FIELDS_METHOD ();

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

  std::unique_ptr<PortIdentifier> dest_port_id_;

  std::unique_ptr<ExtPort> dev_port_;

  std::array<midi_byte_t, 3> buf_;
};

/**
 * @}
 */

#endif
