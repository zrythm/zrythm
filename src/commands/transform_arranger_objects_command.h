// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include "dsp/curve.h"
#include "dsp/tick_types.h"
#include "structure/arrangement/arranger_object.h"

#include <QUndoCommand>

namespace zrythm::commands
{

/**
 * @brief The state of a clip-owned object an editor function reads and
 * writes, in the object's own position space (content ticks).
 *
 * Only the fields relevant to the object type carry meaning: pitch and
 * velocity for MIDI notes, automation value and curve options for
 * automation points.
 */
struct TransformableObjectState
{
  dsp::ContentTick             position_ticks_{};
  dsp::ContentTick             length_ticks_{};
  int                          pitch_ = 0;
  int                          velocity_ = 0;
  float                        automation_value_ = 0.F;
  dsp::CurveOptions::Algorithm curve_algorithm_{};
  double                       curve_curviness_ = 0;

  bool operator== (const TransformableObjectState &) const = default;
};

/**
 * @brief Writes absolute before/after snapshots of clip-owned objects
 * produced by an editor function.
 *
 * The constructor captures each object's current state as the before
 * snapshot; redo() writes the after states passed at construction and
 * undo() restores the captured states. Both directions write absolute
 * values, so repeated undo/redo cycles are idempotent.
 */
class TransformArrangerObjectsCommand : public QUndoCommand
{
public:
  /**
   * @param objects Objects to transform, in the same order as their states.
   * @param after_states Target state of each object.
   * @param text Undo-stack text for the transformation.
   *
   * @pre objects.size() == after_states.size().
   */
  TransformArrangerObjectsCommand (
    std::vector<structure::arrangement::ArrangerObjectUuidReference> objects,
    std::vector<TransformableObjectState> after_states,
    const QString                        &text);

  void undo () override;
  void redo () override;

private:
  std::vector<structure::arrangement::ArrangerObjectUuidReference> objects_;
  std::vector<TransformableObjectState> before_states_;
  std::vector<TransformableObjectState> after_states_;
};

} // namespace zrythm::commands
