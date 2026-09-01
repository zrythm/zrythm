// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQml.Models
import ZrythmStyle

/**
 * Preset browser with search, group filtering and keyboard auditioning,
 * shown when a PresetSelector's name button is clicked.
 *
 * Used as the content of PresetSelector's in-scene popup, and standalone
 * as the root of windowed popups over native plugin strips.
 *
 * Auditioning: when the model has a `plugin` property (e.g.
 * PluginPresetListModel), a session begins with
 * Plugin.beginPresetAudition() and each selection (click or keyboard
 * navigation, debounced) is applied to the plugin so it can be heard
 * while browsing. Applying (Enter/Return, OK button, double-click)
 * commits the selection; cancelling (Escape, Cancel button) reverts to
 * the pre-audition snapshot; any other close path (outside click, focus
 * loss, host close) also commits the selected preset. With a model
 * without a plugin, the browser only reports selections.
 *
 * Session lifetime: with a popupHost (windowed hosts create the browser
 * per open), the session begins on creation and ends on destruction;
 * in-scene the browser persists, so the session follows visibility.
 *
 * Indices crossing the component boundary (currentIndex, activated(),
 * applied()) are source-model rows, not rows of the internal filter
 * proxy.
 */
Item {
  id: root

  // Item count of the filtered list
  readonly property int count: listPane.count
  // Group selected in the group pane, or "" for all groups
  property string currentGroup: ""

  // Index of the selected preset, as a source-model row, or -1 for none
  property int currentIndex: -1
  readonly property alias groupList: groupList
  // Source row of the last audition-applied preset; skips redundant
  // applies when the debounce fires without navigation
  property int lastAuditionedSourceRow: -1
  // The preset list view, the group list and the search field
  readonly property alias listView: listPane.listView
  // Source row selected when the session began (the revert target,
  // shown in the footer)
  property int loadedSourceRow: -1
  // Source model of presets with a "name" role and optionally a "group"
  // role (e.g. PluginPresetListModel); auditioning requires its `plugin`
  // property
  property var model: null
  // Whether the model has more than one named group; with zero or one
  // group the group pane would only duplicate the "All" entry, so it is
  // hidden (same for the list's section headers)
  readonly property bool multipleGroups: (root.model?.groups?.length ?? 0) > 1

  // The plugin being auditioned, or null when the model has none
  readonly property var plugin: root.model?.plugin ?? null
  // When set (windowed hosts), committed/dismissed are also reported by
  // calling the host's presetPopupActivated()/presetPopupDismissed()
  // invokables, since C++ cannot connect to this component's QML-declared
  // signals. Must outlive this item.
  property var popupHost: null
  readonly property alias searchField: searchField

  // Whether the plugin accepted the audition session (a state snapshot
  // exists to revert to). When false the browser is pick-only: nothing
  // is auditioned and commit/revert are not called for a session that
  // does not exist
  property bool pluginSessionActive: false

  // Whether an audition session is active; also guards the multiple
  // hide/destroy paths against ending twice
  property bool sessionActive: false

  // Emitted when the user applies a preset (Enter/Return, OK button,
  // double-click); index is a source-model row
  signal activated(int index)
  // Emitted when an active session ends by committing passively (the
  // browser was hidden or destroyed without an explicit apply or cancel)
  // and the selection differs from what was loaded when the session
  // began; index is a source-model row. Only emitted in-scene (no
  // popupHost): windowed hosts derive their state from the plugin itself
  signal applied(int index)
  // Emitted when the user dismisses the browser with Escape or the Cancel
  // button (other dismissal paths hide or destroy the browser directly)
  signal closeRequested

  // Applies the highlighted preset to the plugin, unless it is already
  // the last auditioned one. Only meaningful while a session is active
  // (the debounce timer can still be pending when a session ends)
  function applyPendingAudition() {
    if (!root.sessionActive || !root.pluginSessionActive || !root.plugin)
      return;
    const sourceRow = root.sourceRowForProxyRow(listPane.listView.currentIndex);
    if (sourceRow < 0 || sourceRow === root.lastAuditionedSourceRow)
      return;
    root.lastAuditionedSourceRow = sourceRow;
    root.plugin.presetIndex = sourceRow;
  }

  function beginSession() {
    if (root.sessionActive)
      return;
    root.sessionActive = true;
    searchField.clear();
    root.currentGroup = "";
    groupList.currentIndex = 0;
    root.lastAuditionedSourceRow = root.currentIndex;
    root.loadedSourceRow = root.currentIndex;
    if (root.plugin)
      root.pluginSessionActive = root.plugin.beginPresetAudition();
    listPane.currentIndex = root.proxyRowForSourceRow(root.currentIndex);
    searchField.forceInputFocus();
  }

  function commitCurrentOrFirst() {
    let row = listPane.listView.currentIndex;
    if (row < 0 && listPane.count > 0)
      row = 0;
    root.commitRow(row);
  }

  function commitRow(proxyRow: int) {
    if (proxyRow < 0)
      return;
    const sourceRow = root.sourceRowForProxyRow(proxyRow);
    if (sourceRow < 0)
      return;
    // Drop the snapshot before reporting: hosts re-apply the selection,
    // and a live snapshot would let a later dismissal revert it
    root.endSession(true);
    root.activated(sourceRow);
    if (root.popupHost)
      root.popupHost.presetPopupActivated(sourceRow);
  }

  function dismiss() {
    root.endSession(false);
    root.closeRequested();
    if (root.popupHost)
      root.popupHost.presetPopupDismissed();
  }

  function endSession(commit: bool) {
    if (!root.sessionActive)
      return;
    root.sessionActive = false;
    auditionDebounce.stop();
    const hadPluginSession = root.pluginSessionActive;
    root.pluginSessionActive = false;
    if (!root.plugin || !hadPluginSession)
      return;
    if (commit)
      root.plugin.commitPresetAudition();
    else
      root.plugin.revertPresetAudition();
  }

  // Ends an active session by committing the auditioned selection: every
  // close path except Escape/Cancel ends here (outside click, focus loss,
  // host close, destruction). Reports the final selection in-scene
  function endSessionPassively() {
    if (!root.sessionActive)
      return;
    // Flush a highlight still inside the debounce window before
    // committing: windowed hosts derive state from the plugin itself,
    // so an unapplied highlight would be silently lost (in-scene hosts
    // get the highlight via applied() below instead)
    if (root.popupHost)
      root.applyPendingAudition();
    let sourceRow = root.sourceRowForProxyRow(listPane.listView.currentIndex);
    if (sourceRow < 0)
      sourceRow = root.lastAuditionedSourceRow;
    root.endSession(true);
    // Nothing auditioned means nothing to report: re-applying the
    // loaded preset would wipe edits made before the session
    // (setPresetIndex re-applies and clears the dirty flag)
    if (!root.popupHost && sourceRow >= 0 && sourceRow !== root.loadedSourceRow)
      root.applied(sourceRow);
  }

  function proxyRowForSourceRow(sourceRow: int): int {
    if (sourceRow < 0 || !root.model)
      return -1;
    const proxyIdx = filterProxy.mapFromSource(root.model.index(sourceRow, 0));
    return (proxyIdx && proxyIdx.valid) ? proxyIdx.row : -1;
  }

  function sourceRowForProxyRow(proxyRow: int): int {
    if (proxyRow < 0)
      return -1;
    const sourceIdx = filterProxy.mapToSource(filterProxy.index(proxyRow, 0));
    return (sourceIdx && sourceIdx.valid) ? sourceIdx.row : -1;
  }

  implicitHeight: 420
  implicitWidth: 620

  Component.onCompleted: {
    // Windowed hosts create the browser per open and their scenes may
    // never emit visibleChanged, so begin here; the in-scene browser
    // persists and follows visibility instead
    if (root.popupHost)
      root.beginSession();
  }
  Component.onDestruction: root.endSessionPassively()
  onVisibleChanged: {
    if (visible)
      root.beginSession();
    else
      root.endSessionPassively();
  }

  // Windowed hosts request the session end explicitly before tearing
  // down the scene: their deferred destruction can outlive the host
  // objects the session end needs (controller, header, plugin)
  Connections {
    function onSessionEndRequested() {
      root.endSessionPassively();
    }

    target: root.popupHost
  }

  // Windowed hosts (native plugin strips) don't inherit the main window's
  // palette. Bind each role instead of assigning the whole palette
  // object: whole-palette assignment copies the colors once and never
  // follows theme changes
  palette {
    accent: ZrythmTheme.colorPalette.accent
    alternateBase: ZrythmTheme.colorPalette.alternateBase
    base: ZrythmTheme.colorPalette.base
    brightText: ZrythmTheme.colorPalette.brightText
    button: ZrythmTheme.colorPalette.button
    buttonText: ZrythmTheme.colorPalette.buttonText
    dark: ZrythmTheme.colorPalette.dark
    highlight: ZrythmTheme.colorPalette.highlight
    highlightedText: ZrythmTheme.colorPalette.highlightedText
    light: ZrythmTheme.colorPalette.light
    link: ZrythmTheme.colorPalette.link
    linkVisited: ZrythmTheme.colorPalette.linkVisited
    mid: ZrythmTheme.colorPalette.mid
    midlight: ZrythmTheme.colorPalette.midlight
    placeholderText: ZrythmTheme.colorPalette.placeholderText
    shadow: ZrythmTheme.colorPalette.shadow
    text: ZrythmTheme.colorPalette.text
    toolTipBase: ZrythmTheme.colorPalette.toolTipBase
    toolTipText: ZrythmTheme.colorPalette.toolTipText
    window: ZrythmTheme.colorPalette.window
    windowText: ZrythmTheme.colorPalette.windowText
  }

  SortFilterProxyModel {
    id: filterProxy

    model: root.model

    filters: [
      FunctionFilter {
        property string searchText: searchField.text.toLowerCase()

        function filter(data: PresetRoleData): bool {
          return searchText.length === 0 || data.name.toLowerCase().includes(searchText) || data.group.toLowerCase().includes(searchText);
        }

        onSearchTextChanged: {
          filterProxy.invalidate();
          // Filtering drops the highlight's row mapping; clear it
          // (Enter then commits the first match, Down highlights the
          // first row). invalidate() does not emit modelReset, so this
          // cannot be left to the Connections below
          listPane.currentIndex = -1;
        }
      },
      FunctionFilter {
        property string group: root.currentGroup

        function filter(data: PresetRoleData): bool {
          return group.length === 0 || data.group === group;
        }

        onGroupChanged: {
          filterProxy.invalidate();
          listPane.currentIndex = -1;
        }
      }
    ]
  }

  // A source-model reset also drops the highlight's row mapping (filter
  // changes are handled at the invalidate() call sites above)
  Connections {
    function onModelReset() {
      listPane.currentIndex = -1;
    }

    target: filterProxy
  }

  PopupBackgroundRect {
    anchors.fill: parent
  }

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: 4
    spacing: 4

    ZrythmSearchField {
      id: searchField

      Layout.fillWidth: true
      placeholderText: qsTr("Search presets")

      Keys.onDownPressed: listPane.listView.incrementCurrentIndex()
      Keys.onEnterPressed: root.commitCurrentOrFirst()
      Keys.onEscapePressed: root.dismiss()
      Keys.onReturnPressed: root.commitCurrentOrFirst()
      Keys.onTabPressed: {
        if (groupList.visible)
          groupList.forceActiveFocus();
      }
      Keys.onUpPressed: listPane.listView.decrementCurrentIndex()
    }

    RowLayout {
      Layout.fillHeight: true
      Layout.fillWidth: true
      spacing: 4

      ListView {
        id: groupList

        Layout.fillHeight: true
        Layout.preferredWidth: 160
        clip: true
        currentIndex: 0
        // Re-evaluating this binding when the source model's groups
        // change resets the selection to "All": a rebuilt group list can
        // make the previous filter meaningless, and a stale filter is
        // worse than a cleared one
        model: [
          {
            "count": root.model?.count ?? 0,
            "name": ""
          }
        ].concat(root.model?.groups ?? [])
        visible: root.multipleGroups

        // Interactive scrollbar; stays transient but is revealed on hover
        // so it can be dragged
        ScrollBar.vertical: ScrollBar {
          active: groupList.moving || hovered || pressed
          hoverEnabled: true
          policy: ScrollBar.AsNeeded
        }
        delegate: ItemDelegate {
          id: groupDelegate

          required property int index
          required property var modelData

          // Clicks must not steal keyboard focus from the search field,
          // or arrow-key navigation dies
          focusPolicy: Qt.NoFocus
          hoverEnabled: true
          width: ListView.view.width

          // Subtler than the accent highlight used for the preset list:
          // this marks the active filter, not the committable selection
          background: Rectangle {
            color: groupDelegate.hovered ? groupDelegate.palette.light : (groupDelegate.index === groupList.currentIndex ? groupDelegate.palette.mid : "transparent")
            radius: 4
          }
          contentItem: RowLayout {
            spacing: 6

            Label {
              Layout.fillWidth: true
              color: groupDelegate.index === groupList.currentIndex ? groupDelegate.palette.buttonText : groupDelegate.palette.text
              elide: Text.ElideRight
              text: groupDelegate.modelData.name === "" ? qsTr("All") : groupDelegate.modelData.name
            }

            Label {
              color: groupDelegate.palette.text
              font: ZrythmTheme.fadedTextFont
              opacity: 0.8
              text: groupDelegate.modelData.count
            }
          }

          onClicked: groupList.currentIndex = groupDelegate.index
        }

        // Keyboard navigation: Up/Down move the highlight (which filters
        // immediately, mirroring mouse clicks); Right/Tab return to the
        // search field
        Keys.onDownPressed: groupList.incrementCurrentIndex()
        Keys.onEnterPressed: root.commitCurrentOrFirst()
        Keys.onEscapePressed: root.dismiss()
        Keys.onReturnPressed: root.commitCurrentOrFirst()
        Keys.onRightPressed: searchField.forceActiveFocus()
        Keys.onTabPressed: searchField.forceActiveFocus()
        Keys.onUpPressed: groupList.decrementCurrentIndex()
        onCurrentIndexChanged: {
          root.currentGroup = groupList.model[groupList.currentIndex]?.name ?? "";
        }
      }

      PresetListPopup {
        id: listPane

        Layout.fillHeight: true
        Layout.fillWidth: true
        clickActivates: false
        model: filterProxy
        ownsKeyboardFocus: false
        showBackground: false
        showGroupHeaders: root.multipleGroups && root.currentGroup.length === 0
        textRole: "name"

        onActivated: proxyRow => root.commitRow(proxyRow)
        onCloseRequested: root.dismiss()
      }
    }

    RowLayout {
      Layout.fillWidth: true

      Label {
        Layout.fillWidth: true
        font: ZrythmTheme.fadedTextFont
        opacity: 0.8
        text: {
          let status = qsTr("%1 of %2 presets").arg(listPane.count).arg(root.model?.count ?? 0);
          if (root.loadedSourceRow >= 0 && root.model?.nameAt)
            status += qsTr(" · Loaded: %1").arg(root.model.nameAt(root.loadedSourceRow));
          return status;
        }
      }

      Button {
        // Mouse-only affordances: the actions stay keyboard-reachable
        // via Enter/Escape, and unfocusable buttons can never steal the
        // search field's focus (same pattern as the list delegates)
        focusPolicy: Qt.NoFocus
        objectName: "cancelButton"
        text: qsTr("Cancel")

        onClicked: root.dismiss()
      }

      Button {
        enabled: listPane.count > 0
        focusPolicy: Qt.NoFocus
        objectName: "okButton"
        text: qsTr("OK")

        onClicked: root.commitCurrentOrFirst()
      }
    }
  }

  // Shown when the filtered list has no entries (e.g. a search string
  // with no matches)
  Label {
    anchors.centerIn: parent
    font: ZrythmTheme.fadedTextFont
    objectName: "emptyStateLabel"
    text: qsTr("No matching presets")
    visible: listPane.count === 0
  }

  // Audition the navigated-to preset after a short debounce, so held-down
  // arrow keys don't apply every intermediate preset
  Connections {
    function onCurrentIndexChanged() {
      auditionDebounce.restart();
    }

    target: listPane.listView
  }

  Timer {
    id: auditionDebounce

    interval: 120

    onTriggered: root.applyPendingAudition()
  }

  component PresetRoleData: QtObject {
    property string group
    property string name
  }
}
