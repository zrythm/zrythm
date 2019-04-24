robtk -- robin's LV2 UI ToolKit
===============================

robtk facilitates creating LV2 plugins UIs with emphasis to
allow porting existing gtk+ plugin UIs.

robtk provides implementations for these existing gtk+ widgets:

*   label
*   separator
*   push button
*   toggle button
*   radio button
*   spin box
*   text combo-box
*   drawing-area

as well as gtk+ container and layout objects:

*   horizontal box
*   vertical box
*   table layout

and additional widgets

*   x/y plot area
*   rgb/rgba image
*   (volume, gain) slider
*   multi-state button

A subset of gtk's functionality and widgets were re-implemented in cairo.
On compile-time GTK+ as well as openGL variants of the UI can be produced.

The complete toolkit consists of header files to be included with
the UI source-code and maps functions to the underlying implementation
e.g. `robtk_lbl_new()` to `gtk_label_new()`.
no additional libraries or dependencies are required.

Similar to widgets and layout, the event-structure and callbacks of robtk
lean onto the GTK API providing

*   mouse-events: move, click up/down, scoll
*   widget-events: enter, leave
*   widget-allocation: size-request, allocate, position
*   window-events: resize, limit-size
*   widget-exposure: complete and partial redraw

robtk includes LV2-GUI wrappers and gnu-make definitions for easy use
in a LV2 project. Currently it is used by meters.lv2, sisco.lv2, tuna.lv2,
mixtri.lv2, fil4.lv2 and setBfree
