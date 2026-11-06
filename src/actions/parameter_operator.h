// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/parameter.h"
#include "undo/undo_stack.h"
#include "utils/iobject_registry.h"

#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::actions
{

/**
 * @brief Pushes undoable edits of processor parameter values.
 *
 * Each edit names its parameter explicitly. Editable parameters must be
 * registered in the registry passed at construction (the project
 * registry); edits of parameters from other registries are refused.
 */
class ProcessorParameterOperator : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  ProcessorParameterOperator (
    undo::UndoStack        &undo_stack,
    utils::IObjectRegistry &registry,
    QObject *               parent = nullptr);

  /**
   * @brief Pushes an undoable edit of @p param's value.
   *
   * Consecutive edits of the same parameter within a short window merge
   * into a single undo step, so a control drag undoes as one action.
   */
  Q_INVOKABLE void setValue (dsp::ProcessorParameter * param, float value);

private:
  undo::UndoStack *        undo_stack_ = nullptr;
  utils::IObjectRegistry * registry_ = nullptr;
};
} // namespace zrythm::actions
