# This is part of the Zrythm Manual.
# Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
# See the file index.rst for copying conditions.
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

LANGUAGES = en en_GB de fr gl it es ja pt pt_BR ru nb_NO zh_Hans zh_Hant

GUILE_DOCS_SRCDIR=/tmp
GUILE_DOCS_DESTDIR=/tmp
.PHONY: update-guile-docs
update-guile-docs:
	echo "copying from $(GUILE_DOCS_SRCDIR) to $(GUILE_DOCS_DESTDIR)"
	cp -f $(GUILE_DOCS_SRCDIR)/*.rst $(GUILE_DOCS_DESTDIR)/

# Skip chinese latexpdf because it doesn't support Chinese yet
# bundle: html epub
.PHONY: bundle
bundle: html #latexpdf #epub
	for lang in $(LANGUAGES) ; do \
		mkdir -p $(RENDERDIR)/$$lang && \
		cp -R $(BUILDDIR)/$$lang/html/* $(RENDERDIR)/$$lang/ ; \
		cp $(BUILDDIR)/$$lang/epub/Zrythm.epub $(RENDERDIR)/$$lang/ || true && \
		zip -r $(RENDERDIR)/$$lang/Zrythm-html.zip $(BUILDDIR)/$$lang/html/* ; \
	done


.PHONY: help Makefile

.PHONY: gettext
gettext:
	$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)
	for lang in $(LANGUAGES) ; do \
		if [[ "x$$lang" == "xen" ]] ; then \
		  echo "sphinx-intl: skipping english (default)" ; \
		else \
			sphinx-intl update -p $(BUILDDIR)/gettext -l $$lang ; \
		fi \
	done
	./post_process_po.py

# Catch-all target: route all unknown targets to Sphinx using the new
# "make mode" option.  $(O) is meant as a shortcut for $(SPHINXOPTS).
# Skip chinese latexpdf because it doesn't support Chinese yet
%: Makefile
	for lang in $(LANGUAGES) ; do \
		if [[ "x$@" == "xlatexpdf" ]] && [[ "x$$lang" == "xzh" ]] ; then \
			echo "skipping $@ $$lang" ; \
		else \
			echo "$@ $$lang" ; \
			if [[ "x$$lang" == "xen" ]] ; then \
				$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)/$$lang" $(SPHINXOPTS) $$SPHINXOPTS $(O) ; \
			else \
				SPHINXOPTS="-D language=$$lang" && \
					$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)/$$lang" $(SPHINXOPTS) $$SPHINXOPTS $(O) ; \
			fi \
		fi \
	done
