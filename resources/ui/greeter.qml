import QtQuick 6.7
import QtQuick.Controls 6.7
import QtQuick.Layouts 6.7

ApplicationWindow {
    id: greeterWidget

    title: qsTr("Greeter")
    modality: Qt.ApplicationModal
    minimumWidth: 256
    visible: true

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
                        Image {
                            //   source: "qrc:/icons/zrythm.svg"

                            anchors.centerIn: parent
                        }

                        Label {
                            anchors.centerIn: parent
                            text: qsTr("Welcome to the Zrythm digital audio workstation. Move to the next page to proceed.")
                            wrapMode: Text.WordWrap
                            horizontalAlignment: Text.AlignHCenter
                        }

                    }

                    Item {
                        Image {
                            //   source: "qrc:/icons/open-book-symbolic.svg"

                            anchors.centerIn: parent
                        }

                        Label {
                            anchors.centerIn: parent
                            text: qsTr("Read the Manual")
                            wrapMode: Text.WordWrap
                            horizontalAlignment: Text.AlignHCenter
                        }

                    }

                    Item {
                        Image {
                            //   source: "qrc:/icons/credit-card-symbolic.svg"

                            anchors.centerIn: parent
                        }

                        Label {
                            anchors.centerIn: parent
                            text: qsTr("Donate")
                            wrapMode: Text.WordWrap
                            horizontalAlignment: Text.AlignHCenter
                        }

                    }

                    Item {
                        Image {
                            //   source: "qrc:/icons/flatpak-symbolic.svg"

                            anchors.centerIn: parent
                        }

                        Label {
                            anchors.centerIn: parent
                            text: qsTr("About Flatpak")
                            wrapMode: Text.WordWrap
                            horizontalAlignment: Text.AlignHCenter
                        }

                    }

                    Item {
                        Button {
                            onClicked: stack.push(configPage)
                            anchors.centerIn: parent
                            text: qsTr("Proceed to Configuration")
                        }

                    }

                }

                PageIndicator {
                    id: indicator

                    count: welcomeCarousel.count
                    currentIndex: welcomeCarousel.currentIndex
                    anchors.bottom: welcomeCarousel.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                header: ToolBar {
                    RowLayout {
                        anchors.fill: parent

                        Label {
                            text: qsTr("Welcome")
                            elide: Text.ElideRight
                            horizontalAlignment: Qt.AlignHCenter
                            verticalAlignment: Qt.AlignVCenter
                            Layout.fillWidth: true
                        }

                    }

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
