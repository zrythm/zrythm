// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma Singleton
import QtQuick

QtObject {
    function getObjectBounds(obj) {
        if (obj.bounds) {
            return obj.bounds;
        } else if (obj.regionMixin?.bounds) {
            return obj.regionMixin.bounds;
        } else {
          return null;
        }
    }

    function getObjectName(obj) {
        if (obj.name) {
            return obj.name;
        } else if (obj.regionMixin?.name) {
            return obj.regionMixin.name;
        } else {
          return null;
        }
    }
}
