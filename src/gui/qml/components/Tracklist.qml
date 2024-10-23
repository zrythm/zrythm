// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

// pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

ListView {
    id: root

    required property var tracklist
    required property bool pinned

    implicitWidth: 200
    implicitHeight: 200

    model: TrackFilterProxyModel {
        sourceModel: tracklist
        Component.onCompleted: {
            addVisibilityFilter(true);
            addPinnedFilter(root.pinned);
        }
    }

    delegate: TrackView {
    }

}
