// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/tempo_map_qml_adapter.h"
#include "structure/arrangement/arranger_object.h"
#include "utils/icloneable.h"

namespace zrythm::structure::arrangement
{

class TempoObject final : public ArrangerObject
{
  Q_OBJECT
  Q_PROPERTY (double tempo READ tempo WRITE setTempo NOTIFY tempoChanged)
  Q_PROPERTY (
    zrythm::dsp::TempoEventWrapper::CurveType curve READ curve WRITE setCurve
      NOTIFY curveChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  using CurveType = dsp::TempoEventWrapper::CurveType;
  static constexpr double DEFAULT_TEMPO = 120.0;

public:
  TempoObject (const dsp::TempoMap &tempo_map, QObject * parent = nullptr);

  // =========================================================
  // QML Interface
  // =========================================================

  double        tempo () const { return tempo_; }
  void          setTempo (double tempo);
  Q_SIGNAL void tempoChanged (double tempo);

  CurveType     curve () const { return curve_; }
  void          setCurve (CurveType curve);
  Q_SIGNAL void curveChanged (CurveType curve);

  // =========================================================

private:
  friend void init_from (
    TempoObject           &obj,
    const TempoObject     &other,
    utils::ObjectCloneType clone_type);

  static constexpr auto kTempoKey = "tempo"sv;
  static constexpr auto kCurveTypeKey = "curveType"sv;
  friend void           to_json (nlohmann::json &j, const TempoObject &so);
  friend void           from_json (const nlohmann::json &j, TempoObject &so);

private:
  double    tempo_{ DEFAULT_TEMPO };
  CurveType curve_{ CurveType::Constant };

  BOOST_DESCRIBE_CLASS (TempoObject, (ArrangerObject), (), (), (tempo_))
};

} // namespace zrythm::structure::arrangement
