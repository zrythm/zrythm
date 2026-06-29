// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <optional>

#include <QObject>
#include <QPointer>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::dsp
{

/**
 * @brief Nested namespace isolating the timebase enum and provider together
 * with the Qt meta-object registration (Q_NAMESPACE/Q_ENUM_NS), so the @ref
 * zrythm::dsp namespace itself stays free of Qt-specific declarations.
 */
namespace timebase
{

Q_NAMESPACE

/**
 * @brief Per-track timebase that controls how clip content maps to the
 * project timeline.
 *
 * - Musical: clip follows project tempo (default for non-audio tracks).
 * - Absolute: clip locked to wall-clock time at its source tempo (default
 *   for audio tracks, where the file's native BPM is the reference).
 */
enum class Timebase : std::uint8_t
{
  Musical,
  Absolute,
};
Q_ENUM_NS (Timebase)

/**
 * @brief Self-nesting timebase provider that resolves an effective timebase
 * through a chain of providers.
 *
 * A provider holds an optional @ref override (an explicit Timebase value) and
 * an optional @ref source (another provider to observe). The effective
 * timebase is: override if set, else the source's effective timebase, else
 * @ref Timebase::Musical as a safe default.
 *
 * This allows arbitrary-depth chains: a track's provider (root, override =
 * Musical or Absolute) → a lane's provider (optional override) → a clip's
 * provider (optional override). Each level connects to its source's
 * @ref effectiveTimebaseChanged and re-emits its own when the resolved value
 * changes.
 *
 * @pre The @ref source graph must be acyclic. Resolution is bounded by
 * @ref kMaxChainDepth as a safety net: a cycle (or an unreasonably deep chain)
 * is treated as if the chain ended and @ref Timebase::Musical is returned.
 */
class TimebaseProvider : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    Timebase effectiveTimebase READ effectiveTimebase NOTIFY
      effectiveTimebaseChanged)
  QML_ELEMENT
  QML_UNCREATABLE ("")

public:
  explicit TimebaseProvider (QObject * parent = nullptr);

  /// Sets the parent provider to observe. Pass nullptr to detach.
  void setSource (TimebaseProvider * source);

  /// Forces a specific timebase value, taking precedence over the source.
  void setOverride (Timebase t);
  void clearOverride ();

  [[nodiscard]] bool hasOverride () const { return override_.has_value (); }
  [[nodiscard]] std::optional<Timebase> overrideValue () const
  {
    return override_;
  }

  /// Returns the resolved value (override if set, else source, else Musical).
  [[nodiscard]] Timebase effectiveTimebase () const;

  Q_SIGNAL void effectiveTimebaseChanged ();
  Q_SIGNAL void overrideChanged ();

private:
  /// Maximum source-chain depth traversed by @ref effectiveTimebase before
  /// falling back to @ref Timebase::Musical. Guards against cycles.
  static constexpr int kMaxChainDepth = 64;

  void recompute_and_emit_if_changed ();

  QPointer<TimebaseProvider> source_;
  std::optional<Timebase>    override_;
  Timebase                   cached_effective_ = Timebase::Musical;
};

} // namespace timebase

/// Convenience alias keeping existing @ref zrythm::dsp::Timebase references
/// valid after the move into the @ref timebase subnamespace.
using Timebase = timebase::Timebase;
/// Convenience alias keeping existing @ref zrythm::dsp::TimebaseProvider
/// references valid after the move into the @ref timebase subnamespace.
using TimebaseProvider = timebase::TimebaseProvider;

} // namespace zrythm::dsp

Q_DECLARE_METATYPE (zrythm::dsp::timebase::Timebase)
