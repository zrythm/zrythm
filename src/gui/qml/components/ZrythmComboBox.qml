// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import QtQuick.Layouts

ComboBox {
    id: control

    Layout.fillWidth: true
    model: ['日本語', 'bbbb long text', 'short']
    implicitHeight: 24
    implicitWidth: 120
    implicitContentWidthPolicy: ComboBox.ContentItemImplicitWidth

    delegate: ItemDelegate {
        id: delegate

        required property var model
        required property int index

        width: control.popup.availableWidth
        height: control.implicitHeight
        hoverEnabled: true
        focus: true
        highlighted: control.highlightedIndex === index

        contentItem: Text {
            opacity: 0.9
            text: delegate.model[control.textRole]
            color: delegate.hovered ? palette.highlightedText : palette.text
            font: delegate.font
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            anchors.fill: delegate
            rightPadding: delegate.spacing
        }

        background: Rectangle {
            anchors.fill: delegate
            radius: 6
            color: delegate.hovered ? palette.highlight : "transparent"
        }

        Component.onCompleted: {
            console.log("onCompleted", delegate.text, delegate.model, delegate.model[control.textRole])
        }

    }

    contentItem: Text {
        anchors.leftMargin: 10
        anchors.fill: control
        rightPadding: control.indicator.width + control.spacing
        opacity: 0.9
        text: control.displayText
        font: control.font
        color: palette.buttonText
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        implicitWidth: control.implicitWidth
        implicitHeight: control.implicitHeight
        radius: 8
        color: palette.button
        border.width: 2
        border.color: control.activeFocus ? palette.accent : palette.base
    }

    popup: Popup {
        width: control.width

        contentItem: ListView {
            implicitHeight: contentHeight
            keyNavigationEnabled: true
            model: control.popup.visible ? control.delegateModel : null
            clip: true
            focus: true
            currentIndex: control.highlightedIndex
        }

        background: Rectangle {
            anchors.fill: parent
            color: palette.window
            border.color: palette.midlight
            border.width: 2
            radius: 8
            clip: true
        }

        enter: Transition {
            NumberAnimation {
                property: "opacity"
                from: 0
                to: 1
                duration: 100
            }

        }

        exit: Transition {
            NumberAnimation {
                property: "opacity"
                from: 1
                to: 0
                duration: 100
            }

        }

    }

    indicator: Item {
        width: Math.min(control.height * 0.6, 20) // Max size of 20px
        height: width
        opacity: 0.6

        anchors {
            right: parent.right
            verticalCenter: parent.verticalCenter
            rightMargin: 8
        }

        Image {
            id: indicatorImage

            source: Qt.resolvedUrl("icons/lucide/chevrons-up-down.svg")
            sourceSize.width: indicator.width
            sourceSize.height: indicator.height
        }

        // Apply a color to the SVG
        MultiEffect {
            source: indicatorImage
            anchors.fill: indicatorImage
            colorization: 1
            colorizationColor: palette.buttonText
        }

    }

}
