-------------------------------
-  README for DISTRHO Ports  -
-----------------------------

DISTRHO is an open source project that has the goal of making cross-platform plugins and Linux ports.

<b>This repository contains the Linux ports</b>.

-----------------------------------------------------------------------------------------
---- BUILD DEPENDENCIES

To build plugins, you first need to install the following dependencies:

All OSes:

- premake (version 3)


Linux: (development versions of these)

- ALSA
- freetype2
- OpenGL/Mesa
- X11 core and extensions (XShm, XRender and XCursor)


-----------------------------------------------------------------------------------------
---- BUILD and INSTALL

In order to build the plugins, first run:

`$ ./scripts/premake-update.sh _OS_`

where _OS_ can be 'linux', 'mac' or 'mingw'.<br/>
This operation requires 'premake' (version 3) to be installed on your system.


You are now ready to start building. Run this on the source root folder:

`$ make`

If you just want to build specific plugin versions, you can use 'make lv2' or 'make vst'.


To build in debug mode, use this:

`$ make CONFIG=Debug`
