// SPDX-FileCopyrightText: © 2019-2022, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/atomic_position_qml_adapter.h"
#include "structure/arrangement/arranger_object_fwd.h"

#include <QtQmlIntegration>

#define DEFINE_ARRANGER_OBJECT_QML_PROPERTIES(ClassType) \
public: \
  Q_PROPERTY (ArrangerObject::Type type READ type CONSTANT) \
  Q_PROPERTY (bool selected READ getSelected NOTIFY selectedChanged) \
  Q_SIGNAL void selectedChanged (bool selected); \
  Q_PROPERTY (dsp::AtomicPositionQmlAdapter * position READ position CONSTANT) \
  Q_SIGNAL void addedToProject (); \
  Q_SIGNAL void removedFromProject ();

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
class ArrangerObject : public utils::UuidIdentifiableObject<ArrangerObject>
{
  Q_GADGET

  Z_DISABLE_COPY_MOVE (ArrangerObject)

public:
  using SelectionStatusGetter = std::function<bool (const Uuid &)>;

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
  ~ArrangerObject () noexcept override = default;

  /**
   * @brief @see @ref is_start_hit_by_range().
   */
  constexpr bool is_start_hit_by_range (
    const signed_frame_t frames_start,
    const signed_frame_t frames_end,
    bool                 range_start_inclusive = true,
    bool                 range_end_inclusive = false) const
  {
    const auto pos_samples = position_.get_samples ();
    return (range_start_inclusive
              ? (pos_samples >= frames_start)
              : (pos_samples > frames_start))
           && (range_end_inclusive ? (pos_samples <= frames_end) : (pos_samples < frames_end));
  }

  void set_selection_status_getter (SelectionStatusGetter getter)
  {
    selection_status_getter_ = getter;
  }
  void unset_selection_status_getter () { selection_status_getter_.reset (); }

  // ========================================================================
  // QML Interface
  // ========================================================================

  auto type () const { return type_; }

  bool getSelected () const
  {
    if (selection_status_getter_)
      {
        return (*selection_status_getter_) (get_uuid ());
      }
    return false;
  }
  dsp::AtomicPositionQmlAdapter * position () const
  {
    return position_adapter_.get ();
  }

  // ========================================================================

  // Convenience getter
  auto &get_tempo_map () const
  {
    return position ()->position ().get_tempo_map ();
  }

protected:
  /**
   * @brief Construct a new ArrangerObject.
   *
   * @param tempo_map
   * @param derived The derived class instance, to be used for parenting any
   * QObjects created by this class.
   */
  ArrangerObject (
    Type                 type,
    const dsp::TempoMap &tempo_map,
    QObject             &derived) noexcept;

  friend void init_from (
    ArrangerObject        &obj,
    const ArrangerObject  &other,
    utils::ObjectCloneType clone_type);

private:
  static constexpr auto kPositionKey = "position"sv;
  friend void
  to_json (nlohmann::json &j, const ArrangerObject &arranger_object);
  friend void
  from_json (const nlohmann::json &j, ArrangerObject &arranger_object);

private:
  Type type_;

  /// Start position (always in Musical mode (ticks)).
  dsp::AtomicPosition                                    position_;
  utils::QObjectUniquePtr<dsp::AtomicPositionQmlAdapter> position_adapter_{
    new dsp::AtomicPositionQmlAdapter{ position_ }
  };

protected:
  std::optional<SelectionStatusGetter> selection_status_getter_;

  BOOST_DESCRIBE_CLASS (
    ArrangerObject,
    (UuidIdentifiableObject<ArrangerObject>),
    (),
    (type_, position_),
    ())
};

using ArrangerObjectRegistry =
  utils::OwningObjectRegistry<ArrangerObjectPtrVariant, ArrangerObject>;
using ArrangerObjectUuidReference = utils::UuidReference<ArrangerObjectRegistry>;
using ArrangerObjectSelectionManager =
  utils::UuidIdentifiableObjectSelectionManager<ArrangerObjectRegistry>;

template <typename T>
concept FinalArrangerObjectSubclass =
  std::derived_from<T, ArrangerObject> && FinalClass<T> && CompleteType<T>;

}
