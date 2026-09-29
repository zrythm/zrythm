// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle
import Qt.labs.synchronizer

Item {
  id: root

  readonly property int activeFilterCount: selectedTypes.length + selectedFormats.length + (favoritesOnly ? 1 : 0)
  // Chip entries for the active spec filters: { key, label, isType, isFavorites }
  readonly property var activeSpecEntries: {
    const typeLabels = {
      "instrument": qsTr("Instrument"),
      "effect": qsTr("Effect"),
      "midi": qsTr("MIDI"),
      "modulator": qsTr("Modulator")
    };
    const specEntries = selectedTypes.map(key => ({
          "key": key,
          "label": typeLabels[key],
          "isType": true,
          "isFavorites": false
        })).concat(selectedFormats.map(fmt => ({
          "key": fmt,
          "label": fmt,
          "isType": false,
          "isFavorites": false
        })));
    return favoritesOnly ? [
      {
        "key": "favorites",
        "label": qsTr("Favorites"),
        "isType": false,
        "isFavorites": true
      }
    ].concat(specEntries) : specEntries;
  }
  required property AppSettings appSettings
  readonly property color fadedTextColor: QmlUtils.adjustOpacity(palette.text, 0.6)
  // Favorited plugin descriptors, as "format:uniqueId" keys, persisted in
  // the app settings via the Synchronizer below
  property list<string> favoriteKeys: []
  // Whether the list is narrowed down to favorited plugins (view state,
  // not persisted)
  property bool favoritesOnly: false
  // The format list is non-empty whenever at least one plugin is known
  readonly property bool hasScannedPlugins: pluginManager.pluginDescriptors.availableFormats.length > 0
  required property PluginManager pluginManager
  readonly property bool scanInProgress: pluginManager.currentlyScanningPlugin.length > 0
  property list<string> selectedFormats: []

  // Selected plugin type spec keys ("instrument", "effect", "midi", "modulator")
  // and plugin format names (subset of pluginDescriptors.availableFormats);
  // both persisted in the app settings
  property list<string> selectedTypes: []

  signal pluginDescriptorActivated(PluginDescriptor descriptor)

  function clearSpecFilters() {
    selectedTypes = [];
    selectedFormats = [];
    favoritesOnly = false;
  }

  function descriptorFavoriteKey(descriptor: PluginDescriptor): string {
    return descriptor.format + ":" + descriptor.uniqueId;
  }

  function toggleFavorite(descriptor: PluginDescriptor) {
    const key = descriptorFavoriteKey(descriptor);
    favoriteKeys = favoriteKeys.includes(key) ? favoriteKeys.filter(k => k !== key) : favoriteKeys.concat([key]);
  }

  function toggleFormatFilter(format: string, selected: bool) {
    selectedFormats = selected ? selectedFormats.concat([format]) : selectedFormats.filter(f => f !== format);
  }

  function toggleTypeFilter(key: string, selected: bool) {
    selectedTypes = selected ? selectedTypes.concat([key]) : selectedTypes.filter(k => k !== key);
  }

  Synchronizer on favoriteKeys {
    sourceObject: root.appSettings
    sourceProperty: "pluginBrowserFavorites"
  }
  Synchronizer on selectedFormats {
    sourceObject: root.appSettings
    sourceProperty: "pluginBrowserSpecFormats"
  }
  Synchronizer on selectedTypes {
    sourceObject: root.appSettings
    sourceProperty: "pluginBrowserSpecTypes"
  }

  // Drop selected formats that are no longer present (e.g. after a rescan)
  Connections {
    function onAvailableFormatsChanged() {
      const available = root.pluginManager.pluginDescriptors.availableFormats;
      root.selectedFormats = root.selectedFormats.filter(fmt => available.includes(fmt));
    }

    target: root.pluginManager.pluginDescriptors
  }

  SortFilterProxyModel {
    id: pluginFilter

    model: root.pluginManager.pluginDescriptors

    filters: [
      FunctionFilter {
        property string searchText: pluginSearch.text

        function filter(data: RoleData): bool {
          return data.name.toLowerCase().includes(searchText.toLowerCase());
        }

        onSearchTextChanged: invalidate()
      },
      FunctionFilter {
        id: specFilter

        property var favoriteKeys: root.favoriteKeys
        property bool favoritesOnly: root.favoritesOnly
        property list<string> selectedFormats: root.selectedFormats
        property list<string> selectedTypes: root.selectedTypes

        function filter(data: RoleData): bool {
          const d = data.descriptor;
          if (favoritesOnly && !favoriteKeys.includes(root.descriptorFavoriteKey(d)))
            return false;
          if (selectedFormats.length > 0 && !selectedFormats.includes(d.format))
            return false;
          if (selectedTypes.length === 0)
            return true;
          return (selectedTypes.includes("instrument") && d.isInstrument()) || (selectedTypes.includes("effect") && d.isEffect()) || (selectedTypes.includes("midi") && d.isMidiModifier()) || (selectedTypes.includes("modulator") && d.isModulator());
        }

        onFavoriteKeysChanged: invalidate()
        onFavoritesOnlyChanged: invalidate()
        onSelectedFormatsChanged: invalidate()
        onSelectedTypesChanged: invalidate()
      }
    ]
    sorters: [
      RoleSorter {
        priority: 0
        roleName: "name"
      }
    ]
  }

  DescriptorDragItem {
    id: draggable
  }

  ColumnLayout {
    anchors.fill: parent

    RowLayout {
      Layout.fillWidth: true
      spacing: 4

      ZrythmSearchField {
        id: pluginSearch

        Layout.fillWidth: true
        placeholderText: qsTr("Search plugins")

        Synchronizer on text {
          sourceObject: root.appSettings
          sourceProperty: "pluginBrowserSearchText"
        }
      }

      ToolButton {
        id: filterButton

        Accessible.name: qsTr("Plugin Spec Filters")
        display: AbstractButton.TextBesideIcon
        icon.height: 12
        icon.source: ResourceManager.getIconUrl("noto-glyphs", "triangle-down.svg")
        icon.width: 12
        text: qsTr("Filters")

        onClicked: filterPopup.open()

        // Tint text and icon while any spec filter is active
        palette {
          buttonText: root.activeFilterCount > 0 ? ZrythmTheme.colorPalette.accent : ZrythmTheme.colorPalette.buttonText
        }

        Popup {
          id: filterPopup

          focus: true
          padding: 8
          x: filterButton.width - width
          y: filterButton.height

          contentItem: ScrollView {
            id: filterScrollView

            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
            implicitHeight: Math.min(filterColumn.implicitHeight, 360)
            implicitWidth: filterColumn.implicitWidth

            ColumnLayout {
              id: filterColumn

              spacing: 2
              width: filterScrollView.availableWidth

              RowLayout {
                Layout.fillWidth: true

                Label {
                  font.bold: true
                  text: qsTr("Filters")
                }

                Item {
                  Layout.fillWidth: true
                }

                Button {
                  enabled: root.activeFilterCount > 0
                  flat: true
                  text: qsTr("Clear")

                  onClicked: root.clearSpecFilters()

                  palette {
                    buttonText: ZrythmTheme.colorPalette.accent
                  }
                }
              }

              SpecFilterRow {
                selectedKeys: root.favoritesOnly ? ["favorites"] : []
                specKey: "favorites"
                text: qsTr("Favorites")

                onSpecToggled: (key, selected) => root.favoritesOnly = selected
              }

              MenuSeparator {
                Layout.fillWidth: true
              }

              Label {
                Layout.topMargin: 4
                color: root.fadedTextColor
                font: ZrythmTheme.smallTextFont
                text: qsTr("Type")
              }

              SpecFilterRow {
                selectedKeys: root.selectedTypes
                specKey: "instrument"
                text: qsTr("Instrument")

                onSpecToggled: (key, selected) => root.toggleTypeFilter(key, selected)
              }

              SpecFilterRow {
                selectedKeys: root.selectedTypes
                specKey: "effect"
                text: qsTr("Effect")

                onSpecToggled: (key, selected) => root.toggleTypeFilter(key, selected)
              }

              SpecFilterRow {
                selectedKeys: root.selectedTypes
                specKey: "midi"
                text: qsTr("MIDI")

                onSpecToggled: (key, selected) => root.toggleTypeFilter(key, selected)
              }

              SpecFilterRow {
                selectedKeys: root.selectedTypes
                specKey: "modulator"
                text: qsTr("Modulator")

                onSpecToggled: (key, selected) => root.toggleTypeFilter(key, selected)
              }

              MenuSeparator {
                Layout.fillWidth: true
              }

              Label {
                color: root.fadedTextColor
                font: ZrythmTheme.smallTextFont
                text: qsTr("Format")
              }

              Repeater {
                model: root.pluginManager.pluginDescriptors.availableFormats

                delegate: SpecFilterRow {
                  required property string modelData

                  selectedKeys: root.selectedFormats
                  specKey: modelData
                  text: modelData

                  onSpecToggled: (key, selected) => root.toggleFormatFilter(key, selected)
                }
              }
            }
          }
        }
      }
    }

    // Removable chips for the active spec filters
    Flow {
      Layout.fillWidth: true
      spacing: 4
      visible: root.activeFilterCount > 0

      Repeater {
        model: root.activeSpecEntries

        delegate: SpecFilterChip {
          required property var modelData

          text: modelData.label

          onRemoveClicked: {
            if (modelData.isFavorites) {
              root.favoritesOnly = false;
            } else if (modelData.isType) {
              root.toggleTypeFilter(modelData.key, false);
            } else {
              root.toggleFormatFilter(modelData.key, false);
            }
          }
        }
      }
    }

    ListView {
      id: pluginListView

      readonly property var selectionModel: ItemSelectionModel {
        model: pluginListView.model
      }

      Layout.fillHeight: true
      Layout.fillWidth: true
      activeFocusOnTab: true
      boundsBehavior: Flickable.StopAtBounds
      clip: true
      focus: true
      model: pluginFilter
      visible: pluginListView.count > 0

      ScrollBar.vertical: ScrollBar {
      }
      delegate: PluginDescriptorRow {
        width: pluginListView.width
      }

      Keys.onDownPressed: incrementCurrentIndex()
      Keys.onUpPressed: decrementCurrentIndex()

      Binding {
        property: "descriptor"
        target: pluginInfoLabel
        value: pluginListView.currentItem ? (pluginListView.currentItem as PluginDescriptorRow).descriptor : null
      }
    }

    // Empty state (shown instead of the list when there is nothing to show)
    ColumnLayout {
      Layout.fillHeight: true
      Layout.fillWidth: true
      spacing: 8
      visible: pluginListView.count === 0

      Item {
        Layout.fillHeight: true
      }

      Label {
        Layout.fillWidth: true
        Layout.leftMargin: 8
        Layout.rightMargin: 8
        color: root.palette.text
        font: ZrythmTheme.semiBoldTextFont
        horizontalAlignment: Text.AlignHCenter
        text: {
          if (root.hasScannedPlugins)
            return qsTr("No Plugins Match");
          return root.scanInProgress ? qsTr("Scanning Plugins…") : qsTr("No Plugins Found");
        }
        wrapMode: Text.WordWrap
      }

      Label {
        Layout.fillWidth: true
        Layout.leftMargin: 8
        Layout.rightMargin: 8
        color: root.fadedTextColor
        font: ZrythmTheme.smallTextFont
        horizontalAlignment: Text.AlignHCenter
        text: {
          if (root.hasScannedPlugins)
            return qsTr("Try a different search or clear the active filters.");
          if (root.scanInProgress)
            return root.pluginManager.currentlyScanningPlugin;
          return qsTr("Scan your system for audio plugins to get started.");
        }
        wrapMode: Text.WordWrap
      }

      // If plugins exist but the list is empty, the search text or spec
      // filters necessarily excluded everything
      Button {
        Layout.alignment: Qt.AlignHCenter
        flat: true
        text: qsTr("Clear Filters")
        visible: root.hasScannedPlugins

        onClicked: {
          root.clearSpecFilters();
          pluginSearch.text = "";
        }

        palette {
          buttonText: ZrythmTheme.colorPalette.accent
        }
      }

      Button {
        Layout.alignment: Qt.AlignHCenter
        flat: true
        text: qsTr("Scan for Plugins")
        visible: !root.hasScannedPlugins && !root.scanInProgress

        onClicked: root.pluginManager.beginScan()
      }

      Item {
        Layout.fillHeight: true
      }
    }

    // Plugin Description Area
    ColumnLayout {
      id: pluginInfoLabel

      property PluginDescriptor descriptor
      readonly property color fadedColor: QmlUtils.adjustOpacity(palette.text, 0.6)

      Layout.leftMargin: 4
      Layout.rightMargin: 4
      // hide when there is nothing valid to describe (empty filtered list or
      // no selection yet)
      visible: pluginListView.count > 0 && pluginListView.currentItem !== null

      ColumnLayout {
        spacing: 1

        // Plugin author and type labels
        Label {
          id: pluginAuthorLabel

          Layout.fillWidth: true
          elide: Text.ElideRight
          font.bold: true
          horizontalAlignment: Text.AlignHCenter
          text: pluginInfoLabel.descriptor ? pluginInfoLabel.descriptor.name : ""
        }

        Label {
          id: pluginTypeLabel

          Layout.fillWidth: true
          color: pluginInfoLabel.fadedColor
          font.bold: true
          horizontalAlignment: Text.AlignHCenter
          text: pluginInfoLabel.descriptor ? pluginInfoLabel.descriptor.category + " • " + pluginInfoLabel.descriptor.format : ""
        }
      }

      // Plugin key-value grid
      GridLayout {
        Layout.fillWidth: true
        columns: 2
        rowSpacing: 4

        // Vendor
        Label {
          Layout.fillWidth: false
          color: pluginInfoLabel.fadedColor
          horizontalAlignment: Text.AlignRight
          text: qsTr("Vendor")
        }

        Label {
          id: plugin_audio_label

          Layout.fillWidth: true
          elide: Text.ElideRight
          horizontalAlignment: Text.AlignLeft
          text: pluginInfoLabel.descriptor ? pluginInfoLabel.descriptor.vendor : ""
        }
      }
    }
  }

  PluginInfoDialog {
    id: pluginInfoDialog
  }

  component PluginDescriptorRow: ActionRow {
    id: pluginDescriptorRow

    required property PluginDescriptor descriptor
    readonly property bool favorited: root.favoriteKeys.includes(descriptor.format + ":" + descriptor.uniqueId)
    required property int index

    alternate: index % 2 === 1
    highlighted: ListView.isCurrentItem
    icon.source: {
      if (descriptor.isInstrument()) {
        return ResourceManager.getIconUrl("zrythm-dark", "instrument.svg");
      } else if (descriptor.isMidiModifier()) {
        return ResourceManager.getIconUrl("zrythm-dark", "signal-midi.svg");
      } else {
        return ResourceManager.getIconUrl("zrythm-dark", "audio-insert.svg");
      }
    }
    interactive: true
    subtitle: descriptor.vendor
    subtitleFallback: descriptor.format
    suffixActions: [favoriteAction, insertAction]
    title: descriptor.name
    width: pluginListView.width

    overflowActions: [
      Action {
        enabled: pluginDescriptorRow.descriptor.pathOrId !== ""
        text: qsTr("Show in File Manager")

        onTriggered: QmlUtils.revealInFileManager(pluginDescriptorRow.descriptor.pathOrId)
      },
      Action {
        text: qsTr("Plugin Info")

        onTriggered: pluginInfoDialog.openFor(pluginDescriptorRow.descriptor)
      }
    ]

    // Non-visual children (actions, handlers) go in `resources` — the
    // component's default property routes plain children into the suffix
    // controls area
    resources: [
      Action {
        id: favoriteAction

        // checked mirrors the persisted favorites list; the action itself
        // is not checkable so the binding cannot be broken by clicks
        checked: pluginDescriptorRow.favorited
        icon.source: ResourceManager.getIconUrl("gnome-icon-library", pluginDescriptorRow.favorited ? "star-large-symbolic.svg" : "star-outline-rounded-symbolic.svg")
        text: qsTr("Favorite")

        onTriggered: root.toggleFavorite(pluginDescriptorRow.descriptor)
      },
      Action {
        id: insertAction

        icon.source: ResourceManager.getIconUrl("gnome-icon-library", "arrow-into-box-symbolic.svg")
        text: qsTr("Import")

        onTriggered: root.pluginDescriptorActivated(pluginDescriptorRow.descriptor)
      },
      DragHandler {
        id: dragHandler

        target: null

        onActiveChanged: {
          if (active) {
            pluginListView.currentIndex = pluginDescriptorRow.index;
            draggable.descriptor = pluginDescriptorRow.descriptor;
            draggable.Drag.active = true;
          } else {
            draggable.Drag.active = false;
          }
        }
      },
      TapHandler {
        onDoubleTapped: root.pluginDescriptorActivated(pluginDescriptorRow.descriptor)
      }
    ]

    onClicked: pluginListView.currentIndex = pluginDescriptorRow.index
  }

  // Dialog showing a plugin descriptor's metadata
  component PluginInfoDialog: Dialog {
    id: infoDialog

    property PluginDescriptor descriptor

    function openFor(descriptor: PluginDescriptor) {
      infoDialog.descriptor = descriptor;
      open();
    }

    anchors.centerIn: Overlay.overlay
    implicitWidth: 440
    modal: true
    standardButtons: Dialog.Close
    title: descriptor?.name ?? ""

    ColumnLayout {
      spacing: 8

      GridLayout {
        columnSpacing: 16
        columns: 2
        rowSpacing: 4

        Label {
          Layout.alignment: Qt.AlignRight
          color: QmlUtils.adjustOpacity(infoDialog.palette.text, 0.6)
          text: qsTr("Vendor")
        }

        Label {
          Layout.fillWidth: true
          elide: Text.ElideRight
          text: infoDialog.descriptor?.vendor ?? ""
        }

        Label {
          Layout.alignment: Qt.AlignRight
          color: QmlUtils.adjustOpacity(infoDialog.palette.text, 0.6)
          text: qsTr("Format")
        }

        Label {
          Layout.fillWidth: true
          text: infoDialog.descriptor?.format ?? ""
        }

        Label {
          Layout.alignment: Qt.AlignRight
          color: QmlUtils.adjustOpacity(infoDialog.palette.text, 0.6)
          text: qsTr("Category")
        }

        Label {
          Layout.fillWidth: true
          elide: Text.ElideRight
          text: infoDialog.descriptor?.category ?? ""
        }

        Label {
          Layout.alignment: Qt.AlignRight
          color: QmlUtils.adjustOpacity(infoDialog.palette.text, 0.6)
          text: qsTr("Location")
        }

        Label {
          Layout.fillWidth: true
          elide: Text.ElideMiddle
          text: infoDialog.descriptor?.pathOrId ?? ""
          visible: (infoDialog.descriptor?.pathOrId ?? "") !== ""
        }
      }
    }
  }
  component RoleData: QtObject {
    property PluginDescriptor descriptor
    property string name
  }
  // Removable lozenge showing one active spec filter
  component SpecFilterChip: Rectangle {
    id: specFilterChip

    required property string text

    signal removeClicked

    color: QmlUtils.adjustOpacity(specFilterChip.palette.accent, 0.25)
    implicitHeight: chipRow.implicitHeight + 4
    implicitWidth: chipRow.implicitWidth + 8
    radius: height / 2

    border {
      color: QmlUtils.adjustOpacity(specFilterChip.palette.accent, 0.5)
      width: 1
    }

    RowLayout {
      id: chipRow

      anchors.centerIn: parent
      spacing: 2

      Label {
        Layout.leftMargin: 4
        color: specFilterChip.palette.text
        font: ZrythmTheme.smallTextFont
        text: specFilterChip.text
      }

      ToolButton {
        Accessible.name: qsTr("Remove Filter")
        display: AbstractButton.IconOnly
        focusPolicy: Qt.NoFocus
        icon.height: 8
        icon.source: ResourceManager.getIconUrl("noto-glyphs", "close.svg")
        icon.width: 8
        implicitHeight: 16
        implicitWidth: 16

        onClicked: specFilterChip.removeClicked()
      }
    }
  }
  // Full-width checkable row for one spec value, with a trailing check
  // indicator
  component SpecFilterRow: ItemDelegate {
    id: specFilterRow

    // The selected spec keys this row reflects
    required property list<string> selectedKeys
    // The spec key this row represents (e.g. "instrument" or a format name)
    required property string specKey

    // Emitted when the user toggles the row; the host applies the change to
    // the selected keys
    signal specToggled(string specKey, bool selected)

    Layout.fillWidth: true
    checked: selectedKeys.includes(specKey)

    contentItem: RowLayout {
      spacing: 6

      Label {
        Layout.fillWidth: true
        color: specFilterRow.palette.text
        elide: Text.ElideRight
        font: specFilterRow.font
        text: specFilterRow.text
      }

      CheckIndicator {
        checked: specFilterRow.checked
        down: specFilterRow.down
        hovered: specFilterRow.hovered
        visualFocus: specFilterRow.visualFocus
      }
    }

    // not checkable: checked is purely driven by selectedKeys above - the row
    // only reports the toggle intent and the host applies it
    onClicked: specToggled(specKey, !checked)
  }
}
