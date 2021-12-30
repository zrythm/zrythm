"""
This is a reST markup explaining the following code, compatible with
`Sphinx Gallery <https://sphinx-gallery.github.io/>`_.
"""

# You can convert the file to a Jupyter notebook using the
# sphx_glr_python_to_jupyter.py utility from Sphinx Gallery.

import math

sin = math.sin(0.13587)
print(sin)

#%%
# And a sum with itself turns it into two sins, because the following holds:
#
# .. math::
#
#   2 a = a + a
#

two_sins = sin + sin
if two_sins != 2*sin:
    print("Assumptions broken. Restart the universe.")
