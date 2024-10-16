import QtQuick
import QtQuick.Controls.Basic

MenuBar {
    /*
    ZrythmMenu {
        title: qsTr("&File")
    }

    ZrythmMenu {
        title: qsTr("&Edit")
    }

    ZrythmMenu {
        title: qsTr("Edit &Something Long")
    }
    */

    id: menuBar

    delegate: MenuBarItem {
        id: menuBarItem

        // hack because customized menubars don't automatically support mnemonic labels (yet?)
        function convertToMnemonicText(text) {
            const cleanText = text.replace("&", "");
            const underlineIndex = text.indexOf("&");
            if (underlineIndex >= 0)
                return cleanText.substring(0, underlineIndex) + "<u>" + cleanText.charAt(underlineIndex) + "</u>" + cleanText.substring(underlineIndex + 1);

            return cleanText;
        }

        implicitHeight: 24

        contentItem: Text {
            text: menuBarItem.convertToMnemonicText(menuBarItem.text)
            opacity: enabled ? 1 : 0.3
            color: menuBarItem.highlighted ? palette.highlightedText : palette.buttonText
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight

            font {
                family: menuBarItem.font.family
                pointSize: 9
            }

        }

        background: Rectangle {
            opacity: enabled ? 1 : 0.3
            color: menuBarItem.highlighted ? palette.highlight : "transparent"
        }

    }

    background: Rectangle {
        color: palette.window
    }

}
