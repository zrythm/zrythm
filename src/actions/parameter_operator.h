// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <stdexcept>

#include "dsp/parameter.h"
#include "undo/undo_stack.h"

#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::actions
{
class ProcessorParameterOperator : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    zrythm::dsp::ProcessorParameter * processorParameter READ processorParameter
      WRITE setProcessorParameter NOTIFY processorParameterChanged)
  Q_PROPERTY (
    zrythm::undo::UndoStack * undoStack READ undoStack WRITE setUndoStack NOTIFY
      undoStackChanged)
  QML_ELEMENT

public:
  explicit ProcessorParameterOperator (QObject * parent = nullptr)
      : QObject (parent)
  {
  }

  Q_SIGNAL void processorParameterChanged ();
  Q_SIGNAL void undoStackChanged ();

  dsp::ProcessorParameter * processorParameter () const { return param_; }
  void setProcessorParameter (dsp::ProcessorParameter * param)
  {
    if (param == nullptr)
      {
        throw std::invalid_argument ("Param cannot be null");
      }
    if (param_ != param)
      {
        param_ = param;
        Q_EMIT processorParameterChanged ();
      }
  }

  undo::UndoStack * undoStack () const { return undo_stack_; }
  void              setUndoStack (undo::UndoStack * undoStack)
  {
    if (undoStack == nullptr)
      {
        throw std::invalid_argument ("UndoStack cannot be null");
      }
    if (undo_stack_ != undoStack)
      {
        undo_stack_ = undoStack;
        Q_EMIT undoStackChanged ();
      }
  }

  Q_INVOKABLE void setValue (float value);

private:
  dsp::ProcessorParameter * param_{};
  undo::UndoStack *         undo_stack_{};
};
}
