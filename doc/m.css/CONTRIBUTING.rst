Contributing guide
##################

Bug reports, feature requests or code contributions are always very welcome.
To make things easier, here are a few tips:

Reporting bugs, requesting features
===================================

-   Best way to report bugs and request new features is to use GitHub
    `issues <https://github.com/mosra/m.css/issues>`_, but you can contact the
    author also in any other way.

Code contribution
=================

-   Setting up and using m.css for your own project is described in the
    `documentation <https://mcss.mosra.cz/>`_.
-   Documentation and the website is an essential part of the project and every
    larger theme, code or CSS contribution should be reflected there.
    Documentation and website content is written in reStructuredText and
    resides in the `doc/ <doc>`_ subdirectory. Please verify that all your
    changes there do not break the website build; see `Building the site`_
    below for a guide how to build it.
-   Python code and Jinja2 templates should be accompanied by tests. See
    `Running tests`_ below for a guide.
-   Best way to contribute is using GitHub `pull requests <https://github.com/mosra/m.css/pulls>`_
    --- fork the repository and make a pull request from a feature branch. You
    can also send patches via e-mail or contact the author in any other way.
-   All your code will be released under license of the project (see `COPYING <COPYING>`_
    file for details), so make sure you and your collaborators (or employers)
    have no problems with it. If you create new files, don't forget to add a
    license header (verbatim copied from other files) and don't forget to add
    yourself to license header of files you added or significantly modified
    (including documentation pages), for example::

        /*
            This file is part of m.css.

            Copyright © 2017, 2018, 2019 Vladimír Vondruš <mosra@centrum.cz>
            Copyright © YEAR YOUR_NAME <your@mail.com>

            Permission is hereby granted, free of charge, to any person obtaining a
            ...

Building the site
=================

The m.css website makes use of all the m.css features, which means that it also
needs all the possible dependencies, combined. Sorry in advance :)

On ArchLinux:

.. code:: sh

    sudo pacman -S texlive-most pelican python-pillow python-matplotlib
    cower -d python-pyphen # Build the python-pyphen package from AUR

On Ubuntu you need these:

.. code:: sh

    sudo apt install texlive-base texlive-latex-extra texlive-fonts-extra python3-matplotlib graphviz
    pip3 install pelican Pyphen Pillow

Once you have all the dependencies, simply go to the ``site/`` subdirectory and
start development server there. The live-reloading website will appear on
http://localhost:8000.

.. code:: sh

    cd site
    pelican -Dlr

Running tests
=============

Each bigger lump of Python code, Jinja2 template markup or JS code has tests.
There are no visual tests for the CSS style at the moment. Running the tests:

.. code:: sh

    cd pelican-theme
    python -m unittest

    cd plugins
    python -m unittest

    cd documentation
    python -m unittest
    node test_doxygen/test-search.js

Code coverage needs `coverage.py <https://coverage.readthedocs.io/>`_ for
Python (use ``pip`` or your system package) and
`istanbul <https://istanbul.js.org/>`_ for JS. Install it locally using ``npm``
to avoid polluting the whole system with crap:

.. code:: sh

    cd documentation
    npm install istanbul

There is no possibility of getting code coverage for Jinja2 templates, though.

.. code:: sh

    cd documentation
    coverage run -m unittest ; coverage html
    # open htmlcov/index.html in your browser

    cd documentation
    node ./node_modules/istanbul/lib/cli.js cover test_doxygen/test-search.js
    # open coverage/lcov-report/index.html in your browser

    cd plugins
    coverage run -m unittest ; coverage html
    # open htmlcov/index.html in your browser

Test organization: files named ``test_something.py`` take their input from
``something[_name]`` directories, ``name`` corresponds to given test class. In
case of Doxygen, comment-out the line that removes the ``html`` directory in
``__init__.py`` to see all test output files.

The project is built on Travis CI on Linux with Python 3.4, 3.5 and 3.6;
Doxygen theme is tested only on 3.6 and math rendering is disabled as it's
impossible to get it working on Travis (https://github.com/mosra/m.css/pull/75).
Build and coverage status is presented at https://mcss.mosra.cz/build-status/.

Contact
=======

-   Website --- https://mcss.mosra.cz
-   GitHub --- https://github.com/mosra/m.css
-   Gitter --- https://gitter.im/mosra/m.css
-   Author's Twitter --- https://twitter.com/czmosra
-   E-mail --- mosra@centrum.cz
-   Jabber --- mosra@jabbim.cz
