// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

// qtmcp declares its QObjects with the `signals`, `slots` and `emit`
// keywords, which Qt leaves undefined when QT_NO_KEYWORDS is set (as it is
// throughout this project). Restore the macros Qt itself would define so
// those headers compile; they expand to the spellings used everywhere else.
// Include this header only from .cpp files: the macros must not leak into
// the library's public headers.
#ifndef signals
#  define signals Q_SIGNALS
#endif
#ifndef slots
#  define slots Q_SLOTS
#endif
#ifndef emit
#  define emit
#endif

// qtmcp's qmcpserversession.h, qmcpserver.h and qmcpserverbackendinterface.h
// use QHash without including it.
// FIXME include <QHash> in those headers in qtmcp, then remove it here.
#include <QHash>
#include <QtMcpServer/QMcpServer>
#include <QtMcpServer/QMcpServerSession>
#include <QtMcpServer/qmcpserverstreamablehttp.h>
