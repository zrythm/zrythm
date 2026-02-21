// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import Zrythm

Rectangle {
  id: root

  required property ArrangerObject arrangerObject
  required property point coordinatesOnConstruction

  color: palette.accent
  height: 16
  radius: 4
  width: 100

  Text {
    text: {
      const nameObj = ArrangerObjectHelpers.getObjectName(root.arrangerObject);
      return nameObj ? nameObj.name : "";
    }
  }
}
