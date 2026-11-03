// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <memory>

#include <QVariantList>
#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::structure::project
{
class Project;
class ProjectUiState;
}

namespace zrythm::gui
{

/**
 * @brief Read-only projection of project processors and effective graph edges
 * for the Modular view.
 *
 * The projection uses stable project UUIDs and is rebuilt on the main thread.
 * It does not own or expose DSP graph nodes, and it does not mutate project
 * routing.
 */
class ModularProjectModel : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  QML_UNCREATABLE ("")
  Q_PROPERTY (QVariantList nodes READ nodes NOTIFY nodesChanged FINAL)
  Q_PROPERTY (QVariantList edges READ edges NOTIFY edgesChanged FINAL)
  Q_PROPERTY (bool global READ global NOTIFY scopeChanged FINAL)
  Q_PROPERTY (
    QString scopeTrackUuid READ scopeTrackUuid NOTIFY scopeChanged FINAL)
  Q_PROPERTY (QString scopeTitle READ scopeTitle NOTIFY scopeChanged FINAL)

public:
  explicit ModularProjectModel (
    structure::project::Project        &project,
    structure::project::ProjectUiState &ui_state,
    QObject *                           parent = nullptr);
  ~ModularProjectModel () override;

  const QVariantList &nodes () const;
  const QVariantList &edges () const;
  bool                global () const;
  QString             scopeTrackUuid () const;
  QString             scopeTitle () const;

  /**
   * @brief Sets the track projected by Track scope without changing scope.
   *
   * Global scope keeps its projection and updates the linked track. Invalid
   * and unknown track UUIDs are ignored.
   *
   * @param track_uuid UUID of a track in the project tracklist.
   */
  Q_INVOKABLE void setScopeTrack (const QString &track_uuid);

  /**
   * @brief Switches to Track scope and projects the given track.
   *
   * @param track_uuid UUID of a track in the project tracklist.
   */
  Q_INVOKABLE void showTrack (const QString &track_uuid);

  /** @brief Switches to Global scope. */
  Q_INVOKABLE void showGlobal ();
  Q_INVOKABLE void setNodePosition (const QString &node_id, double x, double y);
  Q_INVOKABLE QVariantMap viewport () const;
  Q_INVOKABLE void        setViewport (double pan_x, double pan_y, double zoom);
  Q_INVOKABLE void        arrangeAutomatically ();

  /**
   * @brief Modulatable parameters of a node's processors.
   *
   * @param node_id A projection node ID (processor node in Track scope or
   * collapsed track node in Global scope).
   * @return Entries with "label", "portId" (modulation input CV port UUID)
   * and "kind" ("parameter").
   */
  Q_INVOKABLE QVariantList modulatableParameters (const QString &node_id) const;

Q_SIGNALS:
  void nodesChanged ();
  void edgesChanged ();
  void scopeChanged ();

private:
  struct Impl;

  void schedule_rebuild ();
  void rebuild_projection ();

  /**
   * @brief Adopts the scope and scope track stored in the UI state, emitting
   * scopeChanged() when the effective scope changes.
   */
  void resume_saved_scope ();
  /** @brief Adopts the persisted scope and queues a projection rebuild. */
  void on_ui_state_restored ();

  structure::project::Project        &project_;
  structure::project::ProjectUiState &ui_state_;
  std::unique_ptr<Impl>               impl_;

  QVariantList nodes_;
  QVariantList edges_;
  bool         global_ = false;
};

} // namespace zrythm::gui
