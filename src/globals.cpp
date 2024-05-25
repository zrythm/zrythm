// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <memory>

#include "zrythm.h"

/** This is declared extern in zrythm.h. */
std::unique_ptr<Zrythm>                 gZrythm = nullptr;
std::unique_ptr<ZrythmDirectoryManager> gZrythmDirMgr =
  std::make_unique<ZrythmDirectoryManager> ();
