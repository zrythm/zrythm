// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import "../config.js" as Config
import QtQuick
import QtQuick.Controls
import Zrythm 1.0
import ZrythmStyle 1.0

MenuBar {
    id: root

    required property var project
    required property var deviceManager

    Menu {
        title: qsTr("&File")

        Action {
            text: qsTr("New Long Long Long Long Long Long Long Long Long Name")
        }

        Action {
            text: qsTr("Open")
        }

    }

    Menu {
        title: qsTr("&Edit")

        Action {
            text: qsTr("Undo")
        }

    }

    Menu {
        title: qsTr("&View")

        Menu {
            title: qsTr("Language")

            MenuItem {
                id: systemLocaleMenuItem

                text: qsTr("System")
                onTriggered: {
                    GlobalState.translationManager.loadTranslation("");
                }
                checkable: true
                checked: GlobalState.settingsManager.uiLocale === ""
            }

            MenuSeparator {
            }

            Repeater {
                model: Object.keys(Config.languageMap).map((code) => {
                    return ({
                        "code": code,
                        "name": Config.languageMap[code]
                    });
                })

                delegate: MenuItem {
                    id: localeMenuItem

                    required property string name
                    required property string code

                    text: name
                    checkable: true
                    onTriggered: {
                        GlobalState.translationManager.loadTranslation(code);
                    }
                    checked: GlobalState.settingsManager.uiLocale === code
                }

            }

        }

        Menu {
            title: qsTr("Appearance")

            Action {
                text: qsTr("Switch Light/Dark Theme")
                icon.source: ResourceManager.getIconUrl("gnome-icon-library", "dark-mode-symbolic.svg")
                onTriggered: {
                    Style.darkMode = !Style.darkMode;
                }
            }

            Menu {
                title: qsTr("Theme Color")

                Action {
                    text: qsTr("Zrythm Orange")
                    onTriggered: {
                        Style.primaryColor = Style.zrythmColor;
                    }
                }

                Action {
                    text: qsTr("Celestial Blue")
                    onTriggered: {
                        Style.primaryColor = Style.celestialBlueColor;
                    }
                }

                Action {
                    text: qsTr("Jonquil Yellow")
                    onTriggered: {
                        Style.primaryColor = Style.jonquilYellowColor;
                    }
                }

            }

        }

    }

    Menu {
        title: qsTr("Devices")

        Action {
            text: qsTr("Audio/MIDI Setup")
            onTriggered: {
                root.deviceManager.showDeviceSelector();
            }
        }

    }

    Menu {
        title: qsTr("&Help")

        Action {
            text: qsTr("About Zrythm")
            onTriggered: {
            }
        }

    }

}
