#!/usr/bin/env python

import math
import os
import sys

import matplotlib
from matplotlib import pyplot
from matplotlib.pyplot import *

matplotlib.rc('text', **{
    'usetex': True})
matplotlib.rc('font', **{
    'family':     'serif',
    'serif':      'Times',
    'sans-serif': 'Helvetica',
    'monospace':  'Courier'})

pyplot.subplots_adjust(wspace=0.2, hspace=0.2)

class SensibleScalarFormatter(matplotlib.ticker.ScalarFormatter):
    "ScalarFormatter which rounds order of magnitude to a multiple of 3"
    def __init__(self):
        matplotlib.ticker.ScalarFormatter.__init__(self)
        self.set_powerlimits([-6, 6])
        self.set_scientific(True)

    def _set_orderOfMagnitude(self, range):
        # Calculate "best" order in the usual way
        matplotlib.ticker.ScalarFormatter._set_orderOfMagnitude(self, range)

        # Round down to sensible (millions, billions, etc) order
        self.orderOfMagnitude = self.orderOfMagnitude - (self.orderOfMagnitude % 3)

        self.set_scientific(True)

file_prefix = os.path.commonprefix(sys.argv[1:])
n_plots     = len(sys.argv) - 2
for i in range(n_plots):
    filename = sys.argv[i+2]
    file = open(filename, 'r')

    # pyplot.clf()

    ax = pyplot.subplot(math.ceil(math.sqrt(n_plots)),
                        math.ceil(math.sqrt(n_plots)),
                        i + 1)

    ax.xaxis.set_major_formatter(SensibleScalarFormatter())
    ax.yaxis.set_major_formatter(SensibleScalarFormatter())
    for a in ['x', 'y']:
        ax.grid(which='major', axis=a, zorder=1,
                linewidth=0.5, linestyle=':', color='0', dashes=[0.5, 8.0])

    header  = file.readline()
    columns = header[1:].split()

    pyplot.xlabel('Elements')
    pyplot.ylabel('Time (s)')

    times = []
    for i in columns:
        times.append([])
    for line in file:
        if line[0] == '#':
            continue;

        fields = line.split()
        num    = 0
        for i in fields:
            times[num].append([float(i)])
            num += 1

    file.close()

    for i in range(len(times) - 1):
        matplotlib.pyplot.plot(times[0], times[i + 1], '-o', label=columns[i + 1])

    pyplot.legend(loc='upper left',
                  handletextpad=0.15, borderpad=0.20, borderaxespad=0,
                  labelspacing=0.10, columnspacing=0,
                  framealpha=0.90)

    pyplot.title(os.path.splitext(filename[len(file_prefix):])[0].title())

    # out = filename.replace('.dat', '.png')
    # print('Writing %s' % out)
    # matplotlib.pyplot.savefig(out, bbox_inches='tight', pad_inches=0.025)

print('Writing %s' % sys.argv[1])
matplotlib.pyplot.tight_layout()
matplotlib.pyplot.savefig(sys.argv[1])

#matplotlib.pyplot.show()
