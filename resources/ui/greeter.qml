import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import "config.js" as Config

ApplicationWindow {
    id: greeter

    title: "Zrythm"
    modality: Qt.ApplicationModal
    minimumWidth: 256
    width: 640
    height: 420
    visible: true
    font.family: interFont.name
    font.pointSize: 10

    palette {
        accent: themeManager.accent
        base: themeManager.base
        brightText: themeManager.base
        button: themeManager.base.lighter().lighter(1.2)
        buttonText: "white"
        dark: "white" // used by paginator
        light: "#999999" // combo box hover background
        highlight: themeManager.accent
        link: themeManager.accent
        text: "white"
        window: themeManager.base
        windowText: "white"
    }

    FontLoader {
        id: dsegRegular

        source: Qt.resolvedUrl("fonts/DSEG14ClassicMini-Regular.ttf")
    }

    FontLoader {
        id: dsegBold

        source: Qt.resolvedUrl("fonts/DSEG14ClassicMini-Bold.ttf")
    }

    FontLoader {
        id: interFont

        source: Qt.resolvedUrl("fonts/InterVariable.ttf")
    }

    FontLoader {
        id: interFontItalic

        source: Qt.resolvedUrl("fonts/InterVariable-Italic.ttf")
    }

    Item {
        id: flatpakPage

        PlaceholderPage {
            icon.source: Qt.resolvedUrl("icons/gnome-icon-library/flatpak-symbolic.svg")
            title: qsTr("About Flatpak")
            description: qsTr("Only audio plugins installed via Flatpak are supported.")
        }

    }

    Item {
        id: donationPage

        PlaceholderPage {
            icon.source: Qt.resolvedUrl("icons/gnome-icon-library/credit-card-symbolic.svg")
            title: qsTr("Donate")
            description: qsTr("Zrythm relies on donations and purchases to sustain development. If you enjoy the software, please consider %1donating%2 or %3buying an installer%2.").arg("<a href=\"" + Config.DONATION_URL + "\">").arg("</a>").arg("<a href=\"" + Config.PURCHASE_URL + "\">")
        }

    }

    Item {
        id: proceedToConfigPage

        PlaceholderPage {
            title: qsTr("All Ready!")

            action: Action {
                text: qsTr("Proceed to Configuration")
                onTriggered: stack.push(configPage)
            }

        }

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
                    Component.onCompleted: {
                        if (!Config.IS_INSTALLER_VER || Config.IS_TRIAL_VER)
                            addItem(donationPage);

                        if (Config.FLATPAK_BUILD)
                            addItem(flatpakPage);

                        addItem(proceedToConfigPage);
                    }

                    Item {
                        PlaceholderPage {
                            icon.source: Qt.resolvedUrl("icons/zrythm.svg")
                            title: qsTr("Welcome")
                            description: qsTr("Welcome to the Zrythm digital audio workstation. Move to the next page to get started.")
                        }

                    }

                    Item {
                        PlaceholderPage {
                            icon.source: Qt.resolvedUrl("icons/gnome-icon-library/open-book-symbolic.svg")
                            title: qsTr("Read the Manual")
                            description: qsTr("If this is your first time using Zrythm, we suggest going through the 'Getting Started' section in the %1user manual%2.").arg("<a href=\"" + Config.USER_MANUAL_URL + "\">").arg("</a>")
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
                    font.bold: true
                }

                // Add right navigation button
                RoundButton {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: ">"
                    onClicked: welcomeCarousel.incrementCurrentIndex()
                    visible: welcomeCarousel.currentIndex < welcomeCarousel.count - 1
                    font.pixelSize: 18
                    font.bold: true
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

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 10

                    ActionRow {
                        title: "Language"
                        subtitle: "Preferred language"

                        ComboBoxText {
                            model: ["English", "Spanish", "French"]
                        }

                    }

                    ActionRow {
                        title: "User Path"
                        subtitle: "Location to save user files"

                        TextField {
                            Layout.fillWidth: true
                        }

                        Button {
                            text: "Browse"
                        }

                    }

                }

                header: ToolBar {
                    RowLayout {
                        anchors.fill: parent

                        ToolButton {
                            text: qsTr("‹")
                            onClicked: stack.pop()
                        }

                        Label {
                            text: qsTr("Configuration")
                            font.bold: true
                            elide: Text.ElideRight
                            horizontalAlignment: Qt.AlignHCenter
                            verticalAlignment: Qt.AlignVCenter
                            Layout.fillWidth: true
                        }

                    }

                }

                footer: DialogButtonBox {
                    standardButtons: DialogButtonBox.Reset
                    onReset: {
                        console.log("Resetting settings...");
                    }
                    horizontalPadding: 10

                    PlainButton {
                        text: qsTr("Continue")
                        highlighted: true
                        onClicked: {
                            console.log("Proceeding to next page...");
                        }
                        DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                    }

                    delegate: PlainButton {
                        id: control
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
                    font.pointSize: 24
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
