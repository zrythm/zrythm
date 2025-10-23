// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/atomic_position_qml_adapter.h"
#include "dsp/tempo_map.h"
#include "structure/arrangement/arranger_object_fwd.h"
#include "structure/arrangement/bounded_object.h"
#include "structure/arrangement/colored_object.h"
#include "structure/arrangement/fadeable_object.h"
#include "structure/arrangement/loopable_object.h"
#include "structure/arrangement/muteable_object.h"
#include "structure/arrangement/named_object.h"
#include "utils/types.h"
#include "utils/units.h"

#include <QtQmlIntegration>

namespace zrythm::structure::arrangement
{
/**
 * @brief Base class for all objects in the arranger.
 *
 * The ArrangerObject class is the base class for all objects that can be
 * placed in the arranger, such as regions, MIDI notes, chord objects, etc.
 * It provides common functionality and properties shared by all these
 * objects.
 */
class ArrangerObject
    : public QObject,
      public utils::UuidIdentifiableObject<ArrangerObject>
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    zrythm::structure::arrangement::ArrangerObject * parentObject READ
      parentObject WRITE setParentObject NOTIFY parentObjectChanged)
  Q_PROPERTY (
    zrythm::structure::arrangement::ArrangerObject::Type type READ type CONSTANT)
  Q_PROPERTY (
    zrythm::dsp::AtomicPositionQmlAdapter * position READ position CONSTANT)
  Q_PROPERTY (
    zrythm::structure::arrangement::ArrangerObjectBounds * bounds READ bounds
      CONSTANT)
  Q_PROPERTY (
    zrythm::structure::arrangement::ArrangerObjectLoopRange * loopRange READ
      loopRange CONSTANT)
  Q_PROPERTY (
    zrythm::structure::arrangement::ArrangerObjectName * name READ name CONSTANT)
  Q_PROPERTY (
    zrythm::structure::arrangement::ArrangerObjectColor * color READ color
      CONSTANT)
  Q_PROPERTY (
    zrythm::structure::arrangement::ArrangerObjectMuteFunctionality * mute READ
      mute CONSTANT)
  Q_PROPERTY (
    zrythm::structure::arrangement::ArrangerObjectFadeRange * fadeRange READ
      fadeRange CONSTANT)
  QML_UNCREATABLE ("")

public:
  /**
   * The type of the object.
   */
  enum class Type : basic_enum_base_type_t
  {
    MidiRegion,
    AudioRegion,
    ChordRegion,
    AutomationRegion,
    MidiNote,
    ChordObject,
    ScaleObject,
    Marker,
    AutomationPoint,
    AudioSourceObject,
  };
  Q_ENUM (Type)

public:
  ~ArrangerObject () noexcept override;
  Z_DISABLE_COPY_MOVE (ArrangerObject)

  /**
   * @brief @see @ref is_start_hit_by_range().
   */
  bool is_start_hit_by_range (
    const units::sample_t frames_start,
    const units::sample_t frames_end,
    bool                  range_start_inclusive = true,
    bool                  range_end_inclusive = false) const
  {
    const auto pos_samples = position_.get_samples ();
    return (range_start_inclusive
              ? (pos_samples >= frames_start)
              : (pos_samples > frames_start))
           && (range_end_inclusive ? (pos_samples <= frames_end) : (pos_samples < frames_end));
  }

  // ========================================================================
  // QML Interface
  // ========================================================================

  auto type () const { return type_; }

  dsp::AtomicPositionQmlAdapter * position () const
  {
    return position_adapter_.get ();
  }

  ArrangerObjectBounds *    bounds () const { return bounds_.get (); }
  ArrangerObjectLoopRange * loopRange () const { return loop_range_.get (); }
  ArrangerObjectName *      name () const { return name_.get (); }
  ArrangerObjectColor *     color () const { return color_.get (); }
  ArrangerObjectMuteFunctionality * mute () const { return mute_.get (); }
  ArrangerObjectFadeRange * fadeRange () const { return fade_range_.get (); }

  /**
   * @brief Emitted when any of the properties of the object changed.
   */
  Q_SIGNAL void propertiesChanged ();

  ArrangerObject * parentObject () const { return parent_object_; }
  void             setParentObject (ArrangerObject * object);
  Q_SIGNAL void    parentObjectChanged (QObject * parentObject);

  // ========================================================================

  // Convenience getter
  auto &get_tempo_map () const { return tempo_map_; }

protected:
  enum class ArrangerObjectFeatures : std::uint8_t
  {
    // individual bit positions
    Bounds = 1 << 0,
    LoopingBit = 1 << 1,
    Name = 1 << 2,
    Color = 1 << 3,
    Mute = 1 << 4,
    Fading = 1 << 5,

    // convenience masks
    Looping = LoopingBit | Bounds,
    Region = Looping | Name | Color | Mute,
  };

  /**
   * @brief Construct a new ArrangerObject.
   *
   * @param tempo_map
   * @param derived The derived class instance, to be used for parenting any
   * QObjects created by this class.
   */
  ArrangerObject (
    Type                   type,
    const dsp::TempoMap   &tempo_map,
    ArrangerObjectFeatures features,
    QObject *              parent = nullptr) noexcept;

  friend void init_from (
    ArrangerObject        &obj,
    const ArrangerObject  &other,
    utils::ObjectCloneType clone_type);

private:
  static constexpr auto kPositionKey = "position"sv;
  static constexpr auto kBoundsKey = "bounds"sv;
  static constexpr auto kLoopRangeKey = "loop_range"sv;
  static constexpr auto kFadeRangeKey = "fadeRange"sv;
  static constexpr auto kNameKey = "name"sv;
  static constexpr auto kColorKey = "color"sv;
  static constexpr auto kMuteKey = "mute"sv;
  friend void
  to_json (nlohmann::json &j, const ArrangerObject &arranger_object);
  friend void
  from_json (const nlohmann::json &j, ArrangerObject &arranger_object);

private:
  Type type_;

  const dsp::TempoMap &tempo_map_;

  /**
   * @brief Time conversion functions to be used by positions.
   *
   * @note The functions may be re-set (by the main thread) when (un)parenting
   * objects, so make sure no one is calling them simultaneously from another
   * thread during reparenting.
   */
  std::unique_ptr<dsp::AtomicPosition::TimeConversionFunctions>
    time_conversion_funcs_;

  /// Start position (always in Musical mode (ticks)).
  dsp::AtomicPosition                                    position_;
  utils::QObjectUniquePtr<dsp::AtomicPositionQmlAdapter> position_adapter_;

  utils::QObjectUniquePtr<ArrangerObjectBounds>            bounds_;
  utils::QObjectUniquePtr<ArrangerObjectLoopRange>         loop_range_;
  utils::QObjectUniquePtr<ArrangerObjectName>              name_;
  utils::QObjectUniquePtr<ArrangerObjectColor>             color_;
  utils::QObjectUniquePtr<ArrangerObjectMuteFunctionality> mute_;
  utils::QObjectUniquePtr<ArrangerObjectFadeRange>         fade_range_;

  /**
   * @brief Parent object, if any (for example the owning clip).
   *
   * This is only used at runtime.
   *
   * @note Only one level of parent-child relationships is supported.
   */
  QPointer<ArrangerObject> parent_object_;

  BOOST_DESCRIBE_CLASS (
    ArrangerObject,
    (UuidIdentifiableObject<ArrangerObject>),
    (),
    (),
    (type_, position_, bounds_, loop_range_, fade_range_, mute_, color_, name_))
};

using ArrangerObjectRegistry =
  utils::OwningObjectRegistry<ArrangerObjectPtrVariant, ArrangerObject>;
using ArrangerObjectUuidReference = utils::UuidReference<ArrangerObjectRegistry>;

template <typename T>
concept FinalArrangerObjectSubclass =
  std::derived_from<T, ArrangerObject> && CompleteType<T>;
}
