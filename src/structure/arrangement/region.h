// SPDX-FileCopyrightText: © 2018-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/colored_object.h"
#include "structure/arrangement/loopable_object.h"
#include "structure/arrangement/muteable_object.h"
#include "structure/arrangement/named_object.h"

namespace zrythm::structure::arrangement
{

class RegionMixin : public QObject
{
  Q_OBJECT
  Q_PROPERTY (ArrangerObjectBounds * bounds READ bounds CONSTANT)
  Q_PROPERTY (ArrangerObjectLoopRange * loopRange READ loopRange CONSTANT)
  Q_PROPERTY (ArrangerObjectName * name READ name CONSTANT)
  Q_PROPERTY (ArrangerObjectColor * color READ color CONSTANT)
  Q_PROPERTY (ArrangerObjectMuteFunctionality * mute READ mute CONSTANT)
  QML_ELEMENT

public:
  RegionMixin (const dsp::AtomicPositionQmlAdapter &start_position);

  // ========================================================================
  // QML Interface
  // ========================================================================

  ArrangerObjectBounds *    bounds () const { return bounds_.get (); }
  ArrangerObjectLoopRange * loopRange () const { return loop_range_.get (); }
  ArrangerObjectName *      name () const { return name_.get (); }
  ArrangerObjectColor *     color () const { return color_.get (); }
  ArrangerObjectMuteFunctionality * mute () const { return mute_.get (); }

  // ========================================================================

protected:
  friend void init_from (
    RegionMixin           &obj,
    const RegionMixin     &other,
    utils::ObjectCloneType clone_type)
  {
    init_from (*obj.bounds_, *other.bounds_, clone_type);
    init_from (*obj.loop_range_, *other.loop_range_, clone_type);
    init_from (*obj.name_, *other.name_, clone_type);
    init_from (*obj.color_, *other.color_, clone_type);
    init_from (*obj.mute_, *other.mute_, clone_type);
  }

private:
  static constexpr auto kBoundsKey = "bounds"sv;
  static constexpr auto kLoopRangeKey = "loop_range"sv;
  static constexpr auto kNameKey = "name"sv;
  static constexpr auto kColorKey = "color"sv;
  static constexpr auto kMuteKey = "mute"sv;
  friend void           to_json (nlohmann::json &j, const RegionMixin &region)
  {
    j[kBoundsKey] = region.bounds_;
    j[kLoopRangeKey] = region.loop_range_;
    j[kNameKey] = region.name_;
    j[kColorKey] = region.color_;
    j[kMuteKey] = region.mute_;
  }
  friend void from_json (const nlohmann::json &j, RegionMixin &region)
  {
    j.at (kBoundsKey).get_to (*region.bounds_);
    j.at (kLoopRangeKey).get_to (*region.loop_range_);
    j.at (kNameKey).get_to (*region.name_);
    j.at (kColorKey).get_to (*region.color_);
    j.at (kMuteKey).get_to (*region.mute_);
  }

private:
  utils::QObjectUniquePtr<ArrangerObjectBounds>            bounds_;
  utils::QObjectUniquePtr<ArrangerObjectLoopRange>         loop_range_;
  utils::QObjectUniquePtr<ArrangerObjectName>              name_;
  utils::QObjectUniquePtr<ArrangerObjectColor>             color_;
  utils::QObjectUniquePtr<ArrangerObjectMuteFunctionality> mute_;

  BOOST_DESCRIBE_CLASS (
    RegionMixin,
    (),
    (),
    (),
    (bounds_, loop_range_, mute_, color_, name_))
};

} // namespace arrangement
