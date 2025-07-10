// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/backend/backend/actions/undoable_action.h"
#include "utils/icloneable.h"

namespace zrythm::gui::actions
{

class PortAction : public QObject, public UndoableAction
{
  Q_OBJECT
  QML_ELEMENT
  DEFINE_UNDOABLE_ACTION_QML_PROPERTIES (PortAction)

public:
  enum class Type
  {
    /** Set control port value. */
    SetControlValue,
  };

public:
  PortAction (QObject * parent = nullptr);
  ~PortAction () override = default;

  /**
   * @brief Construct a new action for setting a control.
   */
  PortAction (
    Type                          type,
    dsp::ProcessorParameter::Uuid port_id,
    float                         normalized_val);

  QString to_string () const override;

  friend void init_from (
    PortAction            &obj,
    const PortAction      &other,
    utils::ObjectCloneType clone_type);

private:
  void init_loaded_impl () override { }
  void perform_impl () override;
  void undo_impl () override;
  void do_or_undo (bool do_it);

public:
  Type type_ = Type::SetControlValue;

  std::optional<dsp::ProcessorParameter::Uuid> port_id_;

  /**
   * Normalized value before/after the change.
   *
   * To be swapped on undo/redo.
   */
  float normalized_val_ = 0.0f;
};

/**
 * @brief Action for resetting a control.
 */
class PortActionResetControl : public PortAction
{
public:
  PortActionResetControl (const dsp::ProcessorParameter &port)
      : PortAction (
          PortAction::Type::SetControlValue,
          port.get_uuid (),
          port.range ().convert_to_0_to_1 (port.range ().deff_))
  {
  }
};

}; // namespace zrythm::gui::actions
