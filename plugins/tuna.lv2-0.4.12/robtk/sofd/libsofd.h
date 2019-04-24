/* libSOFD - Simple Open File Dialog [for X11 without toolkit]
 *
 * Copyright (C) 2014 Robin Gareus <robin@gareus.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifdef HAVE_X11
#include <X11/Xlib.h>

///////////////////////////////////////////////////////////////////////////////
/* public API */

/** open a file select dialog
 * @param dpy X Display connection
 * @param parent (optional) if not NULL, become transient for given window
 * @param x if >0 set explict initial width of the window
 * @param y if >0 set explict initial height of the window
 * @return 0 on success
 */
int x_fib_show (Display *dpy, Window parent, int x, int y);

/** force close the dialog.
 * This is normally not needed, the dialog closes itself
 * when a file is selected or the user cancels selection.
 * @param dpy X Display connection
 */
void x_fib_close (Display *dpy);

/** non-blocking X11 event handler.
 * It is safe to run this function even if the dialog is
 * closed or was not initialized.
 *
 * @param dpy X Display connection
 * @param event the XEvent to process
 * @return status
 *   0:  the event was not for this window, or file-dialog still
 *       active, or the dialog window is not displayed.
 *   >0: file was selected, dialog closed
 *   <0: file selection was cancelled.
 */
int x_fib_handle_events (Display *dpy, XEvent *event);

/** last status of the dialog
 * @return >0: file was selected, <0: canceled or inactive. 0: active
 */
int x_fib_status ();

/** query the selected filename
 * @return NULL if none set, or allocated string to be free()ed by the called
 */
char *x_fib_filename ();

/** customize/configure the dialog before calling \ref x_fib_show
 * changes only have any effect if the dialog is not visible.
 * @param k key to change
 *  0: set current dir to display (must end with slash)
 *  1: set title of dialog window
 *  2: specify a custom X11 font to use
 *  3: specify a custom 'places' file to include
 *     (following gtk-bookmark convention)
 * @param v value
 * @return 0 on success.
 */
int x_fib_configure (int k, const char *v);

/** customize/configure the dialog before calling \ref x_fib_show
 * changes only have any effect if the dialog is not visible.
 *
 * @param k button to change:
 *  1: show hidden files
 *  2: show places
 *  3: show filter/list all (automatically hidden if there is no
 *     filter function)
 * @param v <0 to hide the button >=0 show button,
 *  0: set button-state to not-checked
 *  1: set button-state to checked
 *  >1: retain current state
 * @return 0 on success.
 */
int x_fib_cfg_buttons (int k, int v);

/** set custom callback to filter file-names.
 * NULL will disable the filter and hide the 'show all' button.
 * changes only have any effect if the dialog is not visible.
 *
 * @param cb callback function to check file
 *  the callback function is called with the file name (basename only)
 *  and is expected to return 1 if the file passes the filter
 *  and 0 if the file should not be listed by default.
 * @return 0 on success.
 */
int x_fib_cfg_filter_callback (int (*cb)(const char*));

#endif /* END X11 specific functions */

/* 'recently used' API. x-platform
 * NOTE: all functions use a static cache and are not reentrant.
 * It is expected that none of these functions are called in
 * parallel from different threads.
 */

/** release static resources of 'recently used files'
 */
void x_fib_free_recent ();

/** add an entry to the recently used list
 *
 * The dialog does not add files automatically on open,
 * if the application succeeds to open a selected file,
 * this function should be called.
 *
 * @param path complete path to file
 * @param atime time of last use, 0: NOW
 * @return -1 on error, number of current entries otherwise
 */
int x_fib_add_recent (const char *path, time_t atime);

/** get a platform specific path to a good location for
 * saving the recently used file list.
 * (follows XDG_DATA_HOME on Unix, and CSIDL_LOCAL_APPDATA spec)
 *
 * @param application-name to use to include in file
 * @return pointer to static path or NULL
 */
const char *x_fib_recent_file(const char *appname);

/** save the current list of recently used files to the given filename
 * (the format is one file per line, filename URL encoded and space separated
 * with last-used timestamp)
 *
 * This function tries to creates the containing directory if it does
 * not exist.
 *
 * @param fn file to save the list to
 * @return 0: on success
 */
int x_fib_save_recent (const char *fn);

/** load a recently used file list.
 *
 * @param fn file to load the list from
 * @return 0: on success
 */
int x_fib_load_recent (const char *fn);

/** get number of entries in the current list
 * @return number of entries in the recently used list
 */
unsigned int x_fib_recent_count ();

/** get recently used entry at given position
 *
 * @param i entry to query
 * @return pointer to static string
 */
const char *x_fib_recent_at (unsigned int i);
