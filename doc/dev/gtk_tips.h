/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
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
 * \page gtk_tips GTK Tips
 *
 * \section dispose_vs_finalize_vs_destroy dispose() vs finalize() vs dispose()
 *
 * According to ebassi from GTK:
 * Rule of thumb: use dispose()
 * for releasing references to objects you acquired
 * from the outside, and finalize() to release
 * memory/references you allocated internally.
 *
 * There's basically no reason to
 * override destroy(); always use dispose/finalize.
 * The "destroy" signal is for
 * other code, using your widget, to release
 * references they might have.
 */
