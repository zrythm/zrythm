// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import Zrythm
import ZrythmMcp
import ZrythmStyle
import "../config.js" as Config

ApplicationWindow {
  id: root

  // View context of the focused view (arranger, tracklist or plugin
  // slot list); see components/ViewContext.qml
  property ViewContext activeViewContext: null
  required property AppSettings appSettings
  required property ChordPresetManager chordPresetManager
  // Set while a requested close waits for the end of a save
  property bool closeAfterSave: false
  required property ControlRoom controlRoom
  readonly property Action copyAction: Action {
    id: copyAction

    enabled: root.activeViewContext !== null && root.activeViewContext.canCopy
    shortcut: StandardKey.Copy
    text: qsTr("&Copy")

    onTriggered: root.activeViewContext.copyRequested()
  }
  readonly property Action cutAction: Action {
    id: cutAction

    enabled: root.activeViewContext !== null && root.activeViewContext.canCopy
    shortcut: StandardKey.Cut
    text: qsTr("Cu&t")

    onTriggered: root.activeViewContext.cutRequested()
  }
  readonly property Action deleteAction: Action {
    id: deleteAction

    enabled: root.activeViewContext !== null && root.activeViewContext.canDelete
    shortcut: StandardKey.Delete
    text: qsTr("&Delete")

    onTriggered: root.activeViewContext.deleteRequested()
  }
  required property DeviceManager deviceManager
  readonly property Action duplicateAction: Action {
    id: duplicateAction

    enabled: root.activeViewContext !== null && root.activeViewContext.canCopy
    shortcut: "Ctrl+D"
    text: qsTr("&Duplicate")

    onTriggered: root.activeViewContext.duplicateRequested()
  }
  readonly property Action fullScreenAction: Action {
    id: fullScreenAction

    shortcut: StandardKey.FullScreen
    text: qsTr("Fullscreen")

    onTriggered: {
      root.visibility = root.visibility === Window.FullScreen ? Window.AutomaticVisibility : Window.FullScreen;
    }
  }
  required property NotificationCenter notificationCenter
  readonly property Action pasteAction: Action {
    id: pasteAction

    enabled: root.activeViewContext !== null && root.activeViewContext.canPaste
    shortcut: StandardKey.Paste
    text: qsTr("&Paste")

    onTriggered: root.activeViewContext.pasteRequested()
  }
  readonly property Project project: session.project
  required property ProjectSession session

  signal closeApproved()
  signal closeCancelled()

  /**
   * Posts an error notification with the reason of the session's MCP
   * endpoint while its state is Failed.
   */
  function reportFailedMcpEndpoint() {
    const endpoint = root.session.mcpEndpoint;
    if (endpoint.state === McpEndpoint.Failed)
      root.notificationCenter.postError(
        qsTr("MCP Server Failed to Start"), endpoint.errorText, "mcp-endpoint");
  }

  /**
   * Closes the window. Unsaved changes ask the user first. Reports
   * closeApproved() when the window closes and closeCancelled() when
   * the user cancels or the save fails. A request that arrives while
   * a close waits for its save is ignored.
   */
  function requestClose() {
    // A close already waits for a save, and that save decides it
    if (root.closeAfterSave)
      return;
    if (root.session.hasUnsavedChanges) {
      unsavedChangesDialog.open();
      return;
    }
    root.closeAndDestroy();
  }

  /**
   * Saves the project and closes the window when the save succeeded.
   * Otherwise the window stays open and closeCancelled() is reported.
   */
  function saveThenClose() {
    if (!saveController.save()) {
      root.closeCancelled();
      return;
    }
    root.closeAfterSave = true;
  }

  /**
   * Closes the window without a check for unsaved changes: schedules
   * the destruction of the window and reports closeApproved().
   */
  function closeAndDestroy() {
    console.log("Closing and destroying project window");
    root.hide();
    // destroy() only posts the deletion, and posted steps run in
    // order: it comes first, so that steps queued by the handlers of
    // closeApproved() run after the window is deleted
    destroy();
    root.closeApproved();
    root.project.engine.deactivate();
  }

  function updateActiveViewContext(focusItem: Item): void {
    let item = focusItem;
    while (item !== null && item !== undefined) {
      if (item.viewContext !== undefined) {
        activeViewContext = item.viewContext;
        return;
      }
      item = item.parent;
    }
  }

  height: 720
  title: session.title + (session.hasUnsavedChanges ? " *" : "")
  visible: true
  width: 1280

  header: MainToolbar {
    id: headerBar

    appSettings: root.appSettings
    controlRoom: root.controlRoom
    notificationCenter: root.notificationCenter
    session: root.session
  }
  menuBar: MainMenuBar {
    id: mainMenuBar

    aboutDialog: aboutDialog
    copyAction: root.copyAction
    cutAction: root.cutAction
    deleteAction: root.deleteAction
    deviceManager: root.deviceManager
    dockWorkspace: projectDockWorkspace
    duplicateAction: root.duplicateAction
    exportDialog: exportDialog
    importFileDialog: importFileDialog
    loadController: loadController
    pasteAction: root.pasteAction
    preferencesDialog: preferencesDialog
    saveController: saveController
    session: root.session
  }

  Component.onCompleted: {
    console.log("ApplicationWindow created on platform", Qt.platform.os);
    project.engine.activate();
    // The endpoint can fail before this window exists
    root.reportFailedMcpEndpoint();
  }

  // Resolves the view context from focus: the nearest ancestor of the
  // focused item that exposes a viewContext property. Focus moving to
  // items without a context (menu bar, menus, buttons, text fields)
  // keeps the last resolved context; the property clears automatically
  // when the providing view is destroyed. Text inputs consume the
  // standard edit shortcuts while focused.
  onActiveChanged: {
    if (active)
      updateActiveViewContext(activeFocusItem);
  }
  onActiveFocusItemChanged: updateActiveViewContext(activeFocusItem)
  onClosing: close => {
    console.log("Closing project window...");
    close.accepted = false;
    root.requestClose();
  }

  // Temporary scissors: override the arranger tool with the Cut tool while
  // Alt is held
  Connections {
    function onModifierHeldChanged() {
      if (KeyboardState.altHeld)
        root.session.uiState.tool.setToolValueOverride(ArrangerTool.Cut);
      else
        root.session.uiState.tool.clearToolValueOverride();
    }

    target: KeyboardState
  }

  NotificationArea {
    notificationCenter: root.notificationCenter

    onCriticalNotification: (notification) => {
      alertDialog.text = notification.title;
      alertDialog.informativeText = notification.detail;
      alertDialog.open();
    }
  }

  Connections {
    function onImportFailed(filePath, reason) {
      root.notificationCenter.postError(qsTr("Cannot Import File"), filePath + "\n" + reason);
    }

    target: root.session.fileImporter
  }

  Connections {
    function onMidiNoteOnReceived() {
      if (root.appSettings.startPlaybackOnMidiInput
          && !root.project.transport.isRolling()) {
        root.project.transport.requestRoll();
      }
    }

    target: root.deviceManager
  }

  Connections {
    function onOperationRefused(reason) {
      root.notificationCenter.postError(qsTr("Cannot Perform Operation"), reason);
    }

    function onPasteContentModified(summary) {
      root.notificationCenter.postWarning(qsTr("Pasted Content Modified"), summary);
    }

    target: root.session.arrangerObjectSelectionOperator
  }

  Connections {
    function onOperationRefused(reason) {
      root.notificationCenter.postError(qsTr("Cannot Perform Operation"), reason);
    }

    target: root.session.pluginOperator
  }

  Connections {
    function onOperationRefused(reason) {
      root.notificationCenter.postError(qsTr("Cannot Perform Operation"), reason);
    }

    function onPasteContentModified(summary) {
      root.notificationCenter.postWarning(qsTr("Pasted Content Modified"), summary);
    }

    target: root.session.trackCollectionOperator
  }

  Connections {
    function onInstantiationFailed(pluginName, error) {
      root.notificationCenter.postError(qsTr("Plugin Instantiation Failed"), qsTr("Failed to instantiate plugin %1:\n\n%2").arg(pluginName).arg(error));
    }

    target: root.session.pluginImporter
  }

  Connections {
    function onStateChanged() {
      root.reportFailedMcpEndpoint();
    }

    target: root.session.mcpEndpoint
  }

  Connections {
    function onRowsAboutToBeRemoved(modelIndex: var, first: int, last: int) {
      // Ignore transient removals that are part of a track move
      if (root.project.tracklist.collection.moveInProgress)
        return;

      // Auto-deselect tracks when removed from the project
      for (let i = first; i <= last; ++i) {
        const trackModelIndex = trackSelectionModel.getModelIndex(i);
        trackSelectionModel.select(trackModelIndex, ItemSelectionModel.Deselect);
      }
    }

    function onRowsInserted(modelIndex: var, first: int, last: int) {
      // Ignore transient insertions that are part of a track move
      if (root.project.tracklist.collection.moveInProgress)
        return;

      // Auto-select tracks when added to the project
      const trackModelIndex = trackSelectionModel.getModelIndex(last);
      trackSelectionModel.selectSingleTrack(trackModelIndex);
    }

    function onRowsRemoved(modelIndex: var, first: int, last: int) {
      // Ignore transient removals that are part of a track move
      if (root.project.tracklist.collection.moveInProgress)
        return;

      // Ensure at least 1 track is always selected
      if (!trackSelectionModel.hasSelection) {
        const numTracks = root.project.tracklist.collection.rowCount();

        // Select next track, or prev track if next doesn't exist
        let indexToSelect = first;
        if (numTracks === first) {
          --indexToSelect;
        }

        const trackModelIndex = trackSelectionModel.getModelIndex(indexToSelect);
        trackSelectionModel.selectSingleTrack(trackModelIndex);
      }
    }

    target: root.project.tracklist.collection
  }

  AboutDialog {
    id: aboutDialog
  }

  ExportDialog {
    id: exportDialog

    exportDirectory: root.session.projectDirectory + "/exports"
    notificationCenter: root.notificationCenter
    session: root.session
  }

  PreferencesDialog {
    id: preferencesDialog

    appSettings: root.appSettings
  }

  FileDialog {
    id: importFileDialog

    fileMode: FileDialog.OpenFiles
    nameFilters: [
      qsTr("Importable files (%1)").arg(
        "*.mid *.midi *.smf *.wav *.aif *.aiff *.flac *.ogg *.mp3"),
      qsTr("MIDI files (%1)").arg("*.mid *.midi *.smf"),
      qsTr("Audio files (%1)").arg("*.wav *.aif *.aiff *.flac *.ogg *.mp3"),
      qsTr("All files (%1)").arg("*")
    ]
    title: qsTr("Import Files")

    onAccepted: {
      const paths = Array.from(importFileDialog.selectedFiles).map(
        url => QmlUtils.toPathString(url));
      root.session.fileImporter.importFiles(paths, 0, null);
    }
  }

  MessageDialog {
    id: alertDialog

    buttons: MessageDialog.Ok
  }

  Dialog {
    id: unsavedChangesDialog

    closePolicy: Popup.CloseOnEscape
    modal: true
    popupType: Popup.Window
    title: qsTr("Save Changes?")
    width: 420

    ColumnLayout {
      anchors.fill: parent
      spacing: ZrythmTheme.buttonPadding

      Label {
        Layout.fillWidth: true
        color: palette.text
        text: qsTr("The project has changes that are not saved. Save them before closing?")
        wrapMode: Text.WordWrap
      }
    }

    footer: DialogButtonBox {
      defaultButton: saveButton

      Button {
        DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
        text: qsTr("Cancel")
      }

      DestructiveButton {
        DialogButtonBox.buttonRole: DialogButtonBox.DestructiveRole
        text: qsTr("Discard")
      }

      Button {
        id: saveButton

        highlighted: true
        text: qsTr("Save")

        onClicked: {
          unsavedChangesDialog.close();
          root.saveThenClose();
        }
      }
    }

    onDiscarded: root.closeAndDestroy()
    onRejected: root.closeCancelled()
  }

  TrackSelectionModel {
    id: trackSelectionModel

    // Map to track which tracks were automatically armed for recording
    // Key: track object, Value: boolean indicating if recording was set automatically
    property var automaticallyArmedTracks: new Map()

    model: root.project.tracklist.collection

    Component.onCompleted: {
      // Select last track
      const numTracks = root.project.tracklist.collection.rowCount();
      if (numTracks > 0) {
        const trackModelIndex = getModelIndex(numTracks - 1);
        selectSingleTrack(trackModelIndex);
      }
    }
    onSelectionChanged: (selected, deselected) => {
      if (root.appSettings.trackAutoArm) {
        // Disarm all previously auto-armed tracks first
        // Auto-arm is host policy, not a user edit: write quietly
        automaticallyArmedTracks.forEach((value, track) => {
          track.recordingParam.setBaseValue(0.0);
        });
        automaticallyArmedTracks.clear();
      }

      if (selectedIndexes.length > 0) {
        selected.forEach(selectedRange => {
          for (let i = selectedRange.topLeft; i <= selectedRange.bottomRight; i++) {
            const track = getTrackFromModelIndex(i);
            if (track.recordingParam) {
              if (root.appSettings.trackAutoArm) {
                if (!track.recordingParam.range.isToggled(track.recordingParam.baseValue)) {
                  track.recordingParam.setBaseValue(1.0);
                  automaticallyArmedTracks.set(track, true);
                }
              }
            }
          }
        });
      }
    }
  }

  // Global spacebar for play/pause
  Shortcut {
    context: Qt.ApplicationShortcut
    sequence: "Space"

    onActivated: {
      // Toggle play/pause regardless of focus
      if (root.project.transport.isRolling()) {
        root.project.transport.requestPause();
      } else {
        root.project.transport.requestRoll();
      }
    }
  }

  // Tools
  Shortcut {
    sequence: "1"

    onActivated: {
      root.session.uiState.tool.toolValue = ArrangerTool.Select;
    }
  }

  Shortcut {
    sequence: "2"

    onActivated: {
      root.session.uiState.tool.toolValue = ArrangerTool.Edit;
    }
  }

  Shortcut {
    sequence: "3"

    onActivated: {
      root.session.uiState.tool.toolValue = ArrangerTool.Cut;
    }
  }

  Shortcut {
    sequence: "4"

    onActivated: {
      root.session.uiState.tool.toolValue = ArrangerTool.Eraser;
    }
  }

  Shortcut {
    sequence: "5"

    onActivated: {
      root.session.uiState.tool.toolValue = ArrangerTool.Ramp;
    }
  }

  Shortcut {
    sequence: "6"

    onActivated: {
      root.session.uiState.tool.toolValue = ArrangerTool.Audition;
    }
  }

  LoadController {
    id: loadController
  }

  SaveController {
    id: saveController

    notificationCenter: root.notificationCenter
    session: root.session

    // Ends a close that waits for this save
    onSaveEnded: saved => {
      if (!root.closeAfterSave)
        return;

      root.closeAfterSave = false;
      if (saved)
        root.closeAndDestroy();
      else
        root.closeCancelled();
    }
  }

  PlaybackCacheActivityAggregator {
    id: cacheActivityAggregator

    collection: root.project.tracklist.collection
  }

  // Generic editor windows for plugins without a native UI
  Instantiator {
    id: genericPluginWindows

    model: root.session.genericPluginUiController

    delegate: GenericPluginEditorWindow {
    }

    onObjectAdded: (index, object) => {
      object.plugin = genericPluginWindows.model.pluginAt(index);
      object.parameterOperator = root.session.processorParameterOperator;
      // Set the transient parent before the window is shown so the WM keeps
      // it stacked above the project window (must happen pre-mapping on
      // Wayland), and inherit the palette (not propagated across windows)
      object.transientParent = root;
      object.palette = Qt.binding(() => root.palette);
      object.visible = true;
    }
  }

  ColumnLayout {
    anchors.fill: parent
    spacing: 0

    ProjectDockWorkspace {
      id: projectDockWorkspace

      Layout.fillHeight: true
      Layout.fillWidth: true
      appSettings: root.appSettings
      cacheActivityAggregator: cacheActivityAggregator
      chordPresetManager: root.chordPresetManager
      controlRoom: root.controlRoom
      deviceManager: root.deviceManager
      dockPalette: root.palette
      importFileDialog: importFileDialog
      session: root.session
      trackSelectionModel: trackSelectionModel

      onFocusItemChanged: focusItem => root.updateActiveViewContext(focusItem)
    }

    StatusBar {
      id: statusBar

      Layout.fillWidth: true

      leftItems: [
        StatusBarText {
          text: qsTr("%1 Hz · %2 samples").arg(root.project.engine.sampleRate.toLocaleString(Qt.locale(), "f", 0)).arg(root.project.engine.blockLength.toLocaleString(Qt.locale(), "f", 0))
        },
        StatusBarText {
          text: qsTr("%1 tracks").arg(root.project.tracklist.collection.trackCount)
        },
        RecordingIndicator {
          recording: root.project.transport.recordingActive
        }
      ]
      rightItems: [
        StatusBarText {
          text: qsTr("Cache: %1 pending · %2 complete").arg(cacheActivityAggregator.cachePendingCount).arg(cacheActivityAggregator.cacheCompleteCount)
          visible: root.appSettings.showCacheActivity
        }
      ]
    }
  }
}
