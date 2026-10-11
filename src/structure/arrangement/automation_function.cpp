// SPDX-FileCopyrightText: © 2020, 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <optional>

#include "structure/arrangement/automation_function.h"
#include "structure/arrangement/automation_point.h"
#include "utils/logger.h"

namespace zrythm::structure::arrangement
{

namespace
{

/// Captures the current state of each point.
std::vector<AutomationFunction::PointState>
capture_states (std::span<AutomationPoint * const> points)
{
  const auto state_of = [] (const AutomationPoint &point) {
    AutomationFunction::PointState state;
    state.position_ticks_ = point.position ()->asTick ();
    state.value_ = point.value ();
    state.curve_algorithm_ = point.curveOpts ()->algorithm ();
    state.curve_curviness_ = point.curveOpts ()->curviness ();
    return state;
  };

  std::vector<AutomationFunction::PointState> states;
  states.reserve (points.size ());
  for (const auto * point : points)
    {
      states.push_back (state_of (*point));
    }
  return states;
}

} // namespace

// --- FlipHorizontal ---

utils::Utf8String
AutomationFunction::FlipHorizontal::label (bool translate)
{
  return translate
           ? utils::Utf8String::from_qstring (QObject::tr ("Flip Horizontal"))
           : utils::Utf8String (u8"Flip Horizontal");
}

std::vector<EditorFunctionParam<AutomationFunction::FlipHorizontal>>
AutomationFunction::FlipHorizontal::params ()
{
  return {};
}

AutomationFunction::Result
AutomationFunction::FlipHorizontal::compute (
  std::span<AutomationPoint * const> points,
  const FlipHorizontal &)
{
  Result result;
  result.new_states_ = capture_states (points);
  if (points.empty ())
    return result;

  // The mirrored position of a point is its reflection across the middle
  // of the selection's span, so points trade places from the outside in
  const auto [first, last] = std::ranges::minmax_element (
    result.new_states_, {}, &PointState::position_ticks_);
  const dsp::ContentTick span_start = first->position_ticks_;
  const dsp::ContentTick span_end = last->position_ticks_;
  for (auto &state : result.new_states_)
    {
      state.position_ticks_ = span_start + (span_end - state.position_ticks_);
      state.curve_curviness_ = -state.curve_curviness_;
    }

  return result;
}

// --- FlipVertical ---

utils::Utf8String
AutomationFunction::FlipVertical::label (bool translate)
{
  return translate
           ? utils::Utf8String::from_qstring (QObject::tr ("Flip Vertical"))
           : utils::Utf8String (u8"Flip Vertical");
}

std::vector<EditorFunctionParam<AutomationFunction::FlipVertical>>
AutomationFunction::FlipVertical::params ()
{
  return {};
}

AutomationFunction::Result
AutomationFunction::FlipVertical::compute (
  std::span<AutomationPoint * const> points,
  const FlipVertical &)
{
  Result result;
  result.new_states_ = capture_states (points);
  if (points.empty ())
    return result;

  for (auto &state : result.new_states_)
    {
      state.value_ = 1.F - state.value_;
      state.curve_curviness_ = -state.curve_curviness_;
    }

  return result;
}

// --- Flatten ---

utils::Utf8String
AutomationFunction::Flatten::label (bool translate)
{
  return translate ? utils::Utf8String::from_qstring (QObject::tr ("Flatten"))
                   : utils::Utf8String (u8"Flatten");
}

std::vector<EditorFunctionParam<AutomationFunction::Flatten>>
AutomationFunction::Flatten::params ()
{
  return {};
}

AutomationFunction::Result
AutomationFunction::Flatten::compute (
  std::span<AutomationPoint * const> points,
  const Flatten &)
{
  Result result;
  result.new_states_ = capture_states (points);
  if (points.empty ())
    return result;

  for (auto &state : result.new_states_)
    {
      state.curve_algorithm_ = dsp::CurveOptions::Algorithm::Pulse;
      state.curve_curviness_ = 1.0;
    }

  return result;
}

// --- Generic dispatch ---

std::optional<AutomationFunction::Kind>
AutomationFunction::kind_from_id (std::string_view id)
{
  return detail::kind_from_id<Kind> (id);
}

utils::Utf8String
AutomationFunction::label (const Kind &kind, bool translate)
{
  return detail::label (kind, translate);
}

AutomationFunction::Result
AutomationFunction::compute (
  std::span<AutomationPoint * const> points,
  const Kind                        &kind)
{
  return detail::compute (points, kind);
}

void
AutomationFunction::apply (std::span<AutomationPoint *> sel, const Kind &kind)
{
  const auto result = compute (sel, kind);
  for (const auto &[point, state] : std::views::zip (sel, result.new_states_))
    {
      point->position ()->set_ticks (state.position_ticks_);
      point->setValue (state.value_);
      point->curveOpts ()->setAlgorithm (state.curve_algorithm_);
      point->curveOpts ()->setCurviness (state.curve_curviness_);
    }
}

} // namespace zrythm::structure::arrangement
