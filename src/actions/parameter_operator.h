// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/parameter.h"

#include <QtQmlIntegration/qqmlintegration.h>

#include "operations/operation_dispatcher.h"

namespace zrythm::actions
{

/**
 * @brief Pushes undoable edits of processor parameter values.
 *
 * All mutations are dispatched through the operations layer and are
 * undoable. Edits of parameters outside the project registry are
 * refused.
 */
class ProcessorParameterOperator : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  explicit ProcessorParameterOperator (
    operations::OperationDispatcher &operation_dispatcher,
    QObject *                        parent = nullptr);

  /**
   * @brief Pushes an undoable edit of @p param's base value.
   *
   * Consecutive edits of the same parameter within a short window merge
   * into a single undo step, so a control drag undoes as one action.
   * Setting the current value elides.
   *
   * @param value New base value in the normalized 0.0-1.0 range.
   * Callers must pass a finite value; non-finite values are not
   * validated.
   * @return Empty string on success, otherwise the refusal reason.
   */
  Q_INVOKABLE QString setValue (dsp::ProcessorParameter * param, qreal value);

private:
  operations::OperationDispatcher &operation_dispatcher_;
};
} // namespace zrythm::actions
