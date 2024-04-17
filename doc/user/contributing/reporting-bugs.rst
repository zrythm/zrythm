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

In this case, please report the issue on our
`issue tracker <issue_tracker_>`_ with enough details to
help us reproduce the problem.

You should include the following details:

* a screenshot/screencast showing the problem
* a description of the problem
* steps to reproduce the problem
* OS and Zrythm version information
* last 100 lines of the log file

The following subsections explain how to provide this
information.

Obtaining a Screenshot/Screencast
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can press the :kbd:`PrintScreen` on your keyboard to
obtain a screenshot.

To obtain a screencast you can use a screen recording software
such as OBS, or :kbd:`Control-Shift-Alt-R` on GNOME.

Problem Description
~~~~~~~~~~~~~~~~~~~

You should include as many details as possible to help us
understand and reproduce the problem, otherwise we can't
do anything about it.
`Here is an example <https://gitlab.zrythm.org/zrythm/zrythm/-/issues/4202>`_
of a good bug description:

  Pixelated and misaligned timebase master and transport client icons

  As can be seen in the following screenshot, the two icons are seemingly misaligned, moved into each other and also very pixelated.

Steps to Reproduce
~~~~~~~~~~~~~~~~~~

Please give the exact steps to reproduce the issue, starting
from a blank project (if possible). We will follow these
exact steps to reproduce the error so that we can fix it. If
we can't reproduce the issue, it is very difficult to find or
fix the issue.

Here is an example of helpful steps to reproduce:

1. Create an empty project
2. Add a MIDI track
3. Create a region starting at bar 1 and ending at bar 5
4. Add a note inside the region starting at bar 2 and ending at bar 4
5. Split the note at bar 3
6. Delete the 2nd note
7. Zrythm freezes

OS and Zrythm Version Information
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You can copy the Zrythm version from the About dialog or
(preferred) use the :option:`--version option <zrythm --version>` when launching Zrythm in a terminal.

.. image:: /_static/img/terminal-version-info.png
   :align: center

You can obtain OS information from the About dialog (click
:guilabel:`Troubleshooting` then
:guilabel:`Debugging Information`)

.. image:: /_static/img/debugging-info.png
   :align: center

Log
~~~

The log file can be found in
:ref:`the location specified here <appendix/files-and-directories:Log File>`.
Please provide at least the last 100 lines. You may be asked
to provide the full log file (please compress it first).

How to Report an Issue on GitLab
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. note:: You must first register an account with us (if you don't already have one).
   You will be asked to login (or create an account) when you try to create an issue.

First, go to our `issue tracker <issue_tracker_>`_. You will see the following page.

.. figure:: /_static/img/gitlab-issues.png
   :align: center

   List of Zrythm issues

Click on :guilabel:`New Issue`. You will see the following page where you can fill in details about the issue you are facing.
Click on :guilabel:`Choose a template...` under :guilabel:`Description` and select what best describes your issue (e.g., a bug).

.. figure:: /_static/img/gitlab-new-issue-selecting-template.png
   :align: center

   Selecting an issue template

Then, enter the required details to help us understand, reproduce and fix the issue.

.. figure:: /_static/img/gitlab-filling-in-new-issue.png
   :align: center

   Filling in details according to the template

When you are done, click the :guilabel:`Create Issue` button to submit the issue to us.

.. figure:: /_static/img/gitlab-clicking-create-issue.png
   :align: center

   Submitting an issue

Existing Issues
+++++++++++++++

Please make sure to first search our issue tracker for any similar issues before making a new one (having multiple tickets about the same issue wastes our time).

If you find the issue you are having in an existing ticket, please read it and comment on it with additional information, if you have any. If you have nothing to add but want us to fix the issue faster, you may add a reaction (like a thumbs up) to let us know it's important.

If you are not sure if your issue is a duplicate, please submit it anyway.

.. tip:: We generally work on issues we consider high-priority first, however if you want something fixed quickly and are willing to pay feel free to contact us with your proposal.

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

Then, run :file:`C:\Program Files\Zrythm\bin\zrythm_debug_gdb.exe`
and make it crash. When Zrythm crashes, you
will see a Dr. Mingw window appear on the screen with error
details.

.. image:: /_static/img/drmingw-trace.png
   :align: center

Click File -> Save As... and save the error file somewhere.
Please send us this error file.

.. image:: /_static/img/drmingw-saveas.png
   :align: center

When you are done, if you wish, you may uninstall Dr. Mingw with the
following command:

.. code-block:: bash

  "C:\Program Files\Zrythm\bin\drmingw.exe" -u"

.. image:: /_static/img/drmingw-uninstall.png
   :align: center

A confirmation dialog will appear:

.. image:: /_static/img/drmingw-uninstall-confirmation.png
   :align: center
