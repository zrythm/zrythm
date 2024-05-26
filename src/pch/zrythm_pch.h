// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __PCH_ZRYTHM_PCH_H__
#define __PCH_ZRYTHM_PCH_H__

#ifdef _WIN32
#include <windows.h>
#endif

#include <memory>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#include <magic_enum_all.hpp>
#pragma GCC diagnostic pop

/* This also includes all native platform headers. */
/* FIXME compilation fails with internal compiler errors if this is included */
// #include "ext/juce/juce.h"

#endif /* __PCH_ZRYTHM_PCH_H__ */
