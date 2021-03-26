/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

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
