/*
  Copyright 2012-2019 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/**
   @file pugl.h Public C API.
*/

#ifndef PUGL_H_INCLUDED
#define PUGL_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef PUGL_SHARED
#    ifdef _WIN32
#        define PUGL_LIB_IMPORT __declspec(dllimport)
#        define PUGL_LIB_EXPORT __declspec(dllexport)
#    else
#        define PUGL_LIB_IMPORT __attribute__((visibility("default")))
#        define PUGL_LIB_EXPORT __attribute__((visibility("default")))
#    endif
#    ifdef PUGL_INTERNAL
#        define PUGL_API PUGL_LIB_EXPORT
#    else
#        define PUGL_API PUGL_LIB_IMPORT
#    endif
#else
#    define PUGL_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
   @defgroup pugl Pugl
   Pugl C API.
   @{
*/

/**
   Handle for opaque user data.
*/
typedef void* PuglHandle;

/**
   Return status code.
*/
typedef enum {
	PUGL_SUCCESS,               /**< Success */
	PUGL_FAILURE,               /**< Non-fatal failure */
	PUGL_UNKNOWN_ERROR,         /**< Unknown system error */
	PUGL_BAD_BACKEND,           /**< Invalid or missing backend */
	PUGL_BACKEND_FAILED,        /**< Backend initialisation failed */
	PUGL_REGISTRATION_FAILED,   /**< Window class registration failed */
	PUGL_CREATE_WINDOW_FAILED,  /**< Window creation failed */
	PUGL_SET_FORMAT_FAILED,     /**< Failed to set pixel format */
	PUGL_CREATE_CONTEXT_FAILED, /**< Failed to create drawing context */
	PUGL_UNSUPPORTED_TYPE,      /**< Unsupported data type */
} PuglStatus;

/**
   Window hint.
*/
typedef enum {
	PUGL_USE_COMPAT_PROFILE,    /**< Use compatible (not core) OpenGL profile */
	PUGL_USE_DEBUG_CONTEXT,     /**< True to use a debug OpenGL context */
	PUGL_CONTEXT_VERSION_MAJOR, /**< OpenGL context major version */
	PUGL_CONTEXT_VERSION_MINOR, /**< OpenGL context minor version */
	PUGL_RED_BITS,              /**< Number of bits for red channel */
	PUGL_GREEN_BITS,            /**< Number of bits for green channel */
	PUGL_BLUE_BITS,             /**< Number of bits for blue channel */
	PUGL_ALPHA_BITS,            /**< Number of bits for alpha channel */
	PUGL_DEPTH_BITS,            /**< Number of bits for depth buffer */
	PUGL_STENCIL_BITS,          /**< Number of bits for stencil buffer */
	PUGL_SAMPLES,               /**< Number of samples per pixel (AA) */
	PUGL_DOUBLE_BUFFER,         /**< True if double buffering should be used */
	PUGL_SWAP_INTERVAL,         /**< Number of frames between buffer swaps */
	PUGL_RESIZABLE,             /**< True if window should be resizable */
	PUGL_IGNORE_KEY_REPEAT,     /**< True if key repeat events are ignored */

	PUGL_NUM_WINDOW_HINTS
} PuglViewHint;

/**
   Special window hint value.
*/
typedef enum {
	PUGL_DONT_CARE = -1,  /**< Use best available value */
	PUGL_FALSE     = 0,   /**< Explicitly false */
	PUGL_TRUE      = 1    /**< Explicitly true */
} PuglViewHintValue;

/**
   A rectangle.

   This is used to describe things like view position and size.  Pugl generally
   uses coordinates where the top left corner is 0,0.
*/
typedef struct {
	double x;
	double y;
	double width;
	double height;
} PuglRect;

/**
   Keyboard modifier flags.
*/
typedef enum {
	PUGL_MOD_SHIFT = 1,       /**< Shift key */
	PUGL_MOD_CTRL  = 1 << 1,  /**< Control key */
	PUGL_MOD_ALT   = 1 << 2,  /**< Alt/Option key */
	PUGL_MOD_SUPER = 1 << 3   /**< Mod4/Command/Windows key */
} PuglMod;

/**
   Special keyboard keys.

   All keys, special or not, are expressed as a Unicode code point.  This
   enumeration defines constants for special keys that do not have a standard
   code point, and some convenience constants for control characters.

   Keys that do not have a standard code point use values in the Private Use
   Area in the Basic Multilingual Plane (U+E000 to U+F8FF).  Applications must
   take care to not interpret these values beyond key detection, the mapping
   used here is arbitrary and specific to Pugl.
*/
typedef enum {
	// ASCII control codes
	PUGL_KEY_BACKSPACE = 0x08,
	PUGL_KEY_ESCAPE    = 0x1B,
	PUGL_KEY_DELETE    = 0x7F,

	// Unicode Private Use Area
	PUGL_KEY_F1 = 0xE000,
	PUGL_KEY_F2,
	PUGL_KEY_F3,
	PUGL_KEY_F4,
	PUGL_KEY_F5,
	PUGL_KEY_F6,
	PUGL_KEY_F7,
	PUGL_KEY_F8,
	PUGL_KEY_F9,
	PUGL_KEY_F10,
	PUGL_KEY_F11,
	PUGL_KEY_F12,
	PUGL_KEY_LEFT,
	PUGL_KEY_UP,
	PUGL_KEY_RIGHT,
	PUGL_KEY_DOWN,
	PUGL_KEY_PAGE_UP,
	PUGL_KEY_PAGE_DOWN,
	PUGL_KEY_HOME,
	PUGL_KEY_END,
	PUGL_KEY_INSERT,
	PUGL_KEY_SHIFT,
	PUGL_KEY_SHIFT_L = PUGL_KEY_SHIFT,
	PUGL_KEY_SHIFT_R,
	PUGL_KEY_CTRL,
	PUGL_KEY_CTRL_L = PUGL_KEY_CTRL,
	PUGL_KEY_CTRL_R,
	PUGL_KEY_ALT,
	PUGL_KEY_ALT_L = PUGL_KEY_ALT,
	PUGL_KEY_ALT_R,
	PUGL_KEY_SUPER,
	PUGL_KEY_SUPER_L = PUGL_KEY_SUPER,
	PUGL_KEY_SUPER_R,
	PUGL_KEY_MENU,
	PUGL_KEY_CAPS_LOCK,
	PUGL_KEY_SCROLL_LOCK,
	PUGL_KEY_NUM_LOCK,
	PUGL_KEY_PRINT_SCREEN,
	PUGL_KEY_PAUSE
} PuglKey;

/**
   The type of a PuglEvent.
*/
typedef enum {
	PUGL_NOTHING,              /**< No event */
	PUGL_BUTTON_PRESS,         /**< Mouse button press */
	PUGL_BUTTON_RELEASE,       /**< Mouse button release */
	PUGL_CONFIGURE,            /**< View moved and/or resized */
	PUGL_EXPOSE,               /**< View exposed, redraw required */
	PUGL_CLOSE,                /**< Close view */
	PUGL_KEY_PRESS,            /**< Key press */
	PUGL_KEY_RELEASE,          /**< Key release */
	PUGL_TEXT,                 /**< Character entry */
	PUGL_ENTER_NOTIFY,         /**< Pointer entered view */
	PUGL_LEAVE_NOTIFY,         /**< Pointer left view */
	PUGL_MOTION_NOTIFY,        /**< Pointer motion */
	PUGL_SCROLL,               /**< Scroll */
	PUGL_FOCUS_IN,             /**< Keyboard focus entered view */
	PUGL_FOCUS_OUT             /**< Keyboard focus left view */
} PuglEventType;

typedef enum {
	PUGL_IS_SEND_EVENT = 1
} PuglEventFlag;

/**
   Reason for a PuglEventCrossing.
*/
typedef enum {
	PUGL_CROSSING_NORMAL,      /**< Crossing due to pointer motion. */
	PUGL_CROSSING_GRAB,        /**< Crossing due to a grab. */
	PUGL_CROSSING_UNGRAB       /**< Crossing due to a grab release. */
} PuglCrossingMode;

/**
   Common header for all event structs.
*/
typedef struct {
	PuglEventType type;        /**< Event type. */
	uint32_t      flags;       /**< Bitwise OR of PuglEventFlag values. */
} PuglEventAny;

/**
   Button press or release event.

   For event types PUGL_BUTTON_PRESS and PUGL_BUTTON_RELEASE.
*/
typedef struct {
	PuglEventType type;        /**< PUGL_BUTTON_PRESS or PUGL_BUTTON_RELEASE. */
	uint32_t      flags;       /**< Bitwise OR of PuglEventFlag values. */
	double        time;        /**< Time in seconds. */
	double        x;           /**< View-relative X coordinate. */
	double        y;           /**< View-relative Y coordinate. */
	double        xRoot;       /**< Root-relative X coordinate. */
	double        yRoot;       /**< Root-relative Y coordinate. */
	uint32_t      state;       /**< Bitwise OR of PuglMod flags. */
	uint32_t      button;      /**< 1-relative button number. */
} PuglEventButton;

/**
   Configure event for when window size or position has changed.
*/
typedef struct {
	PuglEventType type;        /**< PUGL_CONFIGURE. */
	uint32_t      flags;       /**< Bitwise OR of PuglEventFlag values. */
	double        x;           /**< New parent-relative X coordinate. */
	double        y;           /**< New parent-relative Y coordinate. */
	double        width;       /**< New width. */
	double        height;      /**< New height. */
} PuglEventConfigure;

/**
   Expose event for when a region must be redrawn.
*/
typedef struct {
	PuglEventType type;        /**< PUGL_EXPOSE. */
	uint32_t      flags;       /**< Bitwise OR of PuglEventFlag values. */
	double        x;           /**< View-relative X coordinate. */
	double        y;           /**< View-relative Y coordinate. */
	double        width;       /**< Width of exposed region. */
	double        height;      /**< Height of exposed region. */
	int           count;       /**< Number of expose events to follow. */
} PuglEventExpose;

/**
   Window close event.
*/
typedef struct {
	PuglEventType type;        /**< PUGL_CLOSE. */
	uint32_t      flags;       /**< Bitwise OR of PuglEventFlag values. */
} PuglEventClose;

/**
   Key press/release event.

   This represents low-level key press and release events.  This event type
   should be used for "raw" keyboard handing (key bindings, for example), but
   must not be interpreted as text input.

   Keys are represented as Unicode code points, using the "natural" code point
   for the key wherever possible (see @ref PuglKey for details).  The `key`
   field will be set to the code for the pressed key, without any modifiers
   applied (by the shift or control keys).
*/
typedef struct {
	PuglEventType type;        /**< PUGL_KEY_PRESS or PUGL_KEY_RELEASE. */
	uint32_t      flags;       /**< Bitwise OR of PuglEventFlag values. */
	double        time;        /**< Time in seconds. */
	double        x;           /**< View-relative X coordinate. */
	double        y;           /**< View-relative Y coordinate. */
	double        xRoot;       /**< Root-relative X coordinate. */
	double        yRoot;       /**< Root-relative Y coordinate. */
	uint32_t      state;       /**< Bitwise OR of PuglMod flags. */
	uint32_t      keycode;     /**< Raw key code. */
	uint32_t      key;         /**< Unshifted Unicode character code, or 0. */
} PuglEventKey;

/**
   Character input event.

   This represents text input, usually as the result of a key press.  The text
   is given both as a Unicode character code and a UTF-8 string.
*/
typedef struct {
	PuglEventType type;        /**< PUGL_CHAR. */
	uint32_t      flags;       /**< Bitwise OR of PuglEventFlag values. */
	double        time;        /**< Time in seconds. */
	double        x;           /**< View-relative X coordinate. */
	double        y;           /**< View-relative Y coordinate. */
	double        xRoot;       /**< Root-relative X coordinate. */
	double        yRoot;       /**< Root-relative Y coordinate. */
	uint32_t      state;       /**< Bitwise OR of PuglMod flags. */
	uint32_t      keycode;     /**< Raw key code. */
	uint32_t      character;   /**< Unicode character code */
	char          string[8];   /**< UTF-8 string. */
} PuglEventText;

/**
   Pointer crossing event (enter and leave).
*/
typedef struct {
	PuglEventType    type;     /**< PUGL_ENTER_NOTIFY or PUGL_LEAVE_NOTIFY. */
	uint32_t         flags;    /**< Bitwise OR of PuglEventFlag values. */
	double           time;     /**< Time in seconds. */
	double           x;        /**< View-relative X coordinate. */
	double           y;        /**< View-relative Y coordinate. */
	double           xRoot;    /**< Root-relative X coordinate. */
	double           yRoot;    /**< Root-relative Y coordinate. */
	uint32_t         state;    /**< Bitwise OR of PuglMod flags. */
	PuglCrossingMode mode;     /**< Reason for crossing. */
} PuglEventCrossing;

/**
   Pointer motion event.
*/
typedef struct {
	PuglEventType type;        /**< PUGL_MOTION_NOTIFY. */
	uint32_t      flags;       /**< Bitwise OR of PuglEventFlag values. */
	double        time;        /**< Time in seconds. */
	double        x;           /**< View-relative X coordinate. */
	double        y;           /**< View-relative Y coordinate. */
	double        xRoot;       /**< Root-relative X coordinate. */
	double        yRoot;       /**< Root-relative Y coordinate. */
	uint32_t      state;       /**< Bitwise OR of PuglMod flags. */
	bool          isHint;      /**< True iff this event is a motion hint. */
	bool          focus;       /**< True iff this is the focused window. */
} PuglEventMotion;

/**
   Scroll event.

   The scroll distance is expressed in "lines", an arbitrary unit that
   corresponds to a single tick of a detented mouse wheel.  For example, `dy` =
   1.0 scrolls 1 line up.  Some systems and devices support finer resolution
   and/or higher values for fast scrolls, so programs should handle any value
   gracefully.
 */
typedef struct {
	PuglEventType type;        /**< PUGL_SCROLL. */
	uint32_t      flags;       /**< Bitwise OR of PuglEventFlag values. */
	double        time;        /**< Time in seconds. */
	double        x;           /**< View-relative X coordinate. */
	double        y;           /**< View-relative Y coordinate. */
	double        xRoot;       /**< Root-relative X coordinate. */
	double        yRoot;       /**< Root-relative Y coordinate. */
	uint32_t      state;       /**< Bitwise OR of PuglMod flags. */
	double        dx;          /**< Scroll X distance in lines. */
	double        dy;          /**< Scroll Y distance in lines. */
} PuglEventScroll;

/**
   Keyboard focus event.
*/
typedef struct {
	PuglEventType type;        /**< PUGL_FOCUS_IN or PUGL_FOCUS_OUT. */
	uint32_t      flags;       /**< Bitwise OR of PuglEventFlag values. */
	bool          grab;        /**< True iff this is a grab/ungrab event. */
} PuglEventFocus;

/**
   Interface event.

   This is a union of all event structs.  The `type` must be checked to
   determine which fields are safe to access.  A pointer to PuglEvent can
   either be cast to the appropriate type, or the union members used.
*/
typedef union {
	PuglEventType      type;       /**< Event type. */
	PuglEventAny       any;        /**< Valid for all event types. */
	PuglEventButton    button;     /**< PUGL_BUTTON_PRESS, PUGL_BUTTON_RELEASE. */
	PuglEventConfigure configure;  /**< PUGL_CONFIGURE. */
	PuglEventExpose    expose;     /**< PUGL_EXPOSE. */
	PuglEventClose     close;      /**< PUGL_CLOSE. */
	PuglEventKey       key;        /**< PUGL_KEY_PRESS, PUGL_KEY_RELEASE. */
	PuglEventText      text;       /**< PUGL_TEXT. */
	PuglEventCrossing  crossing;   /**< PUGL_ENTER_NOTIFY, PUGL_LEAVE_NOTIFY. */
	PuglEventMotion    motion;     /**< PUGL_MOTION_NOTIFY. */
	PuglEventScroll    scroll;     /**< PUGL_SCROLL. */
	PuglEventFocus     focus;      /**< PUGL_FOCUS_IN, PUGL_FOCUS_OUT. */
} PuglEvent;

/**
   @anchor world
   @name World
   The top level context of a Pugl application.
   @{
*/

/**
   The "world" of application state.

   The world represents things that are not associated with a particular view.
   Several worlds can be created in a process (which is the case when many
   plugins use Pugl, for example), but code using different worlds must be
   isolated so they are never mixed.  Views are strongly associated with the
   world they were created for.
*/
typedef struct PuglWorldImpl PuglWorld;

/**
   Create a new world.

   @return A newly created world.
*/
PUGL_API PuglWorld*
puglNewWorld(void);

/**
   Free a world allocated with puglNewWorld().
*/
PUGL_API void
puglFreeWorld(PuglWorld* world);

/**
   Set the class name of the application.

   This is a stable identifier for the application, used as the window
   class/instance name on X11 and Windows.  It is not displayed to the user,
   but can be used in scripts and by window managers, so it should be the same
   for every instance of the application, but different from other
   applications.
*/
PUGL_API PuglStatus
puglSetClassName(PuglWorld* world, const char* name);

/**
   Return the time in seconds.

   This is a monotonically increasing clock with high resolution.  The returned
   time is only useful to compare against other times returned by this
   function, its absolute value has no meaning.
*/
PUGL_API double
puglGetTime(const PuglWorld* world);

/**
   Poll for events that are ready to be processed.

   This polls for events that are ready for any view in the application,
   potentially blocking depending on `timeout`.

   @param world The world for all the views to poll.
   @param timeout Maximum time to wait, in seconds.  If zero, the call returns
   immediately, if negative, the call blocks indefinitely.
   @return PUGL_SUCCESS if events are read, PUGL_FAILURE if not, or an error.
*/
PUGL_API PuglStatus
puglPollEvents(PuglWorld* world, double timeout);

/**
   Dispatch any pending events to views.

   This processes all pending events, dispatching them to the appropriate
   views.  View event handlers will be called in the scope of this call.  This
   function does not block, if no events are pending it will return
   immediately.
*/
PUGL_API PuglStatus
puglDispatchEvents(PuglWorld* world);

/**
   @}
   @anchor view
   @name View
   A view is a drawing region that receives events, which may correspond to a
   top-level window or be embedded in some other window.
   @{
*/

/**
   A Pugl view.
*/
typedef struct PuglViewImpl PuglView;

/**
   Create a new view.

   A view represents a window, but a window will not be shown until configured
   with the various puglInit functions and shown with puglShowWindow().
*/
PUGL_API PuglView*
puglNewView(PuglWorld* world);

/**
   Free a view created with puglNewView().
*/
PUGL_API void
puglFreeView(PuglView* view);

/**
   Return the world that `view` is a part of.
*/
PUGL_API PuglWorld*
puglGetWorld(PuglView* view);

/**
   Set the handle to be passed to all callbacks.

   This is generally a pointer to a struct which contains all necessary state.
   Everything needed in callbacks should be here, not in static variables.
*/
PUGL_API void
puglSetHandle(PuglView* view, PuglHandle handle);

/**
   Get the handle to be passed to all callbacks.
*/
PUGL_API PuglHandle
puglGetHandle(PuglView* view);

/**
   Set a hint to configure window properties.

   This only has an effect when called before puglCreateWindow().
*/
PUGL_API PuglStatus
puglSetViewHint(PuglView* view, PuglViewHint hint, int value);

/**
   Return true iff the view is currently visible.
*/
PUGL_API bool
puglGetVisible(PuglView* view);

/**
   Request a redisplay on the next call to puglDispatchEvents().
*/
PUGL_API PuglStatus
puglPostRedisplay(PuglView* view);

/**
   @}
   @anchor frame
   @name Frame
   Functions for working with the position and size of a view.
   @{
*/

/**
   Get the current position and size of the view.
*/
PUGL_API PuglRect
puglGetFrame(const PuglView* view);

/**
   Set the current position and size of the view.
*/
PUGL_API PuglStatus
puglSetFrame(PuglView* view, PuglRect frame);

/**
   Set the minimum size of the view.

   To avoid stutter, this should be called before creating the window.
*/
PUGL_API PuglStatus
puglSetMinSize(PuglView* view, int width, int height);

/**
   Set the window aspect ratio range.

   The x and y values here represent a ratio of width to height.  To set a
   fixed aspect ratio, set the minimum and maximum values to the same ratio.

   Note that setting different minimum and maximum constraints does not
   currenty work on MacOS (the minimum is used), so only setting a fixed aspect
   ratio works properly across all platforms.
*/
PUGL_API PuglStatus
puglSetAspectRatio(PuglView* view, int minX, int minY, int maxX, int maxY);

/**
   @}
   @name Windows
   Functions for working with top-level windows.
   @{
*/

/**
   A native window handle.

   On X11, this is a Window.
   On OSX, this is an NSView*.
   On Windows, this is a HWND.
*/
typedef intptr_t PuglNativeWindow;

/**
   Set the title of the window.

   This only makes sense for non-embedded views that will have a corresponding
   top-level window, and sets the title, typically displayed in the title bar
   or in window switchers.
*/
PUGL_API PuglStatus
puglSetWindowTitle(PuglView* view, const char* title);

/**
   Set the parent window before creating a window (for embedding).

   This only works when called before creating the window with
   puglCreateWindow(), reparenting is not supported.
*/
PUGL_API PuglStatus
puglSetParentWindow(PuglView* view, PuglNativeWindow parent);

/**
   Set the transient parent of the window.

   This is used for things like dialogs, to have them associated with the
   window they are a transient child of properly.
*/
PUGL_API PuglStatus
puglSetTransientFor(PuglView* view, PuglNativeWindow parent);

/**
   Create a window with the settings given by the various puglInit functions.

   @return 1 (pugl does not currently support multiple windows).
*/
PUGL_API PuglStatus
puglCreateWindow(PuglView* view, const char* title);

/**
   Show the current window.
*/
PUGL_API PuglStatus
puglShowWindow(PuglView* view);

/**
   Hide the current window.
*/
PUGL_API PuglStatus
puglHideWindow(PuglView* view);

/**
   Return the native window handle.
*/
PUGL_API PuglNativeWindow
puglGetNativeWindow(PuglView* view);

/**
   @}
   @name Graphics Context
   Functions for working with the drawing context.
   @{
*/

/**
   Graphics backend interface.
*/
typedef struct PuglBackendImpl PuglBackend;

/**
   OpenGL extension function.
*/
typedef void (*PuglGlFunc)(void);

/**
   Set the graphics backend to use.

   This needs to be called once before creating the window to set the graphics
   backend.  There are two backend accessors included with pugl:
   puglGlBackend() and puglCairoBackend(), declared in pugl_gl_backend.h and
   pugl_cairo_backend.h, respectively.
*/
PUGL_API PuglStatus
puglSetBackend(PuglView* view, const PuglBackend* backend);

/**
   Return the address of an OpenGL extension function.
*/
PUGL_API PuglGlFunc
puglGetProcAddress(const char* name);

/**
   Get the drawing context.

   The context is only guaranteed to be available during an expose.

   For OpenGL backends, this is unused and returns NULL.
   For Cairo backends, this returns a pointer to a `cairo_t`.
*/
PUGL_API void*
puglGetContext(PuglView* view);

/**
   Enter the drawing context.

   Note that pugl automatically enters and leaves the drawing context during
   configure and expose events, so it is not normally necessary to call this.
   However, it can be used to enter the drawing context elsewhere, for example
   to call any GL functions during setup.

   @param view The view being entered.
   @param drawing If true, prepare for drawing.
*/
PUGL_API PuglStatus
puglEnterContext(PuglView* view, bool drawing);

/**
   Leave the drawing context.

   This must be called after puglEnterContext() with a matching `drawing`
   parameter.

   @param view The view being left.
   @param drawing If true, finish drawing, for example by swapping buffers.
*/
PUGL_API PuglStatus
puglLeaveContext(PuglView* view, bool drawing);

/**
   @}
   @anchor interaction
   @name Interaction
   Interacting with the system and user with events.
   @{
*/

/**
   A function called when an event occurs.
*/
typedef PuglStatus (*PuglEventFunc)(PuglView* view, const PuglEvent* event);

/**
   Set the function to call when an event occurs.
*/
PUGL_API PuglStatus
puglSetEventFunc(PuglView* view, PuglEventFunc eventFunc);

/**
   Return true iff `view` has the input focus.
*/
PUGL_API bool
puglHasFocus(const PuglView* view);

/**
   Grab the input focus.
*/
PUGL_API PuglStatus
puglGrabFocus(PuglView* view);

/**
   Get clipboard contents.

   @param view The view.
   @param[out] type Set to the MIME type of the data.
   @param[out] len Set to the length of the data in bytes.
   @return The clipboard contents.
*/
PUGL_API const void*
puglGetClipboard(PuglView* view, const char** type, size_t* len);

/**
   Set clipboard contents.

   @param view The view.
   @param type The MIME type of the data, "text/plain" is assumed if NULL.
   @param data The data to copy to the clipboard.
   @param len The length of data in bytes (including terminator if necessary).
*/
PUGL_API PuglStatus
puglSetClipboard(PuglView*   view,
                 const char* type,
                 const void* data,
                 size_t      len);

/**
   Request user attention.

   This hints to the system that the window or application requires attention
   from the user.  The exact effect depends on the platform, but is usually
   something like flashing a task bar entry.
*/
PUGL_API PuglStatus
puglRequestAttention(PuglView* view);

#ifndef PUGL_DISABLE_DEPRECATED

/**
   @}
   @name Deprecated API
   @{
*/

#if defined(__clang__)
#    define PUGL_DEPRECATED_BY(name) __attribute__((deprecated("", name)))
#elif defined(__GNUC__)
#    define PUGL_DEPRECATED_BY(name) __attribute__((deprecated("Use " name)))
#else
#    define PUGL_DEPRECATED_BY(name)
#endif

/**
   Create a Pugl application and view.

   To create a window, call the various puglInit* functions as necessary, then
   call puglCreateWindow().

   @deprecated Use puglNewApp() and puglNewView().

   @param pargc Pointer to argument count (currently unused).
   @param argv  Arguments (currently unused).
   @return A newly created view.
*/
static inline PUGL_DEPRECATED_BY("puglNewView") PuglView*
puglInit(const int* pargc, char** argv)
{
	(void)pargc;
	(void)argv;

	return puglNewView(puglNewWorld());
}

/**
   Destroy an app and view created with `puglInit()`.

   @deprecated Use puglFreeApp() and puglFreeView().
*/
static inline PUGL_DEPRECATED_BY("puglFreeView") void
puglDestroy(PuglView* view)
{
	PuglWorld* const world = puglGetWorld(view);

	puglFreeView(view);
	puglFreeWorld(world);
}

/**
   Set the window class name before creating a window.
*/
static inline PUGL_DEPRECATED_BY("puglSetClassName") void
puglInitWindowClass(PuglView* view, const char* name)
{
	puglSetClassName(puglGetWorld(view), name);
}

/**
   Set the window size before creating a window.

   @deprecated Use puglSetFrame().
*/
static inline PUGL_DEPRECATED_BY("puglSetFrame") void
puglInitWindowSize(PuglView* view, int width, int height)
{
	PuglRect frame = puglGetFrame(view);

	frame.width = width;
	frame.height = height;

	puglSetFrame(view, frame);
}

/**
   Set the minimum window size before creating a window.
*/
static inline PUGL_DEPRECATED_BY("puglSetMinSize") void
puglInitWindowMinSize(PuglView* view, int width, int height)
{
	puglSetMinSize(view, width, height);
}

/**
   Set the window aspect ratio range before creating a window.

   The x and y values here represent a ratio of width to height.  To set a
   fixed aspect ratio, set the minimum and maximum values to the same ratio.

   Note that setting different minimum and maximum constraints does not
   currenty work on MacOS (the minimum is used), so only setting a fixed aspect
   ratio works properly across all platforms.
*/
static inline PUGL_DEPRECATED_BY("puglSetAspectRatio") void
puglInitWindowAspectRatio(PuglView* view,
                          int       minX,
                          int       minY,
                          int       maxX,
                          int       maxY)
{
	puglSetAspectRatio(view, minX, minY, maxX, maxY);
}

/**
   Set transient parent before creating a window.

   On X11, parent must be a Window.
   On OSX, parent must be an NSView*.
*/
static inline PUGL_DEPRECATED_BY("puglSetTransientFor") void
puglInitTransientFor(PuglView* view, uintptr_t parent)
{
	puglSetTransientFor(view, (PuglNativeWindow)parent);
}

/**
   Enable or disable resizing before creating a window.

   @deprecated Use puglSetViewHint() with @ref PUGL_RESIZABLE.
*/
static inline PUGL_DEPRECATED_BY("puglSetViewHint") void
puglInitResizable(PuglView* view, bool resizable)
{
	puglSetViewHint(view, PUGL_RESIZABLE, resizable);
}

/**
   Get the current size of the view.

   @deprecated Use puglGetFrame().

*/
static inline PUGL_DEPRECATED_BY("puglGetFrame") void
puglGetSize(PuglView* view, int* width, int* height)
{
	const PuglRect frame = puglGetFrame(view);

	*width  = (int)frame.width;
	*height = (int)frame.height;
}

/**
   Ignore synthetic repeated key events.

   @deprecated Use puglSetViewHint() with @ref PUGL_IGNORE_KEY_REPEAT.
*/
static inline PUGL_DEPRECATED_BY("puglSetViewHint") void
puglIgnoreKeyRepeat(PuglView* view, bool ignore)
{
	puglSetViewHint(view, PUGL_IGNORE_KEY_REPEAT, ignore);
}

/**
   Set a hint before creating a window.

   @deprecated Use puglSetWindowHint().
*/
static inline PUGL_DEPRECATED_BY("puglSetViewHint") void
puglInitWindowHint(PuglView* view, PuglViewHint hint, int value)
{
	puglSetViewHint(view, hint, value);
}

/**
   Set the parent window before creating a window (for embedding).

   @deprecated Use puglSetWindowParent().
*/
static inline PUGL_DEPRECATED_BY("puglSetParentWindow") void
puglInitWindowParent(PuglView* view, PuglNativeWindow parent)
{
	puglSetParentWindow(view, parent);
}

/**
   Set the graphics backend to use.

   @deprecated Use puglSetBackend().
*/
static inline PUGL_DEPRECATED_BY("puglSetBackend") int
puglInitBackend(PuglView* view, const PuglBackend* backend)
{
	return puglSetBackend(view, backend);
}

/**
   Block and wait for an event to be ready.

   This can be used in a loop to only process events via puglProcessEvents when
   necessary.  This function will block indefinitely if no events are
   available, so is not appropriate for use in programs that need to perform
   regular updates (e.g. animation).

   @deprecated Use puglPollEvents().
*/
PUGL_API PUGL_DEPRECATED_BY("puglPollEvents") PuglStatus
puglWaitForEvent(PuglView* view);

/**
   Process all pending window events.

   This handles input events as well as rendering, so it should be called
   regularly and rapidly enough to keep the UI responsive.  This function does
   not block if no events are pending.

   @deprecated Use puglDispatchEvents().
*/
PUGL_API PUGL_DEPRECATED_BY("puglDispatchEvents") PuglStatus
puglProcessEvents(PuglView* view);

#endif  /* PUGL_DISABLE_DEPRECATED */

/**
   @}
   @}
*/

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* PUGL_H_INCLUDED */
