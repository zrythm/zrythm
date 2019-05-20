Page title should not be hyphenated. "But quotes yes."
######################################################

:header: A header that *should be* hyphenated. "Quotes."
:footer: The footer *should be* hyphenated as well. "Quotes."
:summary: Page summary in the meta tag should not be hyphenated. "But quotes yes."
:description: Neither the description. "But quotes yes."
:css: and-definitely-not-css-links.css

.. role:: raw-html(raw)
    :format: html

Page content should be hyphenated. ``But "code" should not be hyphenated.``

::

    Neither preformatted text. "No quotes for you."

.. container:: m-note m-info

    Nested content should be hyphenated also! And also **bold sections** or
    *italics text*. Oh and "quotes should be smart" --- with proper em-dash,
    8--22 (en-dash) and ellipsis...

`Link titles can be hyphenated as well. <http://blog.mosra.cz/>`_ But inline
verbatim stuff shouldn't: :raw-html:`hello "this" is not hyphenated`. Neither
verbatim blocks:

.. raw:: html

    "quote" hyphenation

.. class:: language-cs

    Odstavec v češtině. "Uvozovky" fungují jinak a dělení slov jakbysmet.

Links
=====

Links with titles that are URLs (or e-mail addresses) shouldn't be hyphenated
either:

-   info@magnum.graphics
-   https://magnum.graphics
-   `Links`_ without refuri should not give an error
