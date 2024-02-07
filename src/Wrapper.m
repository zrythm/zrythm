// SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <Cocoa/Cocoa.h>
void show_on_top(void) {
    [NSApp activateIgnoringOtherApps:YES];
}
