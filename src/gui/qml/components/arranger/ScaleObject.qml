// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Layouts
import Zrythm 1.0
import ZrythmStyle 1.0

ArrangerObjectBase {
    id: root

    height: textMetrics.height + 2 * Style.buttonPadding
    width: textMetrics.width + 2 * Style.buttonPadding

    Rectangle {
        color: root.objectColor
        anchors.fill: parent
        radius: Style.toolButtonRadius
    }

    Text {
        id: nameText

        text: {
          // bindings
          arrangerObject.scale.scaleType;
          arrangerObject.scale.rootKey;
          return arrangerObject.scale.toString();
        }
        font: root.font
        color: root.palette.text
        padding: Style.buttonPadding
    }

    TextMetrics {
        id: textMetrics

        text: nameText.text
        font: nameText.font
    }

}
