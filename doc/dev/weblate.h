// SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \page weblate Weblate Translations
 *
 * \section introduction_weblate Introduction
 *
 * Zrythm uses Weblate, a web tool where translators
 * can easily edit the translations (PO files).
 * This page explains how to handle those
 * translations.
 *
 * \section weblate_merging Merging the Translations
 *
 * From https://docs.weblate.org/en/latest/faq.html:
 * - ``git remote add weblate https://hosted.weblate.org/git/project/component/``
 * - ``git fetch weblate``
 * - ``git checkout translate && git rebase weblate/translate`` to rebase the remote branch to the local one
 *
 * Then merge that in master, fix conflicts, rebase master
 * back to local translate to get new strings and
 * push.
 *
 */
