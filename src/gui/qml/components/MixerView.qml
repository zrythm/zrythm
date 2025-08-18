// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick.Layouts
import QtQuick
import Zrythm

RowLayout {
  id: root

  required property Tracklist tracklist

  Repeater {
    id: allChannels

    Layout.fillHeight: true

    delegate: ChannelView {
      channel: track.channel
      tracklist: root.tracklist
    }
    model: TrackFilterProxyModel {
      sourceModel: root.tracklist

      Component.onCompleted: {
        addVisibilityFilter(true);
        addChannelFilter(true);
      }
    }
  }
}
