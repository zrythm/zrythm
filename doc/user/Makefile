# Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
#
# This file is part of Zrythm
#
# Zrythm is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# Zrythm is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU General Affero Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#
# ----------------------------------------------------------------------
#
# Minimal makefile for Sphinx documentation
#

# You can set these variables from the command line.
SPHINXOPTS    =
SPHINXBUILD   = sphinx-build
SOURCEDIR     = .
BUILDDIR      = _build
RENDERDIR     = _rendered

# Put it first so that "make" without argument is like "make help".
help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

.PHONY: bundle

LANGUAGES = en de fr it es ja pt pt_BR ru zh nb_NO

# Skip chinese latexpdf because it doesn't support Chinese yet
# bundle: html epub
bundle: html latexpdf epub
	for lang in $(LANGUAGES) ; do \
		mkdir -p $(RENDERDIR)/$$lang && \
		cp -R $(BUILDDIR)/$$lang/html/* $(RENDERDIR)/$$lang/ ; \
		if [[ "x$$lang" == "xzh" ]] ; then \
			echo "skipping $$lang pdf" ; \
		else \
			cp $(BUILDDIR)/$$lang/latex/Zrythm.pdf $(RENDERDIR)/$$lang/ ; \
		fi ; \
		cp $(BUILDDIR)/$$lang/epub/Zrythm.epub $(RENDERDIR)/$$lang/ && \
		zip -r $(RENDERDIR)/$$lang/Zrythm-html.zip $(BUILDDIR)/$$lang/html/* ; \
	done


.PHONY: help Makefile

.PHONY: gettext
gettext:
	$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
	for lang in $(LANGUAGES) ; do \
		sphinx-intl update -p $(BUILDDIR)/gettext -l $$lang ; \
	done

# Catch-all target: route all unknown targets to Sphinx using the new
# "make mode" option.  $(O) is meant as a shortcut for $(SPHINXOPTS).
# Skip chinese latexpdf because it doesn't support Chinese yet
%: Makefile
	for lang in $(LANGUAGES) ; do \
		if [[ "x$@" == "xlatexpdf" ]] && [[ "x$$lang" == "xzh" ]] ; then \
			echo "skipping $@ $$lang" ; \
		else \
			echo "$@ $$lang" ; \
			SPHINXOPTS="-D language=$$lang" && \
			$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)/$$lang" $(SPHINXOPTS) $$SPHINXOPTS $(O) ; \
		fi \
	done
