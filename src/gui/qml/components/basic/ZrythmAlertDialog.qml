// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls.Basic

Dialog {
  id: alertDialog

  property alias alertMessage: messageText.text
  property alias alertTitle: titleText.text

  modal: true
  standardButtons: Dialog.Ok
  title: "Alert"

  Column {
    spacing: 10

    Text {
      id: titleText

      font.bold: true
      font.pixelSize: 16
    }

    Text {
      id: messageText

      width: parent.width
      wrapMode: Text.Wrap
    }
  }
}
