# You can set these variables from the command line.

# module_name === project in conf.py, use \_ instead of _ if module name contains _
module_name := BMCV
output_file_prefix := SOPHON_BMCV_Technical_Reference_Manual

LANG := zh

ifeq ($(LANG), en)
  SOURCEDIR     = source_en
  pdf_name      = $(output_file_prefix).pdf
else ifeq ($(LANG), zh)
  SOURCEDIR     = source_zh
  pdf_name      = $(output_file_prefix).pdf
else
  exit 1
endif

#echo $SOURCEDIR
#exit 1

SPHINXOPTS    ?= -W -n
SPHINXBUILD   ?= sphinx-build
BUILDDIR      = build

# Put it first so that "make" without argument is like "make help".
help:
	@$(SPHINXBUILD) -M help "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

.PHONY: help Makefile

# Catch-all target: route all unknown targets to Sphinx using the new
# "make mode" option.  $(O) is meant as a shortcut for $(SPHINXOPTS).
%: Makefile
	@$(SPHINXBUILD) -M $@ "$(SOURCEDIR)" "$(BUILDDIR)" $(SPHINXOPTS) $(O)

pdf: latex
	@cd $(BUILDDIR)/latex && xelatex $(module_name).tex
	@cd $(BUILDDIR)/latex && xelatex $(module_name).tex
	@mv $(BUILDDIR)/latex/$(module_name).pdf $(BUILDDIR)/"${pdf_name}" && rm -rf $(BUILDDIR)/latex

web: html
	#@python3 -m http.server --directory build/html

clean:
	@rm -rf $(BUILDDIR)

livehtml:
	@sphinx-autobuild -b html $(SOURCEDIR) $(BUILDDIR)/html

clean_pdf:
	@rm -rf $(BUILDDIR)/"${pdf_name}" && rm -rf $(BUILDDIR)/doctrees

