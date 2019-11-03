This is the API documentation for Pugl.
The complete C API is documented in the [Pugl](@ref pugl) group,
and the C++ wrapper in the [Puglmm](@ref puglmm) group.

The Pugl API revolves around two main objects:
the [World](@ref world) and the [View](@ref view).
An application creates a single world to manage system-level state,
then creates one or more views to display.

## View creation

Creating a visible view is a multi-step process.
A new view allocated with #puglNewView does not yet represent a "real" system window.
To display, it must first have a [backend set](@ref puglSetBackend),
and be configured by [setting hints](@ref puglSetViewHint)
and [configuring the frame](@ref frame).

Once the view is configured,
the corresponding window can be [created](@ref puglCreateWindow)
and [shown](@ref puglShowWindow).

Note that a view does not necessary correspond to a top-level system window.
To create a view within another window,
call #puglSetParentWindow before #puglCreateWindow.

## Interaction

Interaction with the user and system happens via [events](@ref interaction).
Before creating a window,
a view must have an [event handler](@ref PuglEventFunc) set with #puglSetEventFunc.
This handler is called whenever something happens that the view must respond to.
This includes user interaction like mouse and keyboard input,
and system events like window resizing and exposure (drawing).

## Event Loop

Two functions are used to drive the event loop:

 * #puglPollEvents waits for events to become available.
 * #puglDispatchEvents processes all pending events.

Redrawing is accomplished by calling #puglPostRedisplay,
which posts an expose event to the queue.
