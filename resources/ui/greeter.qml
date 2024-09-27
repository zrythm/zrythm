import QtQuick 6.7
import QtQuick.Controls 6.7
import QtQuick.Layouts 6.7
import org.kde.kirigami as Kirigami
import "qrc:/config.js" as Config

ApplicationWindow {
    id: greeterWidget

    title: "Zrythm"
    modality: Qt.ApplicationModal
    minimumWidth: 256
    width: 640
    height: 480
    visible: true
    Component.onCompleted: {
        Qt.application.font.family = interFontItalic.name;
        Qt.application.font.pixelSize = 16;
    }

    FontLoader {
        id: dsegRegular

        source: "qrc:/fonts/DSEG14ClassicMini-Regular.ttf"
    }

    FontLoader {
        id: dsegBold

        source: "qrc:/fonts/DSEG14ClassicMini-Bold.ttf"
    }

    FontLoader {
        id: interFont

        source: "qrc:/fonts/InterVariable.ttf"
    }

    FontLoader {
        id: interFontItalic

        source: "qrc:/fonts/InterVariable-Italic.ttf"
    }

    StackView {
        id: stack

        anchors.fill: parent
        initialItem: settingsManager.first_run ? firstRunPage : progressPage

        Component {
            id: firstRunPage

            Page {
                title: qsTr("Welcome")

                SwipeView {
                    id: welcomeCarousel

                    anchors.fill: parent
                    clip: true

                    Item {
                        Kirigami.PlaceholderMessage {
                            anchors.centerIn: parent
                            width: parent.width - (Kirigami.Units.largeSpacing * 4)
                            // visible: root.loading
                            icon.source: "qrc:/icons/zrythm.svg"
                            text: qsTr("Welcome")
                            explanation: qsTr("Welcome to the Zrythm digital audio workstation. Move to the next page to get started.")
                        }

                    }

                    Item {
                        Kirigami.PlaceholderMessage {
                            anchors.centerIn: parent
                            width: parent.width - (Kirigami.Units.largeSpacing * 4)
                            icon.source: "qrc:/icons/gnome-icon-library/open-book-symbolic.svg"
                            text: qsTr("Read the Manual")
                            explanation: qsTr("If this is your first time using Zrythm, we suggest going through the 'Getting Started' section in the %1user manual%2.").arg("<a href=\"" + Config.USER_MANUAL_URL + "\">").arg("</a>")
                            onLinkActivated: (link) => {
                                console.log("Opening link: " + link);
                                return Qt.openUrlExternally(link);
                            }
                        }

                    }

                    Loader {
                        active: !Config.IS_INSTALLER_VER || Config.IS_TRIAL_VER

                        sourceComponent: Item {
                            Kirigami.PlaceholderMessage {
                                anchors.centerIn: parent
                                width: parent.width - (Kirigami.Units.largeSpacing * 4)
                                icon.source: "qrc:/icons/gnome-icon-library/credit-card-symbolic.svg"
                                text: qsTr("Donate")
                                explanation: qsTr("Zrythm relies on donations and purchases to sustain development. If you enjoy the software, please consider %1donating%2 or %3buying an installer%2.").arg("<a href=\"" + Config.DONATION_URL + "\">").arg("</a>").arg("<a href=\"" + Config.PURCHASE_URL + "\">")
                                onLinkActivated: (link) => {
                                    console.log("Opening link: " + link);
                                    return Qt.openUrlExternally(link);
                                }
                            }

                        }

                    }

                    Item {
                        Kirigami.PlaceholderMessage {
                            anchors.centerIn: parent
                            width: parent.width - (Kirigami.Units.largeSpacing * 4)
                            icon.source: "qrc:/icons/gnome-icon-library/flatpak-symbolic.svg"
                            text: qsTr("About Flatpak")
                        }

                    }

                    Item {
                        Kirigami.PlaceholderMessage {
                            anchors.centerIn: parent
                            width: parent.width - (Kirigami.Units.largeSpacing * 4)
                            text: qsTr("Proceed to Configuration")

                            helpfulAction: Kirigami.Action {
                                text: qsTr("Proceed")
                                onTriggered: stack.push(configPage)
                            }

                        }

                    }

                }

                // Add left navigation button
                RoundButton {
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    text: "<"
                    onClicked: welcomeCarousel.decrementCurrentIndex()
                    visible: welcomeCarousel.currentIndex > 0
                    font.pixelSize: 18
                }

                // Add right navigation button
                RoundButton {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: ">"
                    onClicked: welcomeCarousel.incrementCurrentIndex()
                    visible: welcomeCarousel.currentIndex < welcomeCarousel.count - 1
                    font.pixelSize: 18
                }

                PageIndicator {
                    id: indicator

                    count: welcomeCarousel.count
                    currentIndex: welcomeCarousel.currentIndex
                    anchors.bottom: welcomeCarousel.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    interactive: true
                    onCurrentIndexChanged: welcomeCarousel.currentIndex = currentIndex
                }

            }

        }

        Component {
            id: configPage

            Page {
                // Add configuration content here

                title: qsTr("Configuration")

                header: ToolBar {
                    RowLayout {
                        anchors.fill: parent

                        Label {
                            text: qsTr("Configuration")
                            elide: Text.ElideRight
                            horizontalAlignment: Qt.AlignHCenter
                            verticalAlignment: Qt.AlignVCenter
                            Layout.fillWidth: true
                        }

                    }

                }

                footer: ToolBar {
                    RowLayout {
                        anchors.fill: parent

                        Item {
                            Layout.fillWidth: true
                        }

                        Button {
                            text: qsTr("Reset")
                        }

                        Button {
                            text: qsTr("Continue")
                            highlighted: true
                        }

                    }

                }

            }

        }

        Component {
            id: progressPage

            Page {
                title: qsTr("Progress")

                BusyIndicator {
                    anchors.centerIn: parent
                    running: true
                }

                Label {
                    anchors.top: parent.top
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Zrythm"
                    font.pixelSize: 24
                }

                ProgressBar {
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 20
                    indeterminate: true
                }

            }

        }

        Component {
            id: projectSelectorPage

            Page {
                title: qsTr("Open a Project")

                ListView {
                    // Add recent projects here

                    anchors.fill: parent

                    model: ListModel {
                    }

                    delegate: ItemDelegate {
                        //   text: model.name
                        width: parent.width
                    }

                }

                header: ToolBar {
                    RowLayout {
                        anchors.fill: parent

                        Label {
                            text: qsTr("Open a Project")
                            elide: Text.ElideRight
                            horizontalAlignment: Qt.AlignHCenter
                            verticalAlignment: Qt.AlignVCenter
                            Layout.fillWidth: true
                        }

                        ToolButton {
                            //   icon.name: "open-menu-symbolic"
                            onClicked: menu.open()

                            Menu {
                                id: menu

                                MenuItem {
                                    // Handle preferences action

                                    text: qsTr("Preferences")
                                    onTriggered: {
                                    }
                                }

                                MenuItem {
                                    // Handle about action

                                    text: qsTr("About Zrythm")
                                    onTriggered: {
                                    }
                                }

                            }

                        }

                    }

                }

                footer: ToolBar {
                    RowLayout {
                        anchors.fill: parent

                        Button {
                            //   onClicked: stack.push(createProjectPage)

                            text: qsTr("Create New Project...")
                        }

                        Button {
                            text: qsTr("Open From Path...")
                        }

                    }

                }

            }

        }

        Component {
            id: createProjectPage

            Page {
                title: qsTr("Create New Project")

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10

                    TextField {
                        Layout.fillWidth: true
                        placeholderText: qsTr("Project Name")
                    }

                    TextField {
                        Layout.fillWidth: true
                        placeholderText: qsTr("Parent Directory")
                    }

                    ComboBox {
                        // Add templates here

                        Layout.fillWidth: true

                        model: ListModel {
                        }

                    }

                    Button {
                        text: qsTr("Create Project")
                        Layout.alignment: Qt.AlignHCenter
                    }

                }

            }

        }

    }

}
