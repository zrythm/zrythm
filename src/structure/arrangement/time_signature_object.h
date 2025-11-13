// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/arranger_object.h"
#include "utils/icloneable.h"

namespace zrythm::structure::arrangement
{

class TimeSignatureObject final : public ArrangerObject
{
  Q_OBJECT
  Q_PROPERTY (
    int numerator READ numerator WRITE setNumerator NOTIFY numeratorChanged)
  Q_PROPERTY (
    int denominator READ denominator WRITE setDenominator NOTIFY
      denominatorChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  static constexpr int DEFAULT_NUMERATOR = 4;
  static constexpr int DEFAULT_DENOMINATOR = 4;

  TimeSignatureObject (
    const dsp::TempoMap &tempo_map,
    QObject *            parent = nullptr);

  // =========================================================
  // QML Interface
  // =========================================================

  int           numerator () const { return numerator_; }
  void          setNumerator (int numerator);
  Q_SIGNAL void numeratorChanged (int numerator);

  int           denominator () const { return denominator_; }
  void          setDenominator (int denominator);
  Q_SIGNAL void denominatorChanged (int denominator);

  // =========================================================

private:
  friend void init_from (
    TimeSignatureObject       &obj,
    const TimeSignatureObject &other,
    utils::ObjectCloneType     clone_type);

  static constexpr auto kNumeratorKey = "numerator"sv;
  static constexpr auto kDenominatorKey = "denominator"sv;
  friend void to_json (nlohmann::json &j, const TimeSignatureObject &so);
  friend void from_json (const nlohmann::json &j, TimeSignatureObject &so);

private:
  int numerator_{ DEFAULT_NUMERATOR };
  int denominator_{ DEFAULT_DENOMINATOR };

  BOOST_DESCRIBE_CLASS (
    TimeSignatureObject,
    (ArrangerObject),
    (),
    (),
    (numerator_, denominator_))
};

} // namespace zrythm::structure::arrangement
