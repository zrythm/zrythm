// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

RowLayout {
    id: root

    required property var transport
    required property var tempoTrack

    LinkedButtons {
        Button {
            icon.source: ResourceManager.getIconUrl("gnome-icon-library", "seek-backward-large-symbolic.svg")
            onClicked: transport.moveBackward()
        }

        Button {
            icon.source: ResourceManager.getIconUrl("gnome-icon-library", "seek-forward-large-symbolic.svg")
            onClicked: transport.moveForward()
        }

        Button {
            icon.source: ResourceManager.getIconUrl("gnome-icon-library", "skip-backward-large-symbolic.svg")
        }

        Button {
            icon.source: ResourceManager.getIconUrl("gnome-icon-library", (transport.playState == 1 ? "pause" : "play") + "-large-symbolic.svg")
            onClicked: {
                transport.isRolling() ? transport.requestPause(true) : transport.requestRoll(true);
            }
        }

        RecordButton {
            id: recordButton

            checked: transport.recordEnabled
            onCheckedChanged: {
                transport.recordEnabled = checked;
            }
        }

        Button {
            checkable: true
            checked: transport.loopEnabled
            onCheckedChanged: {
                transport.loopEnabled = checked;
            }
            icon.source: ResourceManager.getIconUrl("gnome-icon-library", "loop-arrow-symbolic.svg")
        }

    }

    RowLayout {
        spacing: 2

        EditableValueDisplay {
            value: tempoTrack.bpm
            label: "bpm"
        }

        EditableValueDisplay {
            // FrameAnimation {
            //     running: true
            //     onTriggered: {
            //         timeDisplay.value = transport.getPlayheadPositionString(tempoTrack);
            //     }
            // }

            id: timeDisplay

            value: {
                transport.playheadPosition.ticks; // Force property dependency
                return transport.playheadPosition.getStringDisplay(transport, tempoTrack);
            }
            label: "time"
            minValueWidth: timeTextMetrics.width
            minValueHeight: timeTextMetrics.height

            TextMetrics {
                id: timeTextMetrics

                text: "99.9.9.999"
                font: Style.semiBoldTextFont
            }

        }

        EditableValueDisplay {
            value: tempoTrack.getBeatsPerBar() + "/" + tempoTrack.getBeatUnit()
            label: "sig"
        }

    }

}
