.. SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
   SPDX-License-Identifier: GFDL-1.3-invariants-or-later
.. This is part of the Zrythm Manual.
   See the file index.rst for copying conditions.

Reporting Bugs
==============

There are three types of bugs:

* wrong or unexpected behavior that don't cause errors
* soft errors
* hard errors (crashes)

Incorrect Behavior
------------------

In this case, please report the issue on our bug tracker with
enough details to reproduce the problem.

Soft Errors
-----------

In this case Zrythm will display a bug report dialog. Please
follow the instructions in this dialog to report the issue.

Crashes
-------

If Zrythm crashed, then a bug report dialog may not be shown
or the log file may not contain enough information to help us
fix the issue. In this case, please follow the steps below to
generate a core dump or a backtrace.

Generating Core Dumps on GNU/Linux
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You will need to type some commands into a terminal. Also,
please make sure you have ``systemd-coredump`` or the equivalent
package installed on your distro.

.. note:: The following steps apply to systemd users (the vast
   majority of GNU/Linux users). For non-systemd users, you can
   usually obtain the core file in the current directory after
   running the first 2 commands below and running Zrythm until
   it crashes.

First, enable core dumps:

.. code-block:: bash

  ulimit -c unlimited

Tell Linux to add the PID (Process ID) to the core dump:

.. code-block:: bash

  sudo sysctl -w kernel.core_uses_pid=1

Run Zrythm and make it crash. An intermediate core dump file
should now be generated in :file:`/var/lib/systemd/coredump`.
Use ``coredumpctl`` to verify:

.. code-block:: bash

  coredumpctl list -1

You should see something like the following:

.. code-block:: text

  TIME                          PID  UID  GID SIG    COREFILE EXE
  Thu 2023-08-24 05:48:14 CDT 23179 1000 1000 SIGILL present  /opt/zrythm-1.0.0.beta.4.12.1/bin/zrythm

Note that under `COREFILE` it says `present`, so a core file
exists.

Now we can finally export the core dump file to a
location of our choice (in this case in my :file:`Downloads`
directory)  by passing the PID to ``coredumpctl``:

.. code-block:: bash

  coredumpctl dump 23179 --output=/home/alex/Downloads/core.23179

.. important:: Change the file path given to ``--output`` to
   your desired location.

Please send us the core dump file (in the example above it can
be found at :file:`/home/alex/Downloads/core.23179`).

.. seealso::
   * `c - Find which assembly instruction caused an Illegal Instruction error without debugging - Stack Overflow <https://stackoverflow.com/questions/10354147/find-which-assembly-instruction-caused-an-illegal-instruction-error-without-debu/40223712#40223712>`_
   * `linux - How do I analyze a program's core dump file with GDB when it has command-line parameters? - Stack Overflow <https://stackoverflow.com/questions/8305866/how-do-i-analyze-a-programs-core-dump-file-with-gdb-when-it-has-command-line-pa>`_

Getting a Backtrace on Windows
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Open the Command Prompt app as an administrator by searching
for `cmd` in the start menu, then right clicking on the app
and selecting `Run as Administrator`. Then, type the following
and press enter/return:

.. code-block:: bash

  "C:\Program Files\Zrythm\bin\drmingw.exe" -i"

.. image:: /_static/img/drmingw-install.png
   :align: center

This will install DrMingw as the default debugger (you can
uninstall it afterwards).

.. image:: /_static/img/drmingw-install-confirmation.png
   :align: center

Then, run Zrythm and make it crash. When Zrythm crashes, you
will see a Dr. Mingw window appear on the screen with error
details.

.. image:: /_static/img/drmingw-trace.png
   :align: center

Click File -> Save As... and save the error file somewhere.
Please send us this error file.

.. image:: /_static/img/drmingw-saveas.png
   :align: center

When you are done, you may uninstall Dr. Mingw with the
following command:

.. code-block:: bash

  "C:\Program Files\Zrythm\bin\drmingw.exe" -u"

.. image:: /_static/img/drmingw-uninstall.png
   :align: center

A confirmation dialog will appear:

.. image:: /_static/img/drmingw-uninstall-confirmation.png
   :align: center
