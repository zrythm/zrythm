// SPDX-FileCopyrightText: © 2019-2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <string_view>
#include <vector>

#include "dsp/position.h"
#include "dsp/tempo_map_qml_adapter.h"
#include "structure/arrangement/arranger_object_fwd.h"
#include "structure/arrangement/colored_object.h"
#include "structure/arrangement/muteable_object.h"
#include "structure/arrangement/named_object.h"
#include "utils/enum_utils.h"
#include "utils/typed_uuid_reference.h"
#include "utils/units.h"

#include <QtQmlIntegration/qqmlintegration.h>

using std::literals::string_view_literals::operator""sv;

namespace zrythm::structure::arrangement
{

class ArrangerObjectListModel;
/**
 * @brief Base class for all objects in the arranger.
 *
 * The ArrangerObject class is the base class for all objects that can be
 * placed in the arranger, such as clips, MIDI notes, chord objects, etc.
 * It provides common functionality and properties shared by all these
 * objects.
 */
class ArrangerObject : public utils::UuidIdentifiableObject<ArrangerObject>
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    zrythm::structure::arrangement::ArrangerObject * parentObject READ
      parentObject WRITE setParentObject NOTIFY parentObjectChanged)
  Q_PROPERTY (
    zrythm::structure::arrangement::ArrangerObject::Type type READ type CONSTANT)
  Q_PROPERTY (zrythm::dsp::Position * position READ position CONSTANT)
  Q_PROPERTY (zrythm::dsp::ContentPosition * length READ length CONSTANT)
  Q_PROPERTY (
    zrythm::structure::arrangement::ArrangerObjectName * name READ name CONSTANT)
  Q_PROPERTY (
    zrythm::structure::arrangement::ArrangerObjectColor * color READ color
      CONSTANT)
  Q_PROPERTY (
    zrythm::structure::arrangement::ArrangerObjectMuteFunctionality * mute READ
      mute CONSTANT)
  QML_UNCREATABLE ("")

public:
  /**
   * The type of the object.
   */
  enum class Type : std::uint8_t
  {
    MidiClip,
    AudioClip,
    ChordClip,
    AutomationClip,
    MidiControlEvent,
    MidiNote,
    ChordObject,
    ScaleObject,
    Marker,
    AutomationPoint,
    AudioSourceObject,
    TempoObject,
    TimeSignatureObject
  };
  Q_ENUM (Type)

public:
  ~ArrangerObject () noexcept override;
  Q_DISABLE_COPY_MOVE (ArrangerObject)

  // ========================================================================
  // QML Interface
  // ========================================================================

  auto type () const { return type_; }

  virtual dsp::Position * position () const { return position_.get (); }

  dsp::ContentPosition *            length () const { return length_.get (); }
  ArrangerObjectName *              name () const { return name_.get (); }
  ArrangerObjectColor *             color () const { return color_.get (); }
  ArrangerObjectMuteFunctionality * mute () const { return mute_.get (); }

  /**
   * @brief Emitted when any of the properties of the object changed.
   */
  Q_SIGNAL void propertiesChanged ();

  ArrangerObject * parentObject () const { return parent_object_; }
  void             setParentObject (ArrangerObject * object);
  Q_SIGNAL void    parentObjectChanged (QObject * parentObject);

  // ========================================================================

  // Convenience getter
  const dsp::TempoMap        &get_tempo_map () const;
  const dsp::TempoMapWrapper &get_tempo_map_wrapper () const
  {
    return tempo_map_wrapper_;
  }

  virtual std::vector<ArrangerObjectListModel *> get_child_list_models () const
  {
    return {};
  }

protected:
  enum class ArrangerObjectFeatures : std::uint8_t
  {
    Bounds = 1 << 0,
    Name = 1 << 1,
    Color = 1 << 2,
    Mute = 1 << 3,
    ClipOwned = 1 << 4,
  };

  /**
   * @brief Construct a new ArrangerObject.
   *
   * @param tempo_map
   * @param derived The derived class instance, to be used for parenting any
   * QObjects created by this class.
   */
  ArrangerObject (
    Type                        type,
    const dsp::TempoMapWrapper &tempo_map_wrapper,
    ArrangerObjectFeatures      features,
    QObject *                   parent = nullptr) noexcept;

  friend void init_from (
    ArrangerObject        &obj,
    const ArrangerObject  &other,
    utils::ObjectCloneType clone_type);

private:
  static constexpr auto kPositionKey = "position"sv;
  static constexpr auto kLengthKey = "length"sv;
  static constexpr auto kNameKey = "name"sv;
  static constexpr auto kColorKey = "color"sv;
  static constexpr auto kMuteKey = "mute"sv;
  friend void
  to_json (nlohmann::json &j, const ArrangerObject &arranger_object);
  friend void
  from_json (const nlohmann::json &j, ArrangerObject &arranger_object);

private:
  Type type_;

  const dsp::TempoMapWrapper &tempo_map_wrapper_;

  /// Start position (always in Musical mode (ticks)).
  utils::QObjectUniquePtr<dsp::Position> position_;

  utils::QObjectUniquePtr<dsp::ContentPosition>            length_;
  utils::QObjectUniquePtr<ArrangerObjectName>              name_;
  utils::QObjectUniquePtr<ArrangerObjectColor>             color_;
  utils::QObjectUniquePtr<ArrangerObjectMuteFunctionality> mute_;

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
    (type_, length_, mute_, color_, name_))
};

using ArrangerObjectUuidReference = utils::TypedUuidReference<ArrangerObject>;

template <typename T>
concept FinalArrangerObjectSubclass =
  std::derived_from<T, ArrangerObject> && utils::CompleteType<T>;
}
