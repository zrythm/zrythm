// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

// Host chrome header strip shown above plugin editors. Instantiated from C++
// with a Plugin as initial property (rendered offscreen by the plugin host
// windows), or declared in QML; theme state comes from the shared application
// engine (QML singletons are per-engine). The root's implicitHeight drives
// the native header strip height.
// No ToolTips when rendered in the native strip: in-scene popups get clipped
// by the strip height.
Rectangle {
  id: root

  // Margin around the controls row
  readonly property int contentMargin: 4
  // Whether the diagnostics row (DSP load + latency) is shown below the
  // controls; toggled by the disclosure button
  property bool detailsExpanded: false
  // Last polled DSP load (refreshed by the timer below)
  property double dspLoadPercent: 0
  // Engine sample rate for the ms conversion (0 when no project is loaded,
  // e.g. in test harnesses using a fallback engine)
  readonly property int engineSampleRate: GlobalState.application?.projectManager?.activeSession?.project?.engine?.sampleRate ?? 0
  // Reactively bound to the plugin's latencySamples property
  readonly property int latencySamples: root.plugin?.latencySamples ?? 0
  // True when rendered offscreen into a native plugin host window strip:
  // popups are then requested from the host (separate window) instead of
  // being shown in-scene, where the strip height would clip them
  readonly property bool nativeStrip: sceneController !== null

  // Non-null in normal operation; QML clears the reference when the plugin
  // is destroyed, which may precede this window's teardown — handlers guard
  // against that window
  required property Plugin plugin
  // OffscreenQmlScene hosting this scene offscreen, or null when used
  // in-scene (generic editor). The header calls its invokables to talk to
  // the host (popup requests, repaint requests)
  property var sceneController: null
  readonly property color themeTextColor: ZrythmTheme.textColor
  readonly property color themeWindowColor: ZrythmTheme.pageColor

  // Applies a selection coming back from an externally hosted popup
  // (untyped parameter: invoked from C++ via QMetaObject::invokeMethod)
  function applyPresetSelection(index) {
    if (!root.plugin)
      return;
    root.plugin.presetIndex = index;
  }

  function refreshDiagnostics() {
    dspLoadPercent = root.plugin?.dspLoadPercentage() ?? 0;
  }

  // Re-resolves the displayed preset name (invokable model lookups don't
  // participate in bindings, so this is called from explicit signal
  // handlers below). A "*" marks state that diverges from the preset
  function refreshPresetText() {
    const idx = root.plugin?.presetIndex ?? -1;
    let text = idx >= 0 ? presetModel.nameAt(idx) : "";
    if (idx >= 0 && (root.plugin?.presetDirty ?? false))
      text += "*";
    presetSelector.currentText = text;
    presetSelector.currentGroup = idx >= 0 ? presetModel.groupAt(idx) : "";
  }

  color: themeWindowColor
  implicitHeight: controlsRow.implicitHeight + 2 * contentMargin + (detailsExpanded ? detailsRow.implicitHeight + contentMargin : 0)
  implicitWidth: controlsRow.implicitWidth + disclosureButton.implicitWidth + 3 * contentMargin

  Component.onCompleted: refreshPresetText()
  onDetailsExpandedChanged: {
    if (detailsExpanded)
      refreshDiagnostics();
  }
  onThemeTextColorChanged: {
    if (sceneController)
      sceneController.scheduleRepaint();
  }
  onThemeWindowColorChanged: {
    if (sceneController)
      sceneController.scheduleRepaint();
  }

  // Offscreen-rendered strips don't inherit the main window's palette.
  // Bind each role instead of assigning the whole palette object:
  // whole-palette assignment copies the colors once and never follows
  // theme changes
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

  // Refresh immediately when the window re-appears with the row open
  // (the poll timer is stopped while hidden). Preset name display:
  // invokable model lookups don't participate in bindings, so refresh
  // explicitly on every relevant change
  Connections {
    function onPresetDirtyChanged() {
      root.refreshPresetText();
    }

    function onPresetIndexChanged() {
      root.refreshPresetText();
    }

    function onUiVisibleChanged() {
      if ((root.plugin?.uiVisible ?? false) && root.detailsExpanded)
        root.refreshDiagnostics();
    }

    target: root.plugin
  }

  Connections {
    function onModelReset() {
      root.refreshPresetText();
    }

    target: presetModel
  }

  ToolBar {
    id: controlsRow

    anchors.left: parent.left
    anchors.leftMargin: root.contentMargin
    // Top-anchored so the diagnostics row extends the strip downwards
    anchors.top: parent.top
    anchors.topMargin: root.contentMargin
    // The strip background is painted by the root rectangle
    background: null
    padding: 0

    contentItem: Row {
      spacing: 4

      ToolButton {
        id: bypassButton

        Accessible.name: qsTr("Bypass")
        checkable: true
        checked: root.plugin?.bypassed ?? false
        display: AbstractButton.IconOnly
        flat: true
        focusPolicy: Qt.NoFocus
        icon.source: ResourceManager.getIconUrl("noto-glyphs", "power.svg")

        onClicked: {
          if (!root.plugin)
            return;
          root.plugin.bypassed = !root.plugin.bypassed;
          // The control writes checked on click - restore the binding
          checked = Qt.binding(function () {
            return root.plugin?.bypassed ?? false;
          });
        }
      }

      ToolSeparator {
        anchors.verticalCenter: parent.verticalCenter
      }

      PresetSelector {
        id: presetSelector

        currentIndex: root.plugin?.presetIndex ?? -1
        enabled: presetModel.count > 0
        externalPopup: root.nativeStrip

        model: PluginPresetListModel {
          id: presetModel

          plugin: root.plugin
        }

        // The browser is wider than the windows this header appears in
        // (e.g. the generic editor), where an in-scene popup would be
        // clipped; unused in native strips (externalPopup above)
        popupType: Popup.Window

        onActivated: idx => {
          if (root.plugin)
            root.plugin.presetIndex = idx;
        }
        onPopupRequested: {
          if (!root.plugin)
            return;
          const topLeft = presetSelector.nameButton.mapToItem(root, 0, 0);
          root.sceneController.requestPresetPopup(Qt.rect(topLeft.x, topLeft.y, presetSelector.nameButton.width, presetSelector.nameButton.height), presetSelector.model, root.plugin.presetIndex);
        }
      }

      ToolButton {
        id: abButton

        Accessible.name: qsTr("Compare two plugin states")
        display: AbstractButton.IconOnly
        flat: true
        focusPolicy: Qt.NoFocus
        icon.color: root.themeTextColor
        icon.source: ResourceManager.getIconUrl("zrythm-dark", (root.plugin?.abActive ?? false) ? "preset-ba.svg" : "preset-ab.svg")
        text: qsTr("A/B")

        onClicked: root.plugin?.switchAbState()
      }
    }
  }

  // Disclosure toggle for the diagnostics row, right end of the strip
  ToolButton {
    id: disclosureButton

    Accessible.name: qsTr("Toggle plugin diagnostics")
    anchors.right: parent.right
    anchors.rightMargin: root.contentMargin
    anchors.verticalCenter: controlsRow.verticalCenter
    display: AbstractButton.IconOnly
    flat: true
    focusPolicy: Qt.NoFocus
    icon.color: root.themeTextColor
    icon.source: root.detailsExpanded ? ResourceManager.getIconUrl("gnome-icon-library", "go-up-symbolic.svg") : ResourceManager.getIconUrl("gnome-icon-library", "go-down-symbolic.svg")

    onClicked: root.detailsExpanded = !root.detailsExpanded
  }

  // Diagnostics line (DSP load + latency), shown on demand; a layout so
  // more diagnostics can be added later
  RowLayout {
    id: detailsRow

    anchors.left: parent.left
    anchors.leftMargin: root.contentMargin
    anchors.right: parent.right
    anchors.rightMargin: root.contentMargin
    anchors.top: controlsRow.bottom
    anchors.topMargin: root.contentMargin
    spacing: 8
    visible: root.detailsExpanded

    Label {
      id: detailsText

      Layout.fillWidth: true
      color: root.themeTextColor
      elide: Label.ElideRight
      opacity: 0.8
      text: {
        const loadText = Qt.locale().toString(root.dspLoadPercent, 'f', 1);
        if (root.engineSampleRate > 0) {
          const ms = Qt.locale().toString(root.latencySamples * 1000.0 / root.engineSampleRate, 'f', 1);
          return qsTr("Load: %1% · Latency: %2 samples (%3 ms)").arg(loadText).arg(root.latencySamples).arg(ms);
        }
        return qsTr("Load: %1% · Latency: %2 samples").arg(loadText).arg(root.latencySamples);
      }
    }
  }

  // Poll only while the diagnostics row is shown and the window is visible
  Timer {
    interval: 600
    repeat: true
    running: root.detailsExpanded && (root.plugin?.uiVisible ?? false)

    onTriggered: root.refreshDiagnostics()
  }
}
