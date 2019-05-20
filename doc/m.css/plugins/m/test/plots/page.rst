m.plots
#######

Note: the test uses DejaVu Sans instead of Source Sans Pro in order to have
predictable rendering on the CIs.

.. raw:: html

    <style>
    div.m-plot svg { font-family: DejaVu Sans; }
    </style>

.. plot:: A plot with a single color
    :type: barh
    :labels:
        First
        Second
    :units: meters, i guess?
    :values: 15 30
    :colors: success

.. plot:: A plot with separate colors, extra labels, error bars and custom height
    :type: barh
    :labels:
        January
        February
        March
    :labels_extra:
        a paradise
        ..
        hell!
    :units: Mondays
    :values: 3 4 5
    :errors: 0.1 2.1 1.0
    :colors: success info danger
    :bar_height: 0.75
