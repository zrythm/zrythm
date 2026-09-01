// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QObject>
#include <QUrl>
#include <QVariantMap>

namespace zrythm::gui
{

/** URL of the QML component shown in preset popup scenes. */
inline QUrl
preset_popup_component_url ()
{
  return QUrl (QStringLiteral (
    "qrc:/qt/qml/Zrythm/components/basic/PresetBrowserPopup.qml"));
}

/**
 * @brief Initial properties every preset popup scene is created with.
 *
 * @param model Preset list model shown in the browser.
 * @param current_index Source-model row to highlight initially.
 * @param popup_host The PresetPopupController bridging the scene to the
 * host.
 */
inline QVariantMap
preset_popup_initial_properties (
  QObject * model,
  int       current_index,
  QObject * popup_host)
{
  return {
    { QStringLiteral ("model"),        QVariant::fromValue (model)      },
    { QStringLiteral ("currentIndex"), current_index                    },
    { QStringLiteral ("popupHost"),    QVariant::fromValue (popup_host) },
  };
}

/**
 * @brief Invokable surface passed to windowed preset popup scenes.
 *
 * The popup scene's signals are QML-declared, so C++ cannot connect to
 * them with member-function-pointer connects; instead the scene calls
 * these invokables on its `popupHost` property, and the controller
 * re-emits them as regular C++ signals for the host window backend.
 *
 * Must outlive the popup scene's root item.
 */
class PresetPopupController final : public QObject
{
  Q_OBJECT

public:
  using QObject::QObject;

  Q_INVOKABLE void presetPopupActivated (int index)
  {
    Q_EMIT activated (index);
  }

  Q_INVOKABLE void presetPopupDismissed () { Q_EMIT dismissed (); }

Q_SIGNALS:
  /** The user picked an item. */
  void activated (int index);
  /** The user dismissed the popup without selecting (Escape). */
  void dismissed ();
  /**
   * The host is about to close/destroy the popup scene; the scene must
   * end any active audition session now, while the controller, header
   * and plugin are all guaranteed alive (deferred scene destruction can
   * outlive them, and the QML-side session end would then find null
   * references and skip the commit).
   */
  void sessionEndRequested ();
};

} // namespace zrythm::gui
