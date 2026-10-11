// SPDX-FileCopyrightText: © 2020, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <optional>
#include <span>
#include <string_view>
#include <variant>
#include <vector>

#include "dsp/curve.h"
#include "dsp/tick_types.h"
#include "structure/arrangement/editor_function_kind.h"
#include "structure/arrangement/editor_function_param.h"
#include "utils/utf8_string.h"

namespace zrythm::structure::arrangement
{
class AutomationPoint;

/**
 * @brief Transformative functions applicable to a selection of automation
 * points.
 *
 * Each function is a kind: a struct whose instance carries the
 * function's parameters, which also declares its parameters for the
 * generic function dialog and its computation. Kinds are pure
 * computations: compute() derives the target state of each point
 * without mutating anything, so the result can be captured in an
 * undoable command.
 */
class AutomationFunction
{
public:
  /// The state of an automation point a computed function result targets.
  struct PointState
  {
    dsp::ContentTick             position_ticks_{};
    float                        value_ = 0.F;
    dsp::CurveOptions::Algorithm curve_algorithm_{};
    double                       curve_curviness_ = 0;
  };

  /// The outcome of computing a function over a selection of points.
  struct Result
  {
    /// Target state of each input point, parallel to the input span.
    std::vector<PointState> new_states_;
  };

  /// Mirrors the positions of the points across the selection's span.
  struct FlipHorizontal
  {
    static constexpr std::string_view kId = "flip-horizontal";
    static constexpr int              kMinimumSelectionCount = 1;

    static utils::Utf8String label (bool translate = false);
    static std::vector<EditorFunctionParam<FlipHorizontal>> params ();
    static Result                                           compute (
      std::span<AutomationPoint * const> points,
      const FlipHorizontal              &params);
  };

  /// Inverts the values of the points around the middle of the
  /// normalized range.
  struct FlipVertical
  {
    static constexpr std::string_view kId = "flip-vertical";
    static constexpr int              kMinimumSelectionCount = 1;

    static utils::Utf8String label (bool translate = false);
    static std::vector<EditorFunctionParam<FlipVertical>> params ();
    static Result                                         compute (
      std::span<AutomationPoint * const> points,
      const FlipVertical                &params);
  };

  /// Replaces each curve with a step.
  struct Flatten
  {
    static constexpr std::string_view kId = "flatten";
    static constexpr int              kMinimumSelectionCount = 1;

    static utils::Utf8String label (bool translate = false);
    static std::vector<EditorFunctionParam<Flatten>> params ();
    static Result
    compute (std::span<AutomationPoint * const> points, const Flatten &params);
  };

  /// The available functions, each carrying its parameters.
  using Kind = std::variant<FlipHorizontal, FlipVertical, Flatten>;

  /**
   * Returns the function kind with the given id, if any.
   */
  static std::optional<Kind> kind_from_id (std::string_view id);

  /**
   * Returns the user-visible name of the function, translated when @p
   * translate is set.
   */
  static utils::Utf8String label (const Kind &kind, bool translate = false);

  /**
   * @brief Computes the target state of each point for the given function
   * kind, without mutating any point.
   *
   * FlipVertical inverts each value around the middle of the normalized
   * range and reverses each curve's direction. FlipHorizontal mirrors the
   * positions across the selection's span, from its first to its last
   * point, and likewise reverses each curve's direction. Flatten replaces
   * each curve with a step.
   *
   * @param points Points the function operates on.
   * @param kind Function kind with its parameters.
   */
  static Result
  compute (std::span<AutomationPoint * const> points, const Kind &kind);

  /**
   * @brief Writes the computed target state of the given function kind to
   * each point of @p sel.
   *
   * @param sel Selections to edit.
   * @param kind Function kind with its parameters.
   */
  static void apply (std::span<AutomationPoint *> sel, const Kind &kind);
};

} // namespace zrythm::structure::arrangement
