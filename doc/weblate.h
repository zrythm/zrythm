/**
 * \page weblate Weblate
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
 * From https://docs.weblate.org/en/latest/faq.html
 * ``add weblate https://hosted.weblate.org/git/project/component/``
 * Then ``git fetch weblate``, followed by
 * git rebase local translate branch with weblate/translate.
 *
 * Then merge that in master, fix conflicts, rebase master
 * back to local translate to get new strings and
 * push.
 *
 */

