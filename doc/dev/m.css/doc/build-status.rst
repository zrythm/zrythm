Build Status
############

:summary: CI build status of m.css
:footer:
    .. raw:: html

        <style>
          table.build-status th, table.build-status td {
            text-align: center;
            vertical-align: middle;
          }
          table.build-status td {
            padding: 0;
            min-width: 5rem;
          }
          table.build-status a {
            display: block;
            width: 100%;
            height: 100%;
            padding: 0.25rem 0.5rem;
            text-decoration: none;
          }
        </style>
        <script>

    .. raw:: html
        :file: build-status.js

    .. raw:: html

        </script>

Show builds for:

-   `master branch <{filename}/build-status.rst>`_
-   `next branch <{filename}/build-status.rst?mosra/m.css=next>`_

.. container:: m-container-inflate

    .. container:: m-scroll

        .. can't name it HTML, because then Pelican tries to parse it (and
            can't disable HTML parsing because we need that for layout tests)

        .. raw:: html
            :file: build-status.html.in
