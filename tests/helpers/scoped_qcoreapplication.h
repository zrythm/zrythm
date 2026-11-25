// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <chrono>
#include <memory>

#include <QCoreApplication>
#include <QString>
#include <QTest>

namespace zrythm::test_helpers
{
/**
 * @brief A base class that starts a QCoreApplication.
 *
 * This is needed in order to test signal functionality.
 *
 * Qt keeps the application name, the organization name and the
 * organization domain for the whole process. The values they had before
 * this object was constructed return when it is destroyed.
 *
 * @see https://forum.qt.io/post/804638
 */
class ScopedQCoreApplication
{
public:
  ScopedQCoreApplication ()
  {
    // Central knob for the QTest::qWaitFor()/QTRY_* timeout. Matches Qt's
    // own 5s default; stored explicitly so there is one place to tune it
    // (individual calls can still pass their own timeout).
    QTest::defaultTryTimeout.store (
      std::chrono::seconds (5), std::memory_order_relaxed);
    application_name_ = QCoreApplication::applicationName ();
    organization_name_ = QCoreApplication::organizationName ();
    organization_domain_ = QCoreApplication::organizationDomain ();
    int     argc = 0;
    char ** argv = nullptr;
    app_ = std::make_unique<QCoreApplication> (argc, argv);
  }

  ScopedQCoreApplication (const ScopedQCoreApplication &) = delete;
  ScopedQCoreApplication &operator= (const ScopedQCoreApplication &) = delete;
  ScopedQCoreApplication (ScopedQCoreApplication &&) = delete;
  ScopedQCoreApplication &operator= (ScopedQCoreApplication &&) = delete;

  ~ScopedQCoreApplication ()
  {
    app_.reset ();
    QCoreApplication::setApplicationName (application_name_);
    QCoreApplication::setOrganizationName (organization_name_);
    QCoreApplication::setOrganizationDomain (organization_domain_);
  }

private:
  QString                           application_name_;
  QString                           organization_name_;
  QString                           organization_domain_;
  std::unique_ptr<QCoreApplication> app_;
};
}
