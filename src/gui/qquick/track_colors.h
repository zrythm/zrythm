// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <algorithm>
#include <utility>
#include <vector>

#include "structure/tracks/track.h"
#include "utils/color_math.h"

#include <QColor>
#include <QObject>
#include <QVariantList>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::gui::qquick
{

/**
 * @brief The track-type color table for the QML layer.
 *
 * Each track type has a fixed OKLCH hue at a shared lightness and
 * chroma; light-mode variants lower the lightness until they reach
 * the contrast target against the light page color. The reactive
 * per-track resolution (user color vs type default) lives in the
 * TrackColors QML singleton, which forwards to this table.
 */
class TrackColorTable : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_SINGLETON

public:
  /**
   * @brief The fixed hue of a track type, in degrees in [0, 360).
   */
  Q_INVOKABLE static double hueOf (structure::tracks::Track::Type type)
  {
    return hue_of_type (type);
  }

  /**
   * @brief The default color of a track type in the given theme mode.
   *
   * @param dark Whether the dark theme mode is active.
   */
  Q_INVOKABLE static QColor
  defaultColor (structure::tracks::Track::Type type, bool dark)
  {
    return utils::ColorMath::oklchColorWithContrast (
      hue_of_type (type), kBaseChroma, kBaseLightness, dark,
      kLightReferenceColor, kLightContrastTarget, kLightLightnessFloor);
  }

  /**
   * @brief All track types in hue order, for the color picker.
   */
  Q_INVOKABLE static QVariantList canonicalTypes ()
  {
    static const QVariantList types = [] {
      std::vector<structure::tracks::Track::Type> all = {
        structure::tracks::Track::Type::Master,
        structure::tracks::Track::Type::MidiGroup,
        structure::tracks::Track::Type::Instrument,
        structure::tracks::Track::Type::Midi,
        structure::tracks::Track::Type::MidiBus,
        structure::tracks::Track::Type::Audio,
        structure::tracks::Track::Type::AudioBus,
        structure::tracks::Track::Type::AudioGroup,
        structure::tracks::Track::Type::Folder,
        structure::tracks::Track::Type::Chord,
        structure::tracks::Track::Type::Modulator,
        structure::tracks::Track::Type::Marker,
      };
      std::ranges::sort (all, [] (auto a, auto b) {
        return hue_of_type (a) < hue_of_type (b);
      });
      QVariantList result;
      result.reserve (all.size ());
      for (auto type : all)
        result.push_back (static_cast<int> (type));
      return result;
    }();
    return types;
  }

private:
  /** Lightness and chroma shared by all derived defaults. */
  static constexpr double kBaseLightness = 0.70;
  static constexpr double kBaseChroma = 0.25;

  /** The light-mode page color the contrast target is measured
   * against (ZrythmTheme.lightPageColor). */
  static inline const QColor kLightReferenceColor{ "#E3E3E3" };

  /** Contrast ratio light-mode defaults must reach. */
  static constexpr double kLightContrastTarget = 3.0;

  /** Minimum lightness the light-mode derivation may descend to. */
  static constexpr double kLightLightnessFloor = 0.45;

  /**
   * @brief The fixed hue of a track type.
   *
   * The switch covers every Track::Type, so adding a type without a
   * hue is a compile error.
   */
  static double hue_of_type (structure::tracks::Track::Type type)
  {
    using Type = structure::tracks::Track::Type;
    switch (type)
      {
      case Type::Master:
        return 0.;
      case Type::MidiGroup:
        return 40.;
      case Type::Instrument:
        return 60.;
      case Type::Midi:
        return 80.;
      case Type::MidiBus:
        return 100.;
      case Type::Audio:
        return 130.;
      case Type::AudioBus:
        return 155.;
      case Type::AudioGroup:
        return 180.;
      case Type::Folder:
        return 210.;
      case Type::Chord:
        return 250.;
      case Type::Modulator:
        return 290.;
      case Type::Marker:
        return 320.;
      }
    std::unreachable ();
  }
};
} // namespace zrythm::gui::qquick
