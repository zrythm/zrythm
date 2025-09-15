// SPDX-FileCopyrightText: © 2019-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object.h"
#include "utils/icloneable.h"

namespace zrythm::structure::arrangement
{

/**
 * Marker for the MarkerTrack.
 */
class Marker final : public ArrangerObject
{
  Q_OBJECT
  Q_PROPERTY (Marker::MarkerType markerType READ markerType CONSTANT)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  /**
   * Marker type.
   */
  enum class MarkerType : std::uint8_t
  {
    /** Song start Marker that cannot be deleted. */
    Start,
    /** Song end Marker that cannot be deleted. */
    End,
    /** Custom Marker. */
    Custom,
  };
  Q_ENUM (MarkerType)

  Marker (
    const dsp::TempoMap &tempo_map,
    MarkerType           type,
    QObject *            parent = nullptr);

  // ========================================================================
  // QML Interface
  // ========================================================================

  auto markerType () const { return marker_type_; }

  Q_INVOKABLE bool isStartMarker () const
  {
    return marker_type_ == MarkerType::Start;
  }
  Q_INVOKABLE bool isEndMarker () const
  {
    return marker_type_ == MarkerType::End;
  }

  // ========================================================================

private:
  friend void
  init_from (Marker &obj, const Marker &other, utils::ObjectCloneType clone_type);

  static constexpr auto kMarkerTypeKey = "markerType"sv;
  friend void           to_json (nlohmann::json &j, const Marker &m)
  {
    to_json (j, static_cast<const ArrangerObject &> (m));
    j[kMarkerTypeKey] = m.marker_type_;
  }
  friend void from_json (const nlohmann::json &j, Marker &m)
  {
    from_json (j, static_cast<ArrangerObject &> (m));
    j.at (kMarkerTypeKey).get_to (m.marker_type_);
  }

private:
  /** Marker type. */
  MarkerType marker_type_ = MarkerType::Custom;

  BOOST_DESCRIBE_CLASS (Marker, (ArrangerObject), (), (), (marker_type_))
};

} // namespace zrythm::structure::arrangement
