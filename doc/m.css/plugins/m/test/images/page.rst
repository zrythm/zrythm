m.images
########

:summary: no.

Image:

.. image:: {static}/ship.jpg

Image with link:

.. image:: {static}/ship.jpg
    :target: {static}/ship.jpg

Image, class on top, custom alt:

.. image:: {static}/ship.jpg
    :class: m-fullwidth
    :alt: A Ship

Image with link, class on top:

.. image:: {static}/ship.jpg
    :target: {static}/ship.jpg
    :class: m-fullwidth

Image with custom width, height and scale (scale gets picked):

.. image:: {static}/ship.jpg
    :width: 4000px
    :height: 8000px
    :scale: 25%

Image with custom width and height (width gets picked):

.. image:: {static}/ship.jpg
    :width: 300px
    :height: 8000px

Image with custom height:

.. image:: {static}/ship.jpg
    :height: 100px

Figure:

.. figure:: {static}/ship.jpg

    A Ship

    Yes.

Figure with link, scale and only a caption:

.. figure:: {static}/ship.jpg
    :target: {static}/ship.jpg
    :scale: 37%

    A Ship

Figure with link and class on top:

.. figure:: {static}/ship.jpg
    :target: {static}/ship.jpg
    :figclass: m-fullwidth

    A Ship

Figure with a width:

.. figure:: {static}/ship.jpg
    :width: 250px

    A Ship

Figure with a height:

.. figure:: {static}/ship.jpg
    :target: {static}/ship.jpg
    :height: 200px

    A Ship

Image grid, second row with a custom title and no title:

.. image-grid::

    {static}/ship.jpg
    {static}/flowers.jpg

    {static}/flowers.jpg A custom title
    {static}/ship.jpg ..

Image grid with a PNG file, JPEG with sparse EXIF data, JPEG with no EXIF data
and long exposure (>1 second):

.. image-grid::

    {static}/tiny.png
    {static}/sparseexif.jpg
    {static}/noexif.jpg
    {static}/longexposure.jpg
