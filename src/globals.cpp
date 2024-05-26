// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <memory>

#include "zrythm.h"
#include "zrythm_app.h"

/** This is declared extern in zrythm.h. */
std::unique_ptr<Zrythm> gZrythm = nullptr;

/** This is declared extern in zrythm_app.h. */
ZrythmApp * zrythm_app = NULL;
