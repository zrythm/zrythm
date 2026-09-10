// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <functional>

#include "controllers/clipboard.h"
#include "plugins/plugin_all.h"
#include "structure/project/project_registry.h"
#include "structure/tracks/track_all.h"
#include "undo/undo_stack.h"

#include <QtQmlIntegration/qqmlintegration.h>

namespace zrythm::actions
{

class PluginOperator : public QObject
{
  Q_OBJECT
  Q_PROPERTY (
    bool canPastePlugins READ canPastePlugins NOTIFY canPastePluginsChanged FINAL)
  QML_ELEMENT
  QML_UNCREATABLE ("Needs plugin registry")

public:
  explicit PluginOperator (
    undo::UndoStack                     &undo_stack,
    structure::project::ProjectRegistry &project_registry,
    controllers::Clipboard              &clipboard,
    std::function<QString ()>            project_id_provider,
    QObject *                            parent = nullptr)
      : QObject (parent), project_registry_ (project_registry),
        undo_stack_ (undo_stack), clipboard_ (clipboard),
        project_id_provider_ (std::move (project_id_provider))
  {
    QObject::connect (
      &clipboard_, &controllers::Clipboard::payloadChanged, this,
      [this] () { Q_EMIT canPastePluginsChanged (); });
  }

  /** Whether the clipboard holds a pasteable plugin payload. */
  bool canPastePlugins () const { return clipboard_.hasPlugins (); }

  /**
   * @brief Moves one or more plugins between PluginGroups.
   *
   * @param plugins Plugins to move.
   * @param source_group Group the plugins currently belong to.
   * @param source_track Track owning the source group (for automation moves).
   * @param target_group Group to move the plugins into.
   * @param target_track Track owning the target group (for automation moves).
   * @param target_start_index Starting position in the target group.
   * -1 means append to the end.
   */
  Q_INVOKABLE void movePlugins (
    QList<plugins::Plugin *>   plugins,
    plugins::PluginGroup *     source_group,
    structure::tracks::Track * source_track,
    plugins::PluginGroup *     target_group,
    structure::tracks::Track * target_track,
    int                        target_start_index = -1);

  /**
   * @brief Removes one or more plugins from their PluginGroup.
   *
   * @param plugins Plugins to remove.
   * @param group Group the plugins belong to.
   * @param track Track owning the group (for automation track cleanup).
   */
  Q_INVOKABLE void removePlugins (
    QList<plugins::Plugin *>   plugins,
    plugins::PluginGroup *     group,
    structure::tracks::Track * track);

  /**
   * @brief Copies the closure of @p plugins (each plugin with its ports
   * and parameters) to the clipboard.
   *
   * @return false if the selection is empty or serialization fails, in
   * which case the clipboard keeps its previous payload.
   */
  Q_INVOKABLE bool copyPlugins (QList<plugins::Plugin *> plugins);

  /**
   * @brief Copies @p plugins to the clipboard, then removes them from
   * @p group.
   *
   * @return false if the copy step refused the operation; in that case
   * nothing is removed.
   */
  Q_INVOKABLE bool cutPlugins (
    QList<plugins::Plugin *>   plugins,
    plugins::PluginGroup *     group,
    structure::tracks::Track * track);

  /**
   * @brief Pastes the clipboard's plugins into @p target_group.
   *
   * Every plugin's category must match the group's signal type
   * (instruments into instrument groups, MIDI modifiers into MIDI
   * groups, everything else into audio groups); a mismatch refuses the
   * whole paste. The pasted plugins must finish instantiating before
   * anything is attached; a failed or timed-out instantiation refuses
   * the paste too.
   *
   * @param target_group Group to paste into.
   * @param index Position to insert at (-1 appends).
   * @return The pasted plugins' UUID strings (brace-less), empty if the
   * paste was refused.
   */
  Q_INVOKABLE QVariantList
  pastePlugins (plugins::PluginGroup * target_group, int index = -1);

  /**
   * @brief Duplicates @p plugins inside @p group, inserting after the
   * last duplicated plugin.
   *
   * The duplicates are cloned directly without touching the clipboard
   * contents.
   *
   * @return The duplicated plugins' UUID strings (brace-less), empty if
   * the operation was refused.
   */
  Q_INVOKABLE QVariantList duplicatePlugins (
    QList<plugins::Plugin *> plugins,
    plugins::PluginGroup *   group);

Q_SIGNALS:
  /**
   * @brief Emitted when an operation is refused (e.g. a plugin's
   * category does not fit the target group), with the reason.
   */
  void operationRefused (const QString &reason);

  void canPastePluginsChanged ();

private:
  struct PreparedPluginPaste
  {
    structure::project::ClipboardPayload payload;
    std::vector<QUuid>                   imported_ids;
  };

  /**
   * @brief Serializes the closure of @p plugins into a plugin payload.
   *
   * @return The payload, or std::nullopt if nothing is copyable or
   * serialization fails.
   */
  [[nodiscard]] std::optional<structure::project::ClipboardPayload>
  build_plugins_payload (const QList<plugins::Plugin *> &plugins) const;

  /**
   * @brief Filters, regenerates the UUIDs of, and imports the clipboard
   * payload's plugins into the project registry.
   */
  [[nodiscard]] std::optional<PreparedPluginPaste> prepare_plugin_paste ();

  /**
   * @brief Filters, regenerates the UUIDs of, and imports @p payload's
   * plugins into the project registry.
   */
  [[nodiscard]] std::optional<PreparedPluginPaste>
  import_payload_for_paste (const structure::project::ClipboardPayload &payload);

  /**
   * @brief Attaches the imported roots of @p paste to @p target_group.
   *
   * Validates each root's category against the group, waits for the
   * instantiations to finish (refusing the paste on failure or timeout)
   * and discards the imports the attached roots do not need.
   *
   * @param label_pattern Translated undo-macro label with a %1
   * placeholder for the plugin count.
   * @return The attached plugins' UUID strings (brace-less), empty if
   * the paste was refused.
   */
  [[nodiscard]] QVariantList attach_paste (
    const PreparedPluginPaste &paste,
    plugins::PluginGroup *     target_group,
    int                        index,
    const QString             &label_pattern);

  /** Logs @p reason and emits operationRefused(). */
  void refuse_operation (const QString &reason);

  structure::project::ProjectRegistry &project_registry_;
  undo::UndoStack                     &undo_stack_;
  controllers::Clipboard              &clipboard_;
  std::function<QString ()>            project_id_provider_;
};

} // namespace zrythm::actions
