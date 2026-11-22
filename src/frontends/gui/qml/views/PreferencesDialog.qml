// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmStyle

Dialog {
  id: root

  property AppSettings appSettings: null

  // a binding (not ListModel/ListElement) so engine retranslation refreshes
  // the titles
  readonly property var categories: [
    { iconFile: "display-symbolic.svg", title: qsTr("Appearance") },
    { iconFile: "sound-wave-symbolic.svg", title: qsTr("Playback & Mixing") },
    { iconFile: "pencil-symbolic.svg", title: qsTr("Editing") },
    { iconFile: "puzzle-piece-symbolic.svg", title: qsTr("Plugins") },
    { iconFile: "terminal-symbolic.svg", title: qsTr("Advanced") }
  ]

  readonly property var visibleSearchEntries: {
    const query = searchField.text.trim().toLowerCase();
    if (query.length === 0)
      return [];
    return collectSearchEntries(query);
  }

  height: 600
  modal: false
  popupType: Popup.Window
  standardButtons: Dialog.NoButton
  title: qsTr("Preferences")
  width: 840

  ColumnLayout {
    anchors.fill: parent
    spacing: ZrythmTheme.buttonPadding

    Keys.onEscapePressed: root.resetSearchOrClose()

    ZrythmSearchField {
      id: searchField

      Layout.fillWidth: true
      objectName: "preferencesSearchField"
      placeholderText: qsTr("Search preferences")

      Keys.onDownPressed: (event) => {
        if (root.visibleSearchEntries.length > 0) {
          searchResults.focusFirstResult();
          event.accepted = true;
        }
      }

      Keys.onEnterPressed: root.selectSearchResult()
      Keys.onEscapePressed: root.resetSearchOrClose()
      Keys.onReturnPressed: root.selectSearchResult()
    }

    RowLayout {
      Layout.fillHeight: true
      Layout.fillWidth: true
      spacing: 12

      Rectangle {
        id: categoryPane

        Layout.fillHeight: true
        Layout.preferredWidth: 180
        color: ZrythmTheme.alternateBackgroundColor
        radius: ZrythmTheme.textFieldRadius

        ListView {
          id: categoryList

          anchors.fill: parent
          anchors.margins: ZrythmTheme.buttonPadding
          boundsBehavior: Flickable.StopAtBounds
          clip: true
          keyNavigationWraps: true
          model: root.categories
          spacing: ZrythmTheme.buttonPadding

          delegate: ItemDelegate {
            id: categoryDelegate

            required property int index
            required property var modelData

            display: AbstractButton.TextBesideIcon
            highlighted: categoryList.currentIndex === categoryDelegate.index
            icon.height: 16
            icon.source: ResourceManager.getIconUrl("gnome-icon-library", categoryDelegate.modelData.iconFile)
            icon.width: 16
            text: categoryDelegate.modelData.title
            width: categoryList.width

            onClicked: {
              searchField.clear();
              categoryList.currentIndex = categoryDelegate.index;
            }
          }
        }
      }

      PreferencesSearchResultsPage {
        id: searchResults

        Layout.fillHeight: true
        Layout.fillWidth: true
        entries: root.visibleSearchEntries
        visible: searchField.text.trim().length > 0

        onResultSelected: (categoryIndex) => {
          searchField.clear();
          categoryList.currentIndex = categoryIndex;
        }
      }

      StackLayout {
        id: pageStack

        Layout.fillHeight: true
        Layout.fillWidth: true
        currentIndex: categoryList.currentIndex
        visible: searchField.text.trim().length === 0

        PreferencesAppearancePage {
          Layout.fillHeight: true
          Layout.fillWidth: true
          appSettings: root.appSettings
        }

        PreferencesPlaybackMixingPage {
          Layout.fillHeight: true
          Layout.fillWidth: true
          appSettings: root.appSettings
        }

        PreferencesEditingPage {
          Layout.fillHeight: true
          Layout.fillWidth: true
          appSettings: root.appSettings
        }

        PreferencesPluginsPage {
          Layout.fillHeight: true
          Layout.fillWidth: true
          appSettings: root.appSettings
        }

        PreferencesAdvancedPage {
          Layout.fillHeight: true
          Layout.fillWidth: true
          appSettings: root.appSettings
        }
      }
    }
  }

  function resetSearchOrClose(): void {
    if (searchField.text.length > 0) {
      searchField.clear();
      searchField.forceInputFocus();
    } else {
      root.close();
    }
  }

  function collectActionRowGroups(item: Item, groups: var): void {
    for (let index = 0; index < item.children.length; ++index) {
      const child = item.children[index];
      if (child instanceof ActionRowGroup)
        groups.push(child);
      collectActionRowGroups(child, groups);
    }
  }

  function collectActionRows(item: Item, rows: var): void {
    for (let index = 0; index < item.children.length; ++index) {
      const child = item.children[index];
      if (child instanceof ActionRow) {
        rows.push(child);
        continue;
      }
      collectActionRows(child, rows);
    }
  }

  function collectSearchEntries(query: string): var {
    const terms = query.split(" ");
    const entries = [];
    for (let categoryIndex = 0; categoryIndex < pageStack.count; ++categoryIndex) {
      const categoryTitle = root.categories[categoryIndex].title;
      const groups = [];
      for (const contentChild of pageStack.children[categoryIndex].contentChildren)
        collectActionRowGroups(contentChild, groups);
      for (const group of groups) {
        const rows = [];
        collectActionRows(group, rows);
        for (const row of rows) {
          const haystack = [
            categoryTitle,
            group.title,
            row.title,
            row.subtitle
          ].join(" ").toLowerCase();
          if (terms.every((term) => haystack.includes(term))) {
            entries.push({
              "categoryIndex": categoryIndex,
              "categoryTitle": categoryTitle,
              "group": group.title,
              "title": row.title,
              "subtitle": row.subtitle
            });
          }
        }
      }
    }
    return entries;
  }

  function selectSearchResult(): void {
    searchResults.selectCurrentResult();
  }
}
